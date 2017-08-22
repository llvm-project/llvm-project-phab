//===--- Utils.cpp - Utility functions for the code generation --*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains utility functions for the code generation.
//
//===----------------------------------------------------------------------===//

#include "polly/CodeGen/Utils.h"
#include "polly/CodeGen/IRBuilder.h"
#include "polly/ScopInfo.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/RegionInfo.h"
#include "llvm/Support/Debug.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

using namespace llvm;

// Alternative to llvm::SplitCriticalEdge.
//
// Creates a new block which branches to Succ. The edge to split is redirected
// to the new block.
//
// The issue with llvm::SplitCriticalEdge is that it does nothing if the edge is
// not critical.
// The issue with llvm::SplitEdge is that it does not always create the middle
// block, but reuses Prev/Succ if it can. We always want a new middle block.
static BasicBlock *splitEdge(BasicBlock *Prev, BasicBlock *Succ,
                             const char *Suffix, DominatorTree *DT,
                             LoopInfo *LI, RegionInfo *RI) {
  assert(Prev && Succ);

  // Before:
  //   \    /     /   //
  //    Prev     /    //
  //     |  \___/     //
  //     |   ___      //
  //     |  /   \     //
  //    Succ     \    //
  //   /    \     \   //

  // The algorithm to update DominatorTree and LoopInfo of
  // llvm::SplitCriticalEdge is more efficient than
  // llvm::SplitBlockPredecessors, which is more general. In the future we might
  // either modify llvm::SplitCriticalEdge to allow skipping the critical edge
  // check; or Copy&Pase it here.
  BasicBlock *MiddleBlock = SplitBlockPredecessors(
      Succ, ArrayRef<BasicBlock *>(Prev), Suffix, DT, LI);

  if (RI) {
    Region *PrevRegion = RI->getRegionFor(Prev);
    Region *SuccRegion = RI->getRegionFor(Succ);
    if (PrevRegion->contains(MiddleBlock)) {
      RI->setRegionFor(MiddleBlock, PrevRegion);
    } else {
      RI->setRegionFor(MiddleBlock, SuccRegion);
    }
  }

  // After:
  //   \    /     /   //
  //    Prev     /    //
  //     |  \___/     //
  //     |            //
  // MiddleBlock      //
  //     |   ___      //
  //     |  /   \     //
  //    Succ     \    //
  //   /    \     \   //

  return MiddleBlock;
}

