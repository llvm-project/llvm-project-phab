//===----- FunctionAttrsTD.cpp - Deduce Function Parameter Attributes -----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This pass traverses the call graph top-down in order to deduce
// function-parameter attributes for local functions based on properties of
// their callers (nonnull, dereferenceable, noalias).
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/IPO.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DepthFirstIterator.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Analysis/CaptureTracking.h"
#include "llvm/Analysis/Loads.h"
#include "llvm/Analysis/MemoryBuiltins.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/DataTypes.h"
#include "llvm/Support/Debug.h"
#include "llvm/Target/TargetLibraryInfo.h"
#include "llvm/Transforms/Utils/Local.h"
using namespace llvm;

#define DEBUG_TYPE "functionattrs-td"

STATISTIC(NumNonNull, "Number of arguments marked nonnull");
STATISTIC(NumNoAlias, "Number of arguments marked noalias");
STATISTIC(NumDereferenceable, "Number of arguments marked dereferenceable");

namespace {
  class FunctionAttrsTD : public ModulePass {
    const DataLayout *DL;
    AliasAnalysis *AA;
    TargetLibraryInfo *TLI;

    bool addFuncAttrs(Function &F, DenseMap<Function *, DominatorTree *> &DTs);

  public:
    static char ID; // Pass identification, replacement for typeid
    explicit FunctionAttrsTD();
    bool runOnModule(Module &M) override;

    void getAnalysisUsage(AnalysisUsage &AU) const override {
      AU.setPreservesCFG();
      AU.addRequired<CallGraphWrapperPass>();
      AU.addPreserved<CallGraphWrapperPass>();
      AU.addRequired<TargetLibraryInfo>();
      AU.addPreserved<TargetLibraryInfo>();
      AU.addRequired<AliasAnalysis>();
      AU.addPreserved<AliasAnalysis>();
    }
  };
} // end anonymous namespace

char FunctionAttrsTD::ID = 0;
INITIALIZE_PASS(FunctionAttrsTD, "functionattrs-td",
                "Top-down function parameter attributes", false, false)

FunctionAttrsTD::FunctionAttrsTD() : ModulePass(ID) {
  initializeFunctionAttrsTDPass(*PassRegistry::getPassRegistry());
}

