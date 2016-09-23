//===- LoopRotation.cpp - Loop Rotation Pass ------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements Loop Rotation Pass:
// 1. Canonicalize loop latch to have only one successor.
// 2. Clone all the BBs which are exiting the loop.
// 3. Adjust phi of cloned BBs
// 4. Add phi to the new loop header
// 5. Update DT
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/Scalar/LoopRotation.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/Analysis/BasicAliasAnalysis.h"
#include "llvm/Analysis/CodeMetrics.h"
#include "llvm/Analysis/GlobalsModRef.h"
#include "llvm/Analysis/InstructionSimplify.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/LoopPassManager.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionAliasAnalysis.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/Utils/Local.h"
#include "llvm/Transforms/Utils/LoopUtils.h"
#include "llvm/Transforms/Utils/SSAUpdater.h"
#include "llvm/Transforms/Utils/ValueMapper.h"

#include<tuple>

using namespace llvm;

#define DEBUG_TYPE "loop-rotate"

static cl::opt<unsigned> RotationMaxSize(
    "rotation-max-size", cl::init(100), cl::Hidden,
    cl::desc("The default maximum loop size for automatic loop rotation"));

static cl::opt<unsigned> MaxExits("lr-max-exits", cl::init(10), cl::Hidden,
                                  cl::desc("The maximum exits to be cloned"));

static cl::opt<unsigned> RotationsMaxAllowed(
    "lr-max-allowed", cl::init(1), cl::Hidden,
    cl::desc("The maximum rotations of a loop"));

STATISTIC(NumRotated, "Number of loops rotated");

// Insert \p NewBB in between \p PredBefore and \p Succ, and redirect edges
// accordingly. PredBefore -> NewBB -> Succ. Also move NewBB before Succ.
static void insertBetween(BasicBlock *NewBB, BasicBlock *PredBefore,
                          BasicBlock *Succ) {
  BranchInst *NewBI = BranchInst::Create(Succ, NewBB);
  NewBI->setDebugLoc(PredBefore->getTerminator()->getDebugLoc());

  BranchInst *BLI = dyn_cast<BranchInst>(PredBefore->getTerminator());
  for (unsigned I = 0, E = BLI->getNumSuccessors(); I < E; ++I)
    if (BLI->getSuccessor(I) == Succ) {
      BLI->setSuccessor(I, NewBB);
      break;
    }
  // Move NewBB physically from the end of the block list.
  Function *F = Succ->getParent();
  F->getBasicBlockList().splice(Succ->getIterator(), F->getBasicBlockList(),
                                NewBB);
}

// Remove the arguments of all phi nodes in PhiBB coming from block From.
static void discardIncomingValues(BasicBlock *PhiBB, BasicBlock *From) {
  for (Instruction &I : *PhiBB) {
    PHINode *PN = dyn_cast<PHINode>(&I);
    if (!PN)
      break;
    PN->removeIncomingValue(PN->getBasicBlockIndex(From));
  }
}

/// A simple loop rotation transformation.
class LoopRotate {
  LoopInfo *LI;
  const TargetTransformInfo *TTI;
  AssumptionCache *AC;
  DominatorTree *DT;
  ScalarEvolution *SE;
  Loop *L;
  const char *LoopRotatedTimes = "llvm.loop.rotated.times";
  typedef SmallVectorImpl<BasicBlock *> SmallVecBB;
  typedef SmallPtrSetImpl<BasicBlock *> SmallPtrSetBB;

public:
  LoopRotate(LoopInfo *LI, const TargetTransformInfo *TTI, AssumptionCache *AC,
             DominatorTree *DT, ScalarEvolution *SE, Loop *L)
      : LI(LI), TTI(TTI), AC(AC), DT(DT), SE(SE), L(L) {}
  bool processLoop();

private:
  void rotateLoop(BasicBlock *NewH, const SmallVecBB &Blocks,
                  const SmallPtrSetBB &Exits) const;

  bool isProfitableToRotate(const SmallVecBB &Blocks,
                            const SmallPtrSetBB &Exits) const;