std::pair<polly::BBPair, BranchInst *>
polly::executeScopConditionally(Scop &S, Value *RTC, DominatorTree &DT,
                                RegionInfo &RI, LoopInfo &LI) {
  Region &R = S.getRegion();
  PollyIRBuilder Builder(S.getEntry());

  // Before:
  //
  //      \   /      //
  //    EnteringBB   //
  //   _____|_____   //
  //  /  EntryBB  \  //
  //  |  (region) |  //
  //  \_ExitingBB_/  //
  //        |        //
  //      ExitBB     //
  //      /    \     //

  // Create a fork block.
  BasicBlock *EnteringBB = S.getEnteringBlock();
  BasicBlock *EntryBB = S.getEntry();
  assert(EnteringBB && "Must be a simple region");
  BasicBlock *SplitBlock =
      splitEdge(EnteringBB, EntryBB, ".split_new_and_old", &DT, &LI, &RI);
  SplitBlock->setName("polly.split_new_and_old");

  // If EntryBB is the exit block of the region that includes Prev, exclude
  // SplitBlock from that region by making it itself the exit block. This is
  // trivially possible because there is just one edge to EnteringBB.
  // This is necessary because we will add an outgoing edge from SplitBlock,
  // which would violate the single exit block requirement of PrevRegion.
  Region *PrevRegion = RI.getRegionFor(EnteringBB);
  while (PrevRegion->getExit() == EntryBB) {
    PrevRegion->replaceExit(SplitBlock);
    PrevRegion = PrevRegion->getParent();
  }
  RI.setRegionFor(SplitBlock, PrevRegion);

  // Create a join block
  BasicBlock *ExitingBB = S.getExitingBlock();
  BasicBlock *ExitBB = S.getExit();
  assert(ExitingBB && "Must be a simple region");
  BasicBlock *MergeBlock =
      splitEdge(ExitingBB, ExitBB, ".merge_new_and_old", &DT, &LI, &RI);
  MergeBlock->setName("polly.merge_new_and_old");

  // Exclude the join block from the region.
  R.replaceExitRecursive(MergeBlock);
  RI.setRegionFor(MergeBlock, R.getParent());

  //      \   /      //
  //    EnteringBB   //
  //        |        //
  //    SplitBlock   //
  //   _____|_____   //
  //  /  EntryBB  \  //
  //  |  (region) |  //
  //  \_ExitingBB_/  //
  //        |        //
  //    MergeBlock   //
  //        |        //
  //      ExitBB     //
  //      /    \     //

  // Create the start and exiting block.
  Function *F = SplitBlock->getParent();
  BasicBlock *StartBlock =
      BasicBlock::Create(F->getContext(), "polly.start", F);
  BasicBlock *ExitingBlock =
      BasicBlock::Create(F->getContext(), "polly.exiting", F);
  SplitBlock->getTerminator()->eraseFromParent();
  Builder.SetInsertPoint(SplitBlock);
  BranchInst *CondBr = Builder.CreateCondBr(RTC, StartBlock, S.getEntry());

  if (Loop *L = LI.getLoopFor(SplitBlock)) {
    L->addBasicBlockToLoop(StartBlock, LI);
    L->addBasicBlockToLoop(ExitingBlock, LI);
  }
  DT.addNewBlock(StartBlock, SplitBlock);
  DT.addNewBlock(ExitingBlock, StartBlock);
  RI.setRegionFor(StartBlock, RI.getRegionFor(SplitBlock));
  RI.setRegionFor(ExitingBlock, RI.getRegionFor(SplitBlock));

  //      \   /                    //
  //    EnteringBB                 //
  //        |                      //
  //    SplitBlock---------\       //
  //   _____|_____         |       //
  //  /  EntryBB  \    StartBlock  //
  //  |  (region) |        |       //
  //  \_ExitingBB_/   ExitingBlock //
  //        |                      //
  //    MergeBlock                 //
  //        |                      //
  //      ExitBB                   //
  //      /    \                   //

  // Connect start block to exiting block.
  Builder.SetInsertPoint(StartBlock);
  Builder.CreateBr(ExitingBlock);
  DT.changeImmediateDominator(ExitingBlock, StartBlock);

  // Connect exiting block to join block.
  Builder.SetInsertPoint(ExitingBlock);
  Builder.CreateBr(MergeBlock);
  DT.changeImmediateDominator(MergeBlock, SplitBlock);

  //      \   /                    //
  //    EnteringBB                 //
  //        |                      //
  //    SplitBlock---------\       //
  //   _____|_____         |       //
  //  /  EntryBB  \    StartBlock  //
  //  |  (region) |        |       //
  //  \_ExitingBB_/   ExitingBlock //
  //        |              |       //
  //    MergeBlock---------/       //
  //        |                      //
  //      ExitBB                   //
  //      /    \                   //
  //

  // Split the edge between SplitBlock and EntryBB, to avoid a critical edge.
  splitEdge(SplitBlock, EntryBB, ".pre_entry_bb", &DT, &LI, &RI);

  //      \   /                    //
  //    EnteringBB                 //
  //        |                      //
  //    SplitBlock---------\       //
  //        |              |       //
  //    PreEntryBB         |       //
  //   _____|_____         |       //
  //  /  EntryBB  \    StartBlock  //
  //  |  (region) |        |       //
  //  \_ExitingBB_/   ExitingBlock //
  //        |              |       //
  //    MergeBlock---------/       //
  //        |                      //
  //      ExitBB                   //
  //      /    \                   //

  return std::make_pair(std::make_pair(StartBlock, ExitingBlock), CondBr);
}

