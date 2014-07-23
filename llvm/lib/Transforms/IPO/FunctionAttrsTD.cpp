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
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SCCIterator.h"
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
STATISTIC(NumAlign, "Number of arguments marked with enhanced alignment");

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
    bool IsAlign;
    uint64_t MaxSize, MaxAlign;

    AttrState() : IsNonNull(true), IsNoAlias(true),
                  IsDereferenceable(true), IsAlign(true),
                  MaxSize(UINT64_MAX), MaxAlign(UINT64_MAX) {}

    bool any() const {
      return IsNonNull || IsNoAlias || IsDereferenceable || IsAlign;
    }

    void setNone() {
      IsNonNull = false;
      IsNoAlias = false;
      IsDereferenceable = false;
      IsAlign = false;
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
        if (!(*AI)->isDereferenceablePointer(DL) &&
            !isSafeToLoadUnconditionally(*AI, CS.getInstruction(),
                                         getKnownAlignment(*AI, DL), DL)) {
          ArgumentState[i].IsDereferenceable = false;
          // FIXME: isSafeToLoadUnconditionally does not understand memset.
          // FIXME: We can use getObjectSize for most things, but for mallocs
          // we need to make sure that the answer is nonnull. We can use the
          // existing logic that checks for nonnull for that.
        } else {
          uint64_t Size;
          if (!getObjectSize(*AI, Size, DL, TLI)) {
            // FIXME: getObjectSize does not use the dereferenceable attribute
            // to get the size when possible.
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

      if (!DL || !ETy->isSized() || Arg->hasByValOrInAllocaAttr()) {
        ArgumentState[i].IsAlign = false;
      } else if (ArgumentState[i].IsAlign &&
                 ArgumentState[i].MaxAlign > Arg->getParamAlignment()) {
        unsigned Align = getKnownAlignment(*AI, DL);
        if (Align > DL->getABITypeAlignment(ETy))
          ArgumentState[i].MaxAlign =
            std::min(ArgumentState[i].MaxAlign, (uint64_t) Align);
        else
          ArgumentState[i].IsAlign = false;
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

    if (AI->getDereferenceableBytes() < ArgumentState[i].MaxSize &&
        ArgumentState[i].IsDereferenceable) {
      AttrBuilder B;
      B.addDereferenceableAttr(ArgumentState[i].MaxSize);

      AI->addAttr(AttributeSet::get(F.getContext(), AI->getArgNo() + 1, B));
      ++NumDereferenceable;
      MadeChange = true;
    }

    if (AI->getParamAlignment() < ArgumentState[i].MaxAlign &&
        ArgumentState[i].IsAlign) {
      AttrBuilder B;
      B.addAlignmentAttr(ArgumentState[i].MaxAlign);

      AI->addAttr(AttributeSet::get(F.getContext(), AI->getArgNo() + 1, B));
      ++NumAlign;
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
  bool Changed = false;

  // We want to iterate top-down, but also want to do a fixed-point iteration
  // on SCCs to ensure a stable answer. The regular SCC iterator returns the
  // SCCs bottom-up (it uses Tarjan's Algorithm, and knows it has found the
  // root of an SCC as it passes it on the way up -- it is not clear there is a
  // better way). Because we can't collect SCCs in a purely top-down fashion
  // anyway, just store those returned by the bottom-up SCC iterator and visit
  // this stored list in reverse order.
  SmallVector<SmallVector<CallGraphNode *, 16>, 8> CGSCCs;
  for (scc_iterator<CallGraph*> CGI = scc_begin(&CG), CGIE = scc_end(&CG);
       CGI != CGIE; ++CGI) {
    CGSCCs.push_back(SmallVector<CallGraphNode *, 16>(CGI->begin(),
                                                      CGI->end()));
  }

  scc_iterator<CallGraph*> CGI = scc_begin(&CG);

  DenseMap<Function *, DominatorTree *> DTs;
  for (auto S = CGSCCs.rbegin(), SE = CGSCCs.rend(); S != SE; ++S) {
    // FIXME: We could get a better answer by speculating the attributes within
    // each SCC (meaning setting all attributes that might be set, and then
    // removing those found be to unprovable at some call sites of the SCC's
    // functions).

    bool ChangedThisSCC;
    do {
      ChangedThisSCC = false;
      for (CallGraphNode *CGN : *S) {
        Function *F = CGN->getFunction();
        if (!F)
          continue;
        DEBUG(dbgs() << "FATD visiting: " << F->getName() << "\n");
        Changed |= (ChangedThisSCC |= addFuncAttrs(*F, DTs));
      }
    } while (ChangedThisSCC);
  }

  return Changed;
}

ModulePass *llvm::createFunctionAttrsTDPass() { return new FunctionAttrsTD(); }