  bool isLegalToRotate() const;

  void adjustNewHeaderPhis(ValueToValueMapTy &VMap, BasicBlock *NewH,
                           BasicBlock *NewPH) const;

  BasicBlock *collectSEMEBlocks(BasicBlock *OrigH, BasicBlock *OrigLatch,
                                SmallVecBB &Blocks, SmallPtrSetBB &Exits) const;

  void addNewPhisToNewHeader(const SmallVecBB &Blocks, BasicBlock *NewHeader,
                             BasicBlock *NewPreheader, BasicBlock *NewLatch,
                             ValueToValueMapTy &VMap) const;
};

// Check if there is something preventing loop from being rotated.
bool LoopRotate::isLegalToRotate() const {
  // If the loop has only one block then there is not much to rotate.
  if (L->getBlocks().size() == 1)
    return false;

  const BasicBlock *LoopLatch = L->getLoopLatch();
  if (!LoopLatch || isa<IndirectBrInst>(LoopLatch->getTerminator()))
    return false;

  const BasicBlock *OrigH = L->getHeader();
  // The header may have invoke instruction to BBs within the loop.
  const BranchInst *BI = dyn_cast<const BranchInst>(OrigH->getTerminator());
  if (!BI || !BI->isConditional())
    return false;

  // If the loop header is not one of the loop exiting blocks then
  // either this loop is already rotated or it is not
  // suitable for loop rotation transformations.
  if (!L->isLoopExiting(OrigH))
    return false;

  // If the loop could not be converted to canonical form, it must have an
  // indirectbr in it, just give up.
  if (!L->getLoopPreheader())
    return false;

  if (!isSingleEntryMultipleExit(OrigH, LoopLatch, DT))
    return false;

  return true;
}

// Check if the size of rotated loop will be within limits.
bool LoopRotate::isProfitableToRotate(const SmallVecBB &Blocks,
                                      const SmallPtrSetBB &Exits) const {
  // Check size of original header and reject loop if it is very big or we can't
  // duplicate blocks inside it.
  if (Exits.size() > MaxExits)
    return false;
  SmallPtrSet<const Value *, 32> EphValues;
  CodeMetrics::collectEphemeralValues(L, AC, EphValues);
  unsigned LoopSize = 0;
  for (const BasicBlock *BB : Blocks) {
    CodeMetrics Metrics;
    Metrics.analyzeBasicBlock(BB, *TTI, EphValues);
    // TODO: Modify this because there might be blocks with indirectbr, invoke
    // in the loop but we can cut the cloning part at that point and that will
    // be the last exit BB.
    if (Metrics.notDuplicatable) {
      DEBUG(dbgs() << "LoopRotation: NOT rotating - contains non-duplicatable"
                   << " instructions: ";
            L->dump());
      return false;
    }

    if (Metrics.convergent) {
      DEBUG(dbgs() << "LoopRotation: NOT rotating - contains convergent "
                      "instructions: ";
            L->dump());
      return false;
    }

    // TODO: Once this is crossed we can copy until the previous BB.
    if (LoopSize + Metrics.NumInsts >= RotationMaxSize)
      return false;
    LoopSize += Metrics.NumInsts;
  }
  return true;
}