// Transforms F to have a single exit node by introducing a new basic block
// and unconditional jumps from the existing nodes with return instructions.
// The return value is obtained with a corresponding phi instruction.
static BasicBlock *unifyFunctionExitNodes(Function &F) {
  // Loop over all of the blocks in a function, tracking all of the blocks that
  // return.
  //
  SmallVector<BasicBlock *, 4> ReturningBlocks;
  BasicBlock *ReturnBlock = nullptr;
  for (BasicBlock &I : F)
    if (isa<ReturnInst>(I.getTerminator()))
      ReturningBlocks.push_back(&I);

  // Now handle return blocks.
  if (ReturningBlocks.empty()) {
    llvm_unreachable("Full function Scop does not return");
  } else if (ReturningBlocks.size() == 1) {
    ReturnBlock = ReturningBlocks.front(); // Already has a single return block
    return ReturnBlock;
  }

  // Otherwise, we need to insert a new basic block into the function, add a PHI
  // nodes (if the function returns values), and convert all of the return
  // instructions into unconditional branches.
  //
  BasicBlock *NewRetBlock = BasicBlock::Create(F.getContext(), "polly.ret", &F);

  PHINode *PN = nullptr;
  if (F.getReturnType()->isVoidTy()) {
    ReturnInst::Create(F.getContext(), nullptr, NewRetBlock);
  } else {
    // If the function doesn't return void... add a PHI node to the block...
    PN = PHINode::Create(F.getReturnType(), ReturningBlocks.size(),
                         "UnifiedRetVal");

    NewRetBlock->getInstList().push_back(PN);
    ReturnInst::Create(F.getContext(), PN, NewRetBlock);
  }

  // Loop over all of the blocks, replacing the return instruction with an
  // unconditional branch.
  for (BasicBlock *BB : ReturningBlocks) {
    // Add an incoming element to the PHI node for every return instruction that
    // is merging into this new block...
    if (PN)
      PN->addIncoming(BB->getTerminator()->getOperand(0), BB);

    BB->getTerminator()->eraseFromParent();
    BranchInst::Create(NewRetBlock, BB);
  }
  return NewRetBlock;
}

std::pair<polly::BBPair, BranchInst *>
polly::executeTopLevelScopConditionally(Scop &S, Value *RTC, DominatorTree &DT,
                                        RegionInfo &RI, LoopInfo &LI) {
  Region *R = &S.getRegion();
  Function *F = R->getEntry()->getParent();

  splitEntryBlockForAlloca(R->getEntry(), &DT, &LI, &RI);

  BasicBlock *SingleExitBlock = unifyFunctionExitNodes(*F);

  BasicBlock *ExitPred = SingleExitBlock->getSinglePredecessor();
  assert(ExitPred && "Multiple predessors not yet supported.");

  ScopAnnotator Annotator;
  PollyIRBuilder Builder = createPollyIRBuilder(R->getEntry(), Annotator);

  BasicBlock *SplitBlock =
      BasicBlock::Create(F->getContext(), "polly.split", F);

  BasicBlock *JoinBlock =
      splitEdge(ExitPred, SingleExitBlock, "polly.merge", &DT, &LI, &RI);

  BasicBlock *StartBlock =
      BasicBlock::Create(F->getContext(), "polly.start", F);

  BasicBlock *ExitingBlock =
      BasicBlock::Create(F->getContext(), "polly.exiting", F);

  Builder.SetInsertPoint(SplitBlock);
  Builder.CreateBr(StartBlock);

  Builder.SetInsertPoint(StartBlock);
  Builder.CreateBr(ExitingBlock);

  Builder.SetInsertPoint(ExitingBlock);
  Builder.CreateBr(JoinBlock);

  BasicBlock *EntrySucc = R->getEntry()->getSingleSuccessor();
  assert(EntrySucc);
  R->getEntry()->getTerminator()->eraseFromParent();
  Builder.SetInsertPoint(R->getEntry());
  BranchInst *CondBr = Builder.CreateCondBr(RTC, EntrySucc, SplitBlock);

  DT.recalculate(*F);
  // TODO : Update DT incrementally
  if (Loop *L = LI.getLoopFor(SplitBlock)) {
    L->addBasicBlockToLoop(StartBlock, LI);
    L->addBasicBlockToLoop(ExitingBlock, LI);
  }
  return std::make_pair(std::make_pair(StartBlock, ExitingBlock), CondBr);
}
