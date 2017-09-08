//===-- BasicBlock.cpp - Implement BasicBlock related methods -------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the BasicBlock class for the IR library.
//
//===----------------------------------------------------------------------===//

#include "llvm/IR/BasicBlock.h"
#include "SymbolTableListTraitsImpl.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Type.h"
#include <algorithm>

using namespace llvm;

ValueSymbolTable *BasicBlock::getValueSymbolTable() {
  if (Function *F = getParent())
    return F->getValueSymbolTable();
  return nullptr;
}

LLVMContext &BasicBlock::getContext() const {
  return getType()->getContext();
}

// Explicit instantiation of SymbolTableListTraits since some of the methods
// are not in the public header file...
template class llvm::SymbolTableListTraits<Instruction>;

BasicBlock::BasicBlock(LLVMContext &C, const Twine &Name, Function *NewParent,
                       BasicBlock *InsertBefore)
  : Value(Type::getLabelTy(C), Value::BasicBlockVal), Parent(nullptr) {

  if (NewParent)
    insertInto(NewParent, InsertBefore);
  else
    assert(!InsertBefore &&
           "Cannot insert block before another block with no function!");

  setName(Name);
}

void BasicBlock::insertInto(Function *NewParent, BasicBlock *InsertBefore) {
  assert(NewParent && "Expected a parent");
  assert(!Parent && "Already has a parent");

  if (InsertBefore)
    NewParent->getBasicBlockList().insert(InsertBefore->getIterator(), this);
  else
    NewParent->getBasicBlockList().push_back(this);
}

BasicBlock::~BasicBlock() {
  // If the address of the block is taken and it is being deleted (e.g. because
  // it is dead), this means that there is either a dangling constant expr
  // hanging off the block, or an undefined use of the block (source code
  // expecting the address of a label to keep the block alive even though there
  // is no indirect branch).  Handle these cases by zapping the BlockAddress
  // nodes.  There are no other possible uses at this point.
  if (hasAddressTaken()) {
    assert(!use_empty() && "There should be at least one blockaddress!");
    Constant *Replacement =
      ConstantInt::get(llvm::Type::getInt32Ty(getContext()), 1);
    while (!use_empty()) {
      BlockAddress *BA = cast<BlockAddress>(user_back());
      BA->replaceAllUsesWith(ConstantExpr::getIntToPtr(Replacement,
                                                       BA->getType()));
      BA->destroyConstant();
    }
  }

  assert(getParent() == nullptr && "BasicBlock still linked into the program!");
  dropAllReferences();
  InstList.clear();
}

void BasicBlock::setParent(Function *parent) {
  // Set Parent=parent, updating instruction symtab entries as appropriate.
  InstList.setSymTabObject(&Parent, parent);
}

void BasicBlock::removeFromParent() {
  getParent()->getBasicBlockList().remove(getIterator());
}

iplist<BasicBlock>::iterator BasicBlock::eraseFromParent() {
  return getParent()->getBasicBlockList().erase(getIterator());
}

/// Unlink this basic block from its current function and
/// insert it into the function that MovePos lives in, right before MovePos.
void BasicBlock::moveBefore(BasicBlock *MovePos) {
  MovePos->getParent()->getBasicBlockList().splice(
      MovePos->getIterator(), getParent()->getBasicBlockList(), getIterator());
}

/// Unlink this basic block from its current function and
/// insert it into the function that MovePos lives in, right after MovePos.
void BasicBlock::moveAfter(BasicBlock *MovePos) {
  MovePos->getParent()->getBasicBlockList().splice(
      ++MovePos->getIterator(), getParent()->getBasicBlockList(),
      getIterator());
}

const Module *BasicBlock::getModule() const {
  return getParent()->getParent();
}

const TerminatorInst *BasicBlock::getTerminator() const {
  if (InstList.empty()) return nullptr;
  return dyn_cast<TerminatorInst>(&InstList.back());
}

const CallInst *BasicBlock::getTerminatingMustTailCall() const {
  if (InstList.empty())
    return nullptr;
  const ReturnInst *RI = dyn_cast<ReturnInst>(&InstList.back());
  if (!RI || RI == &InstList.front())
    return nullptr;

  const Instruction *Prev = RI->getPrevNode();
  if (!Prev)
    return nullptr;

  if (Value *RV = RI->getReturnValue()) {
    if (RV != Prev)
      return nullptr;

    // Look through the optional bitcast.
    if (auto *BI = dyn_cast<BitCastInst>(Prev)) {
      RV = BI->getOperand(0);
      Prev = BI->getPrevNode();
      if (!Prev || RV != Prev)
        return nullptr;
    }
  }

  if (auto *CI = dyn_cast<CallInst>(Prev)) {
    if (CI->isMustTailCall())
      return CI;
  }
  return nullptr;
}