// Add phis to the new header and adjust the phi nodes from the OrigHeader.
void LoopRotate::addNewPhisToNewHeader(const SmallVecBB &Blocks,
                                       BasicBlock *NewHeader,
                                       BasicBlock *NewPreheader,
                                       BasicBlock *NewLatch,
                                       ValueToValueMapTy &VMap) const {
  // Add to NewHeader phi nodes for all copied variables which are used.
  for (BasicBlock *BB : Blocks) {
    for (Instruction &Inst : *BB) {
      // Skip Ins with no use e.g., branches.
      if (Inst.use_begin() == Inst.use_end())
        continue;

      for (auto UI = Inst.use_begin(), E = Inst.use_end(); UI != E;) {
        Use &U = *UI++;
        Instruction *UserInst = cast<Instruction>(U.getUser());

        // Nothing to rename when the use is dominated by the definition.
        if (DT->dominates(&Inst, UserInst))
          continue;

        if (!L->contains(UserInst->getParent())) {
          // Handle uses in the loop-closed-phi.
          PHINode *ClosePhi = cast<PHINode>(UserInst);
          BasicBlock *Pred = ClosePhi->getIncomingBlock(U.getOperandNo());

          // Do not rename a loop close phi node if its predecessor in the loop
          // is dominated by Inst.
          if (L->contains(Pred) && DT->dominates(BB, Pred))
            continue;
        }

        PHINode *PN =
            PHINode::Create(Inst.getType(), 2, "phi.nh", &*NewHeader->begin());
        PN->addIncoming(&Inst, NewLatch);
        PN->addIncoming(cast<Instruction>(VMap[&Inst]), NewPreheader);

        // When Inst does not dominate U, it is going to use the updated
        // definition coming from PN.
        U.set(PN);
      }
    }
  }
}

// Add incoming values to the (already present) PHIs of NewH.
void LoopRotate::adjustNewHeaderPhis(ValueToValueMapTy &VMap, BasicBlock *NewH,
                                     BasicBlock *NewPH) const {
  for (Instruction &Inst : *NewH) {
    PHINode *PN = dyn_cast<PHINode>(&Inst);
    if (!PN)
      break;
    assert((PN->getNumOperands() == 1) && "NewH had multiple predecessors.");
    Value *Op = PN->getIncomingValue(0);
    if (Value *RenamedVal = VMap[Op])
      PN->addIncoming(RenamedVal, NewPH);
    else // When no mapping is available (e.g., in case of a constant).
      PN->addIncoming(Op, NewPH);
  }
}

// OrigH and OrigLatch represent an SEME region.  Collect all BasicBlocks
// (bounded by \p OrigH and \p OrigLatch), which are exiting the loop in
// \p Blocks and collect all the exits from the region in \p Exits Returns the
// first basic block which was not copied.
BasicBlock *LoopRotate::collectSEMEBlocks(BasicBlock *OrigH,
                                          BasicBlock *OrigLatch,
                                          SmallVecBB &Blocks,
                                          SmallPtrSetBB &Exits) const {
  BasicBlock *NewH = nullptr;
  for (auto BB = df_begin(OrigH), E = df_end(OrigH); BB != E;) {
    if (!L->contains(*BB)) {
      BB.skipChildren();
      continue;
    }

    // Copy until any BB where the branch does not exit loop, or the loop-latch.
    if (OrigLatch == *BB || !L->isLoopExiting(*BB) ||
        !isa<BranchInst>((*BB)->getTerminator())) {
      // This will become the new header.
      NewH = *BB;
      BB.skipChildren();
    } else {
      Blocks.push_back(*BB);

      BranchInst *BI = cast<BranchInst>((*BB)->getTerminator());
      for (unsigned B = 0, E = BI->getNumSuccessors(); B < E; ++B) {
        BasicBlock *Succ = BI->getSuccessor(B);
        if (!L->contains(Succ))
          Exits.insert(Succ);
      }
      ++BB;
    }
  }
  return NewH;
}

// Helper function for copySEME. Adjusts the PHIs of all the \p Exits bounding
// an SEME. \p VMap contains mapping of original BB vs copied BB.
static void adjustExitingPhis(ValueToValueMapTy &VMap,
                              const SmallPtrSetImpl<BasicBlock *> &Exits) {
  for (BasicBlock *BB : Exits) {
    for (Instruction &Inst : *BB) {
      PHINode *PN = dyn_cast<PHINode>(&Inst);
      if (!PN)
        break;
      bool EdgeFromOrigBB = false;
      for (unsigned i = 0, e = PN->getNumOperands(); i != e; ++i) {
        Value *CopyB = VMap[PN->getIncomingBlock(i)];
        if (!CopyB) // Skip args coming from outside the SEME.
          continue;
        BasicBlock *CopyBB = cast<BasicBlock>(CopyB);
        EdgeFromOrigBB = true;
        Value *Op = PN->getIncomingValue(i);
        if (Value *RenamedVal = VMap[Op])
          PN->addIncoming(RenamedVal, CopyBB);
        else
          // When no mapping is available it must be a constant.
          PN->addIncoming(Op, CopyBB);
      }
      assert(EdgeFromOrigBB && "Illegal exit from SEME.");
    }
  }
}