bool FunctionAttrsTD::addFuncAttrs(Function &F,
       DenseMap<Function *, DominatorTree *> &DTs) {
  bool MadeChange = false;
  if (F.isDeclaration() || !F.hasLocalLinkage() || F.arg_empty() ||
      F.use_empty())
    return MadeChange;

  struct AttrState {
    bool IsNonNull;
    bool IsNoAlias;
    bool IsDereferenceable;
    uint64_t MaxSize;

    AttrState() : IsNonNull(true), IsNoAlias(true),
                  IsDereferenceable(true), MaxSize(UINT64_MAX) {}

    bool any() const {
      return IsNonNull || IsNoAlias || IsDereferenceable;
    }

    void setNone() {
      IsNonNull = false;
      IsNoAlias = false;
      IsDereferenceable = false;
    }
  };

  // For each argument, keep track of whether it is donated or not.
  // The bool is set to true when found to be non-donated.
  SmallVector<AttrState, 16> ArgumentState;
  ArgumentState.resize(F.arg_size());

  for (Use &U : F.uses()) {
    User *UR = U.getUser();
    // Ignore blockaddress uses.
    if (isa<BlockAddress>(UR)) continue;
    
    // Used by a non-instruction, or not the callee of a function, do not
    // transform.
    if (!isa<CallInst>(UR) && !isa<InvokeInst>(UR))
      return MadeChange;
    
    CallSite CS(cast<Instruction>(UR));
    if (!CS.isCallee(&U))
      return MadeChange;

    // Check out all of the arguments. Note that we don't inspect varargs here.
    CallSite::arg_iterator AI = CS.arg_begin();
    Function::arg_iterator Arg = F.arg_begin();
    for (unsigned i = 0, e = ArgumentState.size(); i != e; ++i, ++AI, ++Arg) {
      // If we've already excluded this argument, ignore it.
      if (!ArgumentState[i].any())
        continue;

      Type *Ty = (*AI)->getType();
      if (!Ty->isPointerTy()) {
        ArgumentState[i].setNone();
        continue;
      }

      if (ArgumentState[i].IsNonNull && !Arg->hasNonNullAttr()) {
        if (!isKnownNonNull(*AI, TLI))
          ArgumentState[i].IsNonNull = false;
      }

      Type *ETy = Ty->getPointerElementType();
      if (!DL || !ETy->isSized()) {
        ArgumentState[i].IsDereferenceable = false;
      } else if (ArgumentState[i].IsDereferenceable &&
                 ArgumentState[i].MaxSize > Arg->getDereferenceableBytes()) {
        uint64_t TypeSize = DL->getTypeStoreSize(ETy);
        if (!isSafeToLoadUnconditionally(*AI, CS.getInstruction(),
                                         getKnownAlignment(*AI, DL), DL)) {
          ArgumentState[i].IsDereferenceable = false;
        } else {
          uint64_t Size;
          if (!getObjectSize(*AI, Size, DL, TLI)) {
            Size = TypeSize;
          } else {
	    // We've already verified that a load of the type size would not
	    // trap, so we can use at least that size.
            Size = std::max(Size, TypeSize);
          }

          ArgumentState[i].MaxSize = std::min(ArgumentState[i].MaxSize, Size);
          if (!ArgumentState[i].MaxSize)
            ArgumentState[i].IsDereferenceable = false;
        }
      }
     
      if (isa<GlobalValue>(*AI)) {
        ArgumentState[i].IsNoAlias = false;
      } else if (ArgumentState[i].IsNoAlias && !Arg->hasNoAliasAttr()) {
        bool NoAlias = true;
        for (CallSite::arg_iterator BI = CS.arg_begin(), BIE = CS.arg_end();
             BI != BIE; ++BI) {
          if (BI == AI)
            continue;

          if (AA->alias(*AI, *BI) != AliasAnalysis::NoAlias) {
            NoAlias = false;
            break;
          }
        }

        SmallVector<Value*, 16> UObjects;
        GetUnderlyingObjects(*AI, UObjects, DL);
        for (Value *V : UObjects)
          if (!isIdentifiedFunctionLocal(V)) {
            NoAlias = false;
            break;
          }

        if (NoAlias) {
          DominatorTree *DT;
          Function *Caller = CS.getInstruction()->getParent()->getParent();
          auto DTI = DTs.find(Caller);
          if (DTI == DTs.end()) {
            DT = new DominatorTree;
            DT->recalculate(*Caller);
            DTs[Caller] = DT;
          } else {
            DT = DTI->second;
          }

          if (PointerMayBeCapturedBefore(*AI, /* ReturnCaptures */ false,
                                         /* StoreCaptures */ false,
                                         CS.getInstruction(), DT))
            ArgumentState[i].IsNoAlias = false;
        } else {
          ArgumentState[i].IsNoAlias = false;
        }
      }
    }

    bool HaveCand = false;
    for (unsigned i = 0, e = ArgumentState.size(); i != e; ++i)
      if (ArgumentState[i].any()) {
        HaveCand = true;
        break;
      }

    if (!HaveCand)
      break;
  }

  Function::arg_iterator AI = F.arg_begin();
  for (unsigned i = 0, e = ArgumentState.size(); i != e; ++i, ++AI) {
    if (!ArgumentState[i].any() || AI->hasInAllocaAttr() || AI->hasByValAttr())
      continue;

    if (!AI->hasNonNullAttr() && ArgumentState[i].IsNonNull &&
        (!ArgumentState[i].IsDereferenceable ||
         AI->getType()->getPointerAddressSpace() != 0)) {
      AttrBuilder B;
      B.addAttribute(Attribute::NonNull);

      AI->addAttr(AttributeSet::get(F.getContext(), AI->getArgNo() + 1, B));
      ++NumNonNull;
      MadeChange = true;
    }

    if (!AI->hasNoAliasAttr() && ArgumentState[i].IsNoAlias) {
      AttrBuilder B;
      B.addAttribute(Attribute::NoAlias);

      AI->addAttr(AttributeSet::get(F.getContext(), AI->getArgNo() + 1, B));
      ++NumNoAlias;
      MadeChange = true;
    }

    if (!AI->getDereferenceableBytes() && ArgumentState[i].IsDereferenceable) {
      AttrBuilder B;
      B.addDereferenceableAttr(ArgumentState[i].MaxSize);

      AI->addAttr(AttributeSet::get(F.getContext(), AI->getArgNo() + 1, B));
      ++NumDereferenceable;
      MadeChange = true;
    }
  }

  return MadeChange;
}

bool FunctionAttrsTD::runOnModule(Module &M) {
  AA = &getAnalysis<AliasAnalysis>();
  TLI = &getAnalysis<TargetLibraryInfo>();
  DataLayoutPass *DLP = getAnalysisIfAvailable<DataLayoutPass>();
  DL = DLP ? &DLP->getDataLayout() : nullptr;

  CallGraph &CG = getAnalysis<CallGraphWrapperPass>().getCallGraph();
  CallGraphNode *ExternalNode = CG.getExternalCallingNode();
  bool Changed = false;

  DenseMap<Function *, DominatorTree *> DTs;
  for (df_iterator<CallGraphNode*> I = df_begin(ExternalNode),
       IE = df_end(ExternalNode); I != IE; ++I) {
    Function *F = (*I)->getFunction();
    if (!F)
      continue;
    DEBUG(dbgs() << "FATD visiting: " << F->getName() << "\n");

    Changed |= addFuncAttrs(*F, DTs);
  }

  return Changed;
}

ModulePass *llvm::createFunctionAttrsTDPass() { return new FunctionAttrsTD(); }