const CallInst *BasicBlock::getTerminatingDeoptimizeCall() const {
  if (InstList.empty())
    return nullptr;
  auto *RI = dyn_cast<ReturnInst>(&InstList.back());
  if (!RI || RI == &InstList.front())
    return nullptr;

  if (auto *CI = dyn_cast_or_null<CallInst>(RI->getPrevNode()))
    if (Function *F = CI->getCalledFunction())
      if (F->getIntrinsicID() == Intrinsic::experimental_deoptimize)
        return CI;

  return nullptr;
}

const Instruction* BasicBlock::getFirstNonPHI() const {
  for (const Instruction &I : *this)
    if (!isa<PHINode>(I))
      return &I;
  return nullptr;
}

const Instruction* BasicBlock::getFirstNonPHIOrDbg() const {
  for (const Instruction &I : *this)
    if (!isa<PHINode>(I) && !isa<DbgInfoIntrinsic>(I))
      return &I;
  return nullptr;
}

const Instruction* BasicBlock::getFirstNonPHIOrDbgOrLifetime() const {
  for (const Instruction &I : *this) {
    if (isa<PHINode>(I) || isa<DbgInfoIntrinsic>(I))
      continue;

    if (auto *II = dyn_cast<IntrinsicInst>(&I))
      if (II->getIntrinsicID() == Intrinsic::lifetime_start ||
          II->getIntrinsicID() == Intrinsic::lifetime_end)
        continue;

    return &I;
  }
  return nullptr;
}

BasicBlock::const_iterator BasicBlock::getFirstInsertionPt() const {
  const Instruction *FirstNonPHI = getFirstNonPHI();
  if (!FirstNonPHI)
    return end();

  const_iterator InsertPt = FirstNonPHI->getIterator();
  if (InsertPt->isEHPad()) ++InsertPt;
  return InsertPt;
}

void BasicBlock::dropAllReferences() {
  for (Instruction &I : *this)
    I.dropAllReferences();
}

/// If this basic block has a single predecessor block,
/// return the block, otherwise return a null pointer.
const BasicBlock *BasicBlock::getSinglePredecessor() const {
  const_pred_iterator PI = pred_begin(this), E = pred_end(this);
  if (PI == E) return nullptr;         // No preds.
  const BasicBlock *ThePred = *PI;
  ++PI;
  return (PI == E) ? ThePred : nullptr /*multiple preds*/;
}

/// If this basic block has a unique predecessor block,
/// return the block, otherwise return a null pointer.
/// Note that unique predecessor doesn't mean single edge, there can be
/// multiple edges from the unique predecessor to this block (for example
/// a switch statement with multiple cases having the same destination).
const BasicBlock *BasicBlock::getUniquePredecessor() const {
  const_pred_iterator PI = pred_begin(this), E = pred_end(this);
  if (PI == E) return nullptr; // No preds.
  const BasicBlock *PredBB = *PI;
  ++PI;
  for (;PI != E; ++PI) {
    if (*PI != PredBB)
      return nullptr;
    // The same predecessor appears multiple times in the predecessor list.
    // This is OK.
  }
  return PredBB;
}

const BasicBlock *BasicBlock::getSingleSuccessor() const {
  succ_const_iterator SI = succ_begin(this), E = succ_end(this);
  if (SI == E) return nullptr; // no successors
  const BasicBlock *TheSucc = *SI;
  ++SI;
  return (SI == E) ? TheSucc : nullptr /* multiple successors */;
}

const BasicBlock *BasicBlock::getUniqueSuccessor() const {
  succ_const_iterator SI = succ_begin(this), E = succ_end(this);
  if (SI == E) return nullptr; // No successors
  const BasicBlock *SuccBB = *SI;
  ++SI;
  for (;SI != E; ++SI) {
    if (*SI != SuccBB)
      return nullptr;
    // The same successor appears multiple times in the successor list.
    // This is OK.
  }
  return SuccBB;
}

iterator_range<BasicBlock::phi_iterator> BasicBlock::phis() {
  return make_range<phi_iterator>(dyn_cast<PHINode>(&front()), nullptr);
}