/// Clones the basic blocks (\p Blocks) of an SEME bounded by \p Exits.
/// Blocks[0] is the entry basic block of the SEME.
/// The mapping between original BBs and correponding copied BBs are
/// populated in \p VMap. During copy the DOM and LI of CFG are updated.
/// \returns The entry point of the copied SEME. \p NameSuffix is used to suffix
/// name of the copied BBs. The copied SEME is also an SEME.
static BasicBlock* copyBlocks(const SmallVectorImpl<BasicBlock *> &Blocks,
                              const SmallPtrSetImpl<BasicBlock *> &Exits,
                              ValueToValueMapTy &VMap,
                              const Twine &NameSuffix,
                              DominatorTree *DT, LoopInfo *LI) {
  // Step1: Clone the basic blocks and populate VMap.
  BasicBlock *OrigH = Blocks[0];

  Function *F = OrigH->getParent();
  SmallVector<BasicBlock *, 4> NewBlocks;
  for (BasicBlock *BB : Blocks) {
    assert(!isa<IndirectBrInst>(BB->getTerminator()) &&
           "Cannot clone SEME with indirect branches.");

    BasicBlock *NewBB = CloneBasicBlock(BB, VMap, NameSuffix, F);
    // Move them physically from the end of the block list.
    F->getBasicBlockList().splice(OrigH->getIterator(), F->getBasicBlockList(),
                                  NewBB);
    Loop *BBLoop = LI->getLoopFor(BB);
    Loop *BBParentLoop = BBLoop->getParentLoop();
    if (BBParentLoop)
      BBParentLoop->addBasicBlockToLoop(NewBB, *LI);
    VMap[BB] = NewBB;
    NewBlocks.push_back(NewBB);
  }

  // Step2: Remap the names in copied BBs.
  remapInstructionsInBlocks(NewBlocks, VMap);

  // Step3: Redirect the edges.
  for (BasicBlock *BB : Blocks) {
    BasicBlock *NewBB = cast<BasicBlock>(VMap[BB]);
    BranchInst *BI = dyn_cast<BranchInst>(NewBB->getTerminator());
    if (!BI)
      continue;

    for (unsigned I = 0, E = BI->getNumSuccessors(); I < E; ++I)
      if (auto *NewSucc = cast_or_null<BasicBlock>(VMap[BI->getSuccessor(I)]))
        BI->setSuccessor(I, NewSucc);
  }

  // Step4: Update the DOM of copied SEME. Except for the entry block its tree
  // structure is the same as of original SEME so the dominators also follow the
  // same structural property. If the IDom of original BB is not in SEME that
  // means it is the entry block, in that case the new IDom of the new BB must
  // be its single predecessor because we are dealing with an SEME region.
  BasicBlock *EntryNewSEME = nullptr;
  if (auto *DomT = DT->getNode(Blocks[0])->getIDom()) {
    // Entry to SEME has a dominator, update the copied entry.
    BasicBlock *Dom = DomT->getBlock();
    assert(!VMap[Dom]); // Dom does not belong to SEME => entry block.
    BasicBlock *NewBB = cast<BasicBlock>(VMap[Blocks[0]]);
    EntryNewSEME = NewBB;
    DT->addNewBlock(NewBB, Dom);
    DT->changeImmediateDominator(NewBB, Dom);
  }

  for (auto BBI = std::next(Blocks.begin()); BBI != Blocks.end(); ++BBI) {
    BasicBlock *NewBB = cast<BasicBlock>(VMap[*BBI]);
    BasicBlock *Dom = DT->getNode(*BBI)->getIDom()->getBlock();
    BasicBlock *NewDom = cast<BasicBlock>(VMap[Dom]);
    DT->addNewBlock(NewBB, NewDom);
    DT->changeImmediateDominator(NewBB, NewDom);
  }

  // Step5: Adjust PHI nodes.
  adjustExitingPhis(VMap, Exits);
  return EntryNewSEME;
}

