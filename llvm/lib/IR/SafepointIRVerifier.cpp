//===-- SafepointIRVerifier.cpp - Verify gc.statepoint invariants ---------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Run a sanity check on the IR to ensure that Safepoints - if they've been
// inserted - were inserted correctly.  In particular, look for use of
// non-relocated values after a safepoint.  It's primary use is to check the
// correctness of safepoint insertion immediately after insertion, but it can
// also be used to verify that later transforms have not found a way to break
// safepoint semenatics.
//
// In its current form, this verify checks a property which is sufficient, but
// not neccessary for correctness.  There are some cases where an unrelocated
// pointer can be used after the safepoint.  Consider this example:
//
//    a = ...
//    b = ...
//    (a',b') = safepoint(a,b)
//    c = cmp eq a b
//    br c, ..., ....
//
// Because it is valid to reorder 'c' above the safepoint, this is legal.  In
// practice, this is a somewhat uncommon transform, but CodeGenPrep does create
// idioms like this.  Today, the verifier would report a spurious failure on
// this case.
//
//===----------------------------------------------------------------------===//

#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/SetOperations.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/SafepointIRVerifier.h"
#include "llvm/IR/Statepoint.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"

#define DEBUG_TYPE "safepoint-ir-verifier"

using namespace llvm;

/// This option is used for writing test cases.  Instead of crashing the program
/// when verification fails, report a message to the console (for FileCheck
/// usage) and continue execution as if nothing happened.
static cl::opt<bool> PrintOnly("safepoint-ir-verifier-print-only",
                               cl::init(false));

static void Verify(const Function &F, const DominatorTree &DT);

struct SafepointIRVerifier : public FunctionPass {
  static char ID; // Pass identification, replacement for typeid
  DominatorTree DT;
  SafepointIRVerifier() : FunctionPass(ID) {
    initializeSafepointIRVerifierPass(*PassRegistry::getPassRegistry());
  }

  bool runOnFunction(Function &F) override {
    DT.recalculate(F);
    Verify(F, DT);
    return false; // no modifications
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.setPreservesAll();
  }
};

void llvm::verifySafepointIR(Function &F) {
  SafepointIRVerifier pass;
  pass.runOnFunction(F);
}

char SafepointIRVerifier::ID = 0;

FunctionPass *llvm::createSafepointIRVerifierPass() {
  return new SafepointIRVerifier();
}

INITIALIZE_PASS_BEGIN(SafepointIRVerifier, "verify-safepoint-ir",
                      "Safepoint IR Verifier", false, true)
INITIALIZE_PASS_END(SafepointIRVerifier, "verify-safepoint-ir",
                    "Safepoint IR Verifier", false, true)

static bool isGCPointerType(Type *T) {
  if (auto *PT = dyn_cast<PointerType>(T))
    // For the sake of this example GC, we arbitrarily pick addrspace(1) as our
    // GC managed heap.  We know that a pointer into this heap needs to be
    // updated and that no other pointer does.
    return (1 == PT->getAddressSpace());
  return false;
}

static bool containsGCPtrType(Type *Ty) {
  if (isGCPointerType(Ty))
    return true;
  if (VectorType *VT = dyn_cast<VectorType>(Ty))
    return isGCPointerType(VT->getScalarType());
  if (ArrayType *AT = dyn_cast<ArrayType>(Ty))
    return containsGCPtrType(AT->getElementType());
  if (StructType *ST = dyn_cast<StructType>(Ty))
    return std::any_of(ST->subtypes().begin(), ST->subtypes().end(),
                       containsGCPtrType);
  return false;
}

// Debugging aid -- prints a [Begin, End) range of values.
template<typename IteratorTy>
static void PrintValueSet(raw_ostream &OS, IteratorTy Begin, IteratorTy End) {
  OS << "[ ";
  while (Begin != End) {
    OS << **Begin << " ";
    ++Begin;
  }
  OS << "]";
}

/// The verifier algorithm is phrased in terms of availability.  The set of
/// values "available" at a given point in the control flow graph is the set of
/// correctly relocated value at that point, and is a subset of the set of
/// definitions dominating that point.

/// State we compute and track per basic block.
struct BasicBlockState {
  // Set of values available coming in, before the phi nodes
  DenseSet<const Value *> AvailableIn;

  // Set of values available going out
  DenseSet<const Value *> AvailableOut;

  // AvailableOut minus AvailableIn.
  // All elements are Instructions
  DenseSet<const Value *> Contribution;

  // True if this block contains a safepoint and thus AvailableIn does not
  // contribute to AvailableOut.
  bool Cleared = false;
};


/// Gather all the definitions dominating the start of BB into Result.  This is
/// simply the Defs introduced by every dominating basic block and the function
/// arguments.
static void GatherDominatingDefs(const BasicBlock *BB,
                                 DenseSet<const Value *> &Result,
                                 const DominatorTree &DT,
                    DenseMap<const BasicBlock *, BasicBlockState *> &BlockMap) {
  DomTreeNode *DTN = DT[const_cast<BasicBlock *>(BB)];

  while (DTN->getIDom()) {
    DTN = DTN->getIDom();
    const auto &Defs = BlockMap[DTN->getBlock()]->Contribution;
    Result.insert(Defs.begin(), Defs.end());
    // If this block is 'Cleared', then nothing LiveIn to this block can be
    // available after this block completes.  Note: This turns out to be 
    // really important for reducing memory consuption of the initial available
    // sets and thus peak memory usage by this verifier.
    if (BlockMap[DTN->getBlock()]->Cleared)
      return;
  }

  for (const Argument &A : BB->getParent()->args())
    if (containsGCPtrType(A.getType()))
      Result.insert(&A);
}