void BasicBlock::removeEdge(BasicBlock *To, DominatorTree *DT,
                            bool removeInvoke) {
  // Terminator instruction types: BranchInst(1/2),
  // SwitchInst(getNumOperands()/2), IndirectBrInst(getNumOperands()-1),
  // InvokeInst(2), CleanupReturnInst(1/0), CatchReturnInst(1),
  // CatchSwitchInst(getNumOperands()-1)
  //
  // ResumeInst(0), UnreachableInst(0) are not checked because they never
  // have successors.
  assert(To && "To cannot be nullptr.");
  assert(find(succ_begin(this), succ_end(this), To) != succ_end(this) &&
         "There exists no edge between this and To.");

  SmallVector<DominatorTree::UpdateType, 2> Updates;
  TerminatorInst *TI = getTerminator();

  if (CleanupReturnInst *CI = dyn_cast<CleanupReturnInst>(TI)) {
    // To is the only successor.
    new UnreachableInst(getContext(), this);
    CI->eraseFromParent();
  } else if (CatchReturnInst *CI = dyn_cast<CatchReturnInst>(TI)) {
    // To is the only successor.
    new UnreachableInst(getContext(), this);
    CI->eraseFromParent();
  } else if (InvokeInst *II = dyn_cast<InvokeInst>(TI)) {
    assert(removeInvoke && "Invokes must have two edges removed at once.");
    if (DT) {
      BasicBlock *NormalDestBB = II->getNormalDest();
      BasicBlock *UnwindDestBB = II->getUnwindDest();
      if (this != NormalDestBB)
        Updates.push_back({DominatorTree::Delete, this, NormalDestBB});
      if (this != UnwindDestBB && NormalDestBB != UnwindDestBB)
        Updates.push_back({DominatorTree::Delete, this, UnwindDestBB});
    }
    new UnreachableInst(getContext(), this);
    II->eraseFromParent();
  } else if (BranchInst *BI = dyn_cast<BranchInst>(TI)) {
    if (BI->isConditional()) {
      BasicBlock *L = BI->getSuccessor(0);
      BasicBlock *R = BI->getSuccessor(1);
      if (To == L && To == R) {
        new UnreachableInst(getContext(), this);
      } else if (To == L) {
        BranchInst::Create(R, BI);
      } else {
        BranchInst::Create(L, BI);
      }
    } else {
      new UnreachableInst(getContext(), this);
    }
    BI->eraseFromParent();
  } else if (IndirectBrInst *II = dyn_cast<IndirectBrInst>(TI)) {
    // There are 1+ instances of To in the list of destinations.
    bool RemovedDest;
    do {
      unsigned NumDests = II->getNumDestinations();
      if (NumDests == 1) {
        if (II->getDestination(0) == To) {
          // The only remaining destination is To.
          new UnreachableInst(getContext(), this);
          II->eraseFromParent();
        }
        break;
      } else {
        RemovedDest = false;
        for (unsigned I = 0; I < NumDests; ++I)
          if (II->getDestination(I) == To) {
            II->removeDestination(I);
            RemovedDest = true;
            break;
          }
      }
    } while (RemovedDest);
  } else if (SwitchInst *SI = dyn_cast<SwitchInst>(TI)) {
    // To is a successor of one or more case statements (including the
    // default case). The first task is to remove all non-default case
    // statements that branch to To.
    bool RemovedCase;
    do {
      RemovedCase = false;
      for (auto I = SI->case_begin(), E = SI->case_end(); I != E; ++I) {
        if (I == SI->case_default())
          continue;
        if (I->getCaseSuccessor() == To) {
          SI->removeCase(I);
          RemovedCase = true;
          break;
        }
      }
    } while (RemovedCase);
    // Now check the default case.
    if (SI->getSuccessor(0) == To) {
      if (SI->getNumSuccessors() == 1) {
        // The only remaining successor is the default case.
        new UnreachableInst(getContext(), this);
        SI->eraseFromParent();
      } else {
        // The SwitchInst has non-default cases remaining. Create a new
        // BasicBlock, set it as the new default destination, and denote
        // the new BasicBlock as unreachable.
        BasicBlock *UnreachBB = BasicBlock::Create(
            getContext(), Twine(getName()) + ".unreachable_switch_default",
            getParent());
        SI->setDefaultDest(UnreachBB);
        new UnreachableInst(UnreachBB->getContext(), UnreachBB);
        if (DT)
          Updates.push_back({DominatorTree::Insert, this, UnreachBB});
      }
    }
    if (DT && this != To)
      Updates.push_back({DominatorTree::Delete, this, To});
  } else if (CatchSwitchInst *CI = dyn_cast<CatchSwitchInst>(TI)) {
    // To is a successor of one or more handlers and may also be the unwind
    // destination. The first task is to remove all the handlers that branch
    // to To.
    bool RemovedHandler;
    do {
      RemovedHandler = false;
      for (auto I = CI->handler_begin(), E = CI->handler_end(); I != E; ++I)
        if (*I == To) {
          CI->removeHandler(I);
          RemovedHandler = true;
          break;
        }
    } while (RemovedHandler);
    // Now check the unwind destination.
    if (CI->getUnwindDest() == To) {
      if (CI->getNumHandlers() == 0) {
        // The only remaining successor is the unwind destination.
        new UnreachableInst(getContext(), this);
        CI->eraseFromParent();
      } else {
        // The CatchSwitchInst has one or more handlers. Create a new
        // BasicBlock, set it as the new unwind destination, and denote
        // the new BasicBlock as unreachable.
        BasicBlock *UnreachBB = BasicBlock::Create(
            getContext(), Twine(getName()) + ".unreachable_unwind_default",
            getParent());
        CI->setUnwindDest(UnreachBB);
        new UnreachableInst(UnreachBB->getContext(), UnreachBB);
        if (DT)
          Updates.push_back({DominatorTree::Insert, this, UnreachBB});
      }
    }
    if (DT && this != To)
      Updates.push_back({DominatorTree::Delete, this, To});
  } else {
    llvm_unreachable("Unexpected TerminatorInst");
  }
  // Remove our edge from the dominator tree. If we used Updates above just
  // apply them. Otherwise remove the edge between this and To provided it's not
  // a loop back to itself.
  if (DT) {
    if (!Updates.empty())
      DT->applyUpdates(Updates);
    else if (this != To)
      DT->deleteEdge(this, To);
  }
}