/// Rotate loop L.
/// TODO: arrange newly inserted bbs.
void LoopRotate::rotateLoop(BasicBlock *NewH, const SmallVecBB &Blocks,
                            const SmallPtrSetBB &Exits) const {
  BasicBlock *OrigH = L->getHeader();
  BasicBlock *OrigLatch = L->getLoopLatch();
  BasicBlock *OrigPH = L->getLoopPreheader();

  DEBUG(dbgs() << "LoopRotation: rotating "; L->dump());

  ValueToValueMapTy VMap;
  copyBlocks(Blocks, Exits, VMap, ".lr", DT, LI);

  // Redirect original preheader to the entry of SEME.
  BranchInst *PBI = dyn_cast<BranchInst>(OrigPH->getTerminator());
  assert(PBI && (1 == PBI->getNumSuccessors()) &&
         "Preheader does not have single successor.");

  BasicBlock *CopyOrigH = cast<BasicBlock>(VMap[OrigH]);
  PBI->setSuccessor(0, CopyOrigH);
  DT->changeImmediateDominator(CopyOrigH, OrigPH);
  L->moveToHeader(NewH);

  BasicBlock *BeforeLoop = nullptr;
  for (BasicBlock *BB : predecessors(NewH))
    if (!L->contains(BB)) {
      BeforeLoop = BB;
      break;
    }
  assert(BeforeLoop && "No entry point to the loop from New Header.");
  Function *F = BeforeLoop->getParent();
  // Move NewH physically to the beginning of the loop.
  F->getBasicBlockList().splice(OrigH->getIterator(), F->getBasicBlockList(),
                                NewH);

  // SplitEdge does not work properly with single-entry PHIs.
  BasicBlock *NewPH = BasicBlock::Create(
      NewH->getContext(), NewH->getName() + ".lr.ph", F, BeforeLoop);

  Loop *OuterLoop = LI->getLoopFor(OrigPH);
  if (OuterLoop)
    OuterLoop->addBasicBlockToLoop(NewPH, *LI);

  // BeforeLoop --> NewPH --> NewH.
  insertBetween(NewPH, BeforeLoop, NewH);

  DT->addNewBlock(NewPH, BeforeLoop);
  DT->changeImmediateDominator(NewPH, BeforeLoop);
  DT->changeImmediateDominator(NewH, NewPH);

  // Also, the original entry lost its immediate dominator so its dominator
  // should be adjusted. We use SEME property => idom (OrigH) = its single pred.
  DT->changeImmediateDominator(OrigH, OrigH->getSinglePredecessor());

  for (BasicBlock *BB : Blocks) {
    typedef DomTreeNodeBase<BasicBlock> DTNode;
    DTNode *IDom = DT->getNode(BB);
    std::vector<DTNode *>::iterator I = IDom->begin();
    for (; I != IDom->end();) {
      BasicBlock *ExitBB = (*I)->getBlock();
      if (L->contains(ExitBB)) {
        ++I;
        continue;
      }

      BasicBlock *StaleIDom = DT->getNode(ExitBB)->getIDom()->getBlock();
      BasicBlock *NewBB = cast<BasicBlock>(VMap[BB]);
      // NewIDom is correct because this part of CFG is up-to-date.
      BasicBlock *NewIDom = DT->findNearestCommonDominator(StaleIDom, NewBB);
      NewIDom = DT->findNearestCommonDominator(NewIDom, BB);
      if (NewIDom != StaleIDom) {
        DT->changeImmediateDominator(ExitBB, NewIDom);
        DEBUG(dbgs() << "\nChanging IDom of " << *ExitBB << "to" << *NewIDom);
        I = IDom->begin();
      } else
        ++I;
    }
  }

  adjustNewHeaderPhis(VMap, NewH, NewPH);

  BasicBlock *NewLatch = L->getLoopLatch();
  assert(L->getLoopPreheader() && "Invalid loop preheader after rotation");
  assert(NewLatch && "Invalid loop latch after rotation");

  addNewPhisToNewHeader(Blocks, NewH, NewPH, NewLatch, VMap);

  // Discard incoming values in the CopyOrigHeader, which are coming from
  // OrigLatch since it has only one predecessor.
  discardIncomingValues(CopyOrigH, OrigLatch);
  discardIncomingValues(OrigH, OrigPH);

  DEBUG(LI->verify(*DT));
  DEBUG(DT->verifyDomTree());
  assert(L->isRecursivelyLCSSAForm(*DT) && "Loop is not in LCSSA form.");

  DEBUG(dbgs() << "\nLoopRotation: rotated "; L->dumpVerbose());

  assert(isSingleEntryMultipleExit(L->getHeader(), NewLatch, DT) &&
         "Rotated loop not an SEME");
  assert(isSingleEntryMultipleExit(CopyOrigH, NewPH, DT) &&
         "Copied SEME not an SEME");

  ++NumRotated;
}