/// Model the effect of an instruction on the set of available values.
static void TransferInstruction(const Instruction &I, bool &Cleared,
                              DenseSet<const Value *> &Available) {
  if (isStatepoint(I)) {
    Cleared = true;
    Available.clear();
  } else if (containsGCPtrType(I.getType()))
    Available.insert(&I);
}

/// TransferInstruction composed over a whole BasicBlock.
static void TransferBlock(const BasicBlock *BB,
                          BasicBlockState &BBS, bool FirstPass) {

  const DenseSet<const Value *> &AvailableIn = BBS.AvailableIn; 
  DenseSet<const Value *> &AvailableOut  = BBS.AvailableOut;

  if (BBS.Cleared) {
    // AvailableOut does not change no matter how the input changes, just
    // leave it be.  We need to force this calculation the first time so that
    // we have a AvailableOut at all.
    if (FirstPass) {
      AvailableOut = BBS.Contribution;
    }
  } else {
    // Otherwise, we need to reduce the AvailableOut set by things which are no
    // longer in our AvailableIn
    DenseSet<const Value *> Temp = BBS.Contribution;
    set_union(Temp, AvailableIn);
    AvailableOut = std::move(Temp);
  }

  DEBUG(dbgs() << "Transfered block " << BB->getName() << " from ";
        PrintValueSet(dbgs(), AvailableIn.begin(), AvailableIn.end());
        dbgs() << " to ";
        PrintValueSet(dbgs(), AvailableOut.begin(), AvailableOut.end());
        dbgs() << "\n";);
}

static void Verify(const Function &F, const DominatorTree &DT) {
  SpecificBumpPtrAllocator<BasicBlockState> BSAllocator;
  DenseMap<const BasicBlock *, BasicBlockState *> BlockMap;

  if (PrintOnly)
    dbgs() << "Verifying gc pointers in function: " << F.getName() << "\n";


  for (const BasicBlock &BB : F) {
    BasicBlockState *BBS = new(BSAllocator.Allocate()) BasicBlockState;
    for (const auto &I : BB)
      TransferInstruction(I, BBS->Cleared, BBS->Contribution);
    BlockMap[&BB] = BBS;
  }

  for (auto &BBI : BlockMap) {
    GatherDominatingDefs(BBI.first, BBI.second->AvailableIn, DT, BlockMap);
    TransferBlock(BBI.first, *BBI.second, true);
  }

  SetVector<const BasicBlock *> Worklist;
  for (auto &BBI : BlockMap)
    Worklist.insert(BBI.first);

  // This loop iterates the AvailableIn and AvailableOut sets to a fixed point.
  // The AvailableIn and AvailableOut sets decrease as we iterate.
  while (!Worklist.empty()) {
    const BasicBlock *BB = Worklist.pop_back_val();
    BasicBlockState *BBS = BlockMap[BB];

    size_t OldInCount = BBS->AvailableIn.size();
    for (const BasicBlock *PBB : predecessors(BB))
      set_intersect(BBS->AvailableIn, BlockMap[PBB]->AvailableOut);

    if (OldInCount == BBS->AvailableIn.size())
      continue;

    assert(OldInCount > BBS->AvailableIn.size() && "invariant!");

    size_t OldOutCount = BBS->AvailableOut.size();
    TransferBlock(BB, *BBS, false);
    if (OldOutCount != BBS->AvailableOut.size()) {
      assert(OldOutCount > BBS->AvailableOut.size() && "invariant!");
      Worklist.insert(succ_begin(BB), succ_end(BB));
    }
  }

  // We now have all the information we need to decide if the use of a heap
  // reference is legal or not, given our safepoint semantics.

  bool AnyInvalidUses = false;

  auto ReportInvalidUse = [&AnyInvalidUses](const Value &V,
                                            const Instruction &I) {
    errs() << "Illegal use of unrelocated value found!\n";
    errs() << "Def: " << V << "\n";
    errs() << "Use: " << I << "\n";
    if (!PrintOnly)
      abort();
    AnyInvalidUses = true;
  };

  for (const BasicBlock &BB : F) {
    // We destructively modify AvailableIn as we traverse the block instruction
    // by instruction.
    DenseSet<const Value *> &AvailableSet = BlockMap[&BB]->AvailableIn;
    for (const Instruction &I : BB) {
      if (const PHINode *PN = dyn_cast<PHINode>(&I)) {
        if (containsGCPtrType(PN->getType()))
          for (unsigned i = 0, e = PN->getNumIncomingValues(); i != e; ++i) {
            const BasicBlock *InBB = PN->getIncomingBlock(i);
            const Value *InValue = PN->getIncomingValue(i);

            if (!isa<Constant>(InValue) &&
                !BlockMap[InBB]->AvailableOut.count(InValue))
              ReportInvalidUse(*InValue, *PN);
          }
      } else {
        for (const Value *V : I.operands())
          if (containsGCPtrType(V->getType()) && !isa<Constant>(V) &&
              !AvailableSet.count(V))
            ReportInvalidUse(*V, I);
      }

      bool Cleared = false;
      TransferInstruction(I, Cleared, AvailableSet);
      (void)Cleared;
    }
  }

  if (PrintOnly && !AnyInvalidUses) {
    dbgs() << "No illegal uses found by SafepointIRVerifier in: " << F.getName()
           << "\n";
  }
}