/// This method is used to notify a BasicBlock that the
/// specified Predecessor of the block is no longer able to reach it.  This is
/// actually not used to update the Predecessor list, but is actually used to
/// update the PHI nodes that reside in the block.  Note that this should be
/// called while the predecessor still refers to this block.
///
void BasicBlock::removePredecessor(BasicBlock *Pred,
                                   bool DontDeleteUselessPHIs) {
  assert((hasNUsesOrMore(16)||// Reduce cost of this assertion for complex CFGs.
          find(pred_begin(this), pred_end(this), Pred) != pred_end(this)) &&
         "removePredecessor: BB is not a predecessor!");

  if (InstList.empty()) return;
  PHINode *APN = dyn_cast<PHINode>(&front());
  if (!APN) return;   // Quick exit.

  // If there are exactly two predecessors, then we want to nuke the PHI nodes
  // altogether.  However, we cannot do this, if this in this case:
  //
  //  Loop:
  //    %x = phi [X, Loop]
  //    %x2 = add %x, 1         ;; This would become %x2 = add %x2, 1
  //    br Loop                 ;; %x2 does not dominate all uses
  //
  // This is because the PHI node input is actually taken from the predecessor
  // basic block.  The only case this can happen is with a self loop, so we
  // check for this case explicitly now.
  //
  unsigned max_idx = APN->getNumIncomingValues();
  assert(max_idx != 0 && "PHI Node in block with 0 predecessors!?!?!");
  if (max_idx == 2) {
    BasicBlock *Other = APN->getIncomingBlock(APN->getIncomingBlock(0) == Pred);

    // Disable PHI elimination!
    if (this == Other) max_idx = 3;
  }

  // <= Two predecessors BEFORE I remove one?
  if (max_idx <= 2 && !DontDeleteUselessPHIs) {
    // Yup, loop through and nuke the PHI nodes
    while (PHINode *PN = dyn_cast<PHINode>(&front())) {
      // Remove the predecessor first.
      PN->removeIncomingValue(Pred, !DontDeleteUselessPHIs);

      // If the PHI _HAD_ two uses, replace PHI node with its now *single* value
      if (max_idx == 2) {
        if (PN->getIncomingValue(0) != PN)
          PN->replaceAllUsesWith(PN->getIncomingValue(0));
        else
          // We are left with an infinite loop with no entries: kill the PHI.
          PN->replaceAllUsesWith(UndefValue::get(PN->getType()));
        getInstList().pop_front();    // Remove the PHI node
      }

      // If the PHI node already only had one entry, it got deleted by
      // removeIncomingValue.
    }
  } else {
    // Okay, now we know that we need to remove predecessor #pred_idx from all
    // PHI nodes.  Iterate over each PHI node fixing them up
    PHINode *PN;
    for (iterator II = begin(); (PN = dyn_cast<PHINode>(II)); ) {
      ++II;
      PN->removeIncomingValue(Pred, false);
      // If all incoming values to the Phi are the same, we can replace the Phi
      // with that value.
      Value* PNV = nullptr;
      if (!DontDeleteUselessPHIs && (PNV = PN->hasConstantValue()))
        if (PNV != PN) {
          PN->replaceAllUsesWith(PNV);
          PN->eraseFromParent();
        }
    }
  }
}