/// Rotate \c L, and return true if any modification was made.
bool LoopRotate::processLoop() {
  int Times = 0;
  if (auto Value = findStringMetadataForLoop(L, LoopRotatedTimes)) {
    const MDOperand *Op = *Value;
    assert(Op && mdconst::hasa<ConstantInt>(*Op) && "invalid metadata");
    Times = mdconst::extract<ConstantInt>(*Op)->getZExtValue();
    if (Times >= RotationsMaxAllowed)
      return false;
  }

  if (!isLegalToRotate())
    return false;

  BasicBlock *OrigH = L->getHeader();
  BasicBlock *LoopLatch = L->getLoopLatch();

  // Basic blocks to be copied.
  SmallVector<BasicBlock *, 4> Blocks;
  SmallPtrSet<BasicBlock *, 4> Exits;
  // Collect all nodes of the loop from header to latch.
  BasicBlock *NewH = collectSEMEBlocks(OrigH, LoopLatch, Blocks, Exits);
  assert(NewH && "Invalid SEME region.");

  if (!isProfitableToRotate(Blocks, Exits))
    return false;

  // Save the loop metadata.
  MDNode *LoopMD = nullptr;
  Instruction *TI = nullptr;
  std::tie(LoopMD, TI) = L->getLoopIDWithInstr();

  // Make sure the latch has only one successor.
  if (!LoopLatch->getSingleSuccessor()) {
    // Since LoopLatch was skipped while collecting SEME blocks, exits out of
    // the loop-latch is collected here.
    BranchInst *BLI = dyn_cast<BranchInst>(LoopLatch->getTerminator());
    if (!BLI)
      return false;
    for (unsigned I = 0, E = BLI->getNumSuccessors(); I < E; ++I) {
      BasicBlock *Succ = BLI->getSuccessor(I);
      if (Succ != OrigH && !L->contains(Succ))
        Exits.insert(Succ);
    }

    // The old loop-latch will become the parent of new loop-latch.
    DEBUG(dbgs() << "\nSplitting the edge of Loop:"; L->dumpVerbose(););
    BasicBlock *NewLoopLatch = SplitEdge(LoopLatch, OrigH, DT, LI);

    // If the NewH was loop-latch => NewH would change as the loop-latch has
    // changed after splitting. Also, we need to add the (old) loop-latch to the
    // blocks because it was skipped (all the blocks before NewH are copied to
    // new SEME).
    if (NewH == LoopLatch) {
      NewH = NewLoopLatch;
      Blocks.push_back(LoopLatch);
    }
    LoopLatch = NewLoopLatch;
  }

  assert(LoopLatch->getSingleSuccessor() && "Invalid SEME region.");

  // Anything ScalarEvolution may know about this loop or the PHI nodes
  // in its header will soon be invalidated.
  if (SE)
    SE->forgetLoop(L);

  rotateLoop(NewH, Blocks, Exits);
  assert(L->isLoopExiting(L->getLoopLatch()) &&
         "Loop latch should be exiting after loop-rotate.");

  if (LoopMD) {
    // Drop the metadata from the old loop header.
    SmallVector<unsigned, 2> MDs;
    TI->getAllNonDebugMetadataIDs(MDs);
    MDs.erase(llvm::remove_if(MDs, [](unsigned ID) {
          return ID == LLVMContext::MD_loop; }), MDs.end());

    TI->dropUnknownNonDebugMetadata(MDs);

    // Restore the loop metadata to the new loop header.
    L->setLoopID(LoopMD);
  }

  // Add new metadata that loop has been rotated once.
  // The metadata reflects how many times loop has been rotated.
  addStringMetadataToLoop(L, LoopRotatedTimes, Times + 1);
  return true;
}