bool BasicBlock::canSplitPredecessors() const {
  const Instruction *FirstNonPHI = getFirstNonPHI();
  if (isa<LandingPadInst>(FirstNonPHI))
    return true;
  // This is perhaps a little conservative because constructs like
  // CleanupBlockInst are pretty easy to split.  However, SplitBlockPredecessors
  // cannot handle such things just yet.
  if (FirstNonPHI->isEHPad())
    return false;
  return true;
}

bool BasicBlock::isLegalToHoistInto() const {
  auto *Term = getTerminator();
  // No terminator means the block is under construction.
  if (!Term)
    return true;

  // If the block has no successors, there can be no instructions to hoist.
  assert(Term->getNumSuccessors() > 0);

  // Instructions should not be hoisted across exception handling boundaries.
  return !Term->isExceptional();
}

/// This splits a basic block into two at the specified
/// instruction.  Note that all instructions BEFORE the specified iterator stay
/// as part of the original basic block, an unconditional branch is added to
/// the new BB, and the rest of the instructions in the BB are moved to the new
/// BB, including the old terminator.  This invalidates the iterator.
///
/// Note that this only works on well formed basic blocks (must have a
/// terminator), and 'I' must not be the end of instruction list (which would
/// cause a degenerate basic block to be formed, having a terminator inside of
/// the basic block).
///
BasicBlock *BasicBlock::splitBasicBlock(iterator I, const Twine &BBName) {
  assert(getTerminator() && "Can't use splitBasicBlock on degenerate BB!");
  assert(I != InstList.end() &&
         "Trying to get me to create degenerate basic block!");

  BasicBlock *New = BasicBlock::Create(getContext(), BBName, getParent(),
                                       this->getNextNode());

  // Save DebugLoc of split point before invalidating iterator.
  DebugLoc Loc = I->getDebugLoc();
  // Move all of the specified instructions from the original basic block into
  // the new basic block.
  New->getInstList().splice(New->end(), this->getInstList(), I, end());

  // Add a branch instruction to the newly formed basic block.
  BranchInst *BI = BranchInst::Create(New, this);
  BI->setDebugLoc(Loc);

  // Now we must loop through all of the successors of the New block (which
  // _were_ the successors of the 'this' block), and update any PHI nodes in
  // successors.  If there were PHI nodes in the successors, then they need to
  // know that incoming branches will be from New, not from Old.
  //
  for (succ_iterator I = succ_begin(New), E = succ_end(New); I != E; ++I) {
    // Loop over any phi nodes in the basic block, updating the BB field of
    // incoming values...
    BasicBlock *Successor = *I;
    for (auto &PN : Successor->phis()) {
      int Idx = PN.getBasicBlockIndex(this);
      while (Idx != -1) {
        PN.setIncomingBlock((unsigned)Idx, New);
        Idx = PN.getBasicBlockIndex(this);
      }
    }
  }
  return New;
}

void BasicBlock::replaceSuccessorsPhiUsesWith(BasicBlock *New) {
  TerminatorInst *TI = getTerminator();
  if (!TI)
    // Cope with being called on a BasicBlock that doesn't have a terminator
    // yet. Clang's CodeGenFunction::EmitReturnBlock() likes to do this.
    return;
  for (BasicBlock *Succ : TI->successors()) {
    // N.B. Succ might not be a complete BasicBlock, so don't assume
    // that it ends with a non-phi instruction.
    for (iterator II = Succ->begin(), IE = Succ->end(); II != IE; ++II) {
      PHINode *PN = dyn_cast<PHINode>(II);
      if (!PN)
        break;
      int i;
      while ((i = PN->getBasicBlockIndex(this)) >= 0)
        PN->setIncomingBlock(i, New);
    }
  }
}

/// Return true if this basic block is a landing pad. I.e., it's
/// the destination of the 'unwind' edge of an invoke instruction.
bool BasicBlock::isLandingPad() const {
  return isa<LandingPadInst>(getFirstNonPHI());
}

/// Return the landingpad instruction associated with the landing pad.
const LandingPadInst *BasicBlock::getLandingPadInst() const {
  return dyn_cast<LandingPadInst>(getFirstNonPHI());
}