LoopRotatePass::LoopRotatePass() {}

PreservedAnalyses LoopRotatePass::run(Loop &L, LoopAnalysisManager &AM) {
  auto &FAM = AM.getResult<FunctionAnalysisManagerLoopProxy>(L).getManager();
  Function *F = L.getHeader()->getParent();

  auto *LI = FAM.getCachedResult<LoopAnalysis>(*F);
  const auto *TTI = FAM.getCachedResult<TargetIRAnalysis>(*F);
  auto *AC = FAM.getCachedResult<AssumptionAnalysis>(*F);
  assert((LI && TTI && AC) && "Analyses for loop rotation not available");

  // Optional analyses.
  auto *DT = FAM.getCachedResult<DominatorTreeAnalysis>(*F);
  auto *SE = FAM.getCachedResult<ScalarEvolutionAnalysis>(*F);
  LoopRotate LR(LI, TTI, AC, DT, SE, &L);

  bool Changed = LR.processLoop();
  if (!Changed)
    return PreservedAnalyses::all();
  return getLoopPassPreservedAnalyses();
}

namespace {

class LoopRotateLegacyPass : public LoopPass {
public:
  static char ID; // Pass ID, replacement for typeid
  LoopRotateLegacyPass() : LoopPass(ID) {
    initializeLoopRotateLegacyPassPass(*PassRegistry::getPassRegistry());
  }

  // LCSSA form makes instruction renaming easier.
  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<AssumptionCacheTracker>();
    AU.addRequired<TargetTransformInfoWrapperPass>();
    AU.addRequired<DominatorTreeWrapperPass>();
    getLoopAnalysisUsage(AU);
  }

  bool runOnLoop(Loop *L, LPPassManager &LPM) override {
    if (skipLoop(L))
      return false;
    Function &F = *L->getHeader()->getParent();

    auto *LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
    const auto *TTI = &getAnalysis<TargetTransformInfoWrapperPass>().getTTI(F);
    auto *AC = &getAnalysis<AssumptionCacheTracker>().getAssumptionCache(F);
    auto *DTWP = getAnalysisIfAvailable<DominatorTreeWrapperPass>();
    auto *DT = DTWP ? &DTWP->getDomTree() : nullptr;
    auto *SEWP = getAnalysisIfAvailable<ScalarEvolutionWrapperPass>();
    auto *SE = SEWP ? &SEWP->getSE() : nullptr;
    LoopRotate LR(LI, TTI, AC, DT, SE, L);
    return LR.processLoop();
  }
};
}

char LoopRotateLegacyPass::ID = 0;
INITIALIZE_PASS_BEGIN(LoopRotateLegacyPass, "loop-rotate", "Rotate Loops",
                      false, false)
INITIALIZE_PASS_DEPENDENCY(AssumptionCacheTracker)
INITIALIZE_PASS_DEPENDENCY(LoopPass)
INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
INITIALIZE_PASS_DEPENDENCY(TargetTransformInfoWrapperPass)
INITIALIZE_PASS_END(LoopRotateLegacyPass, "loop-rotate", "Rotate Loops", false,
                    false)

Pass *llvm::createLoopRotatePass() { return new LoopRotateLegacyPass(); }
