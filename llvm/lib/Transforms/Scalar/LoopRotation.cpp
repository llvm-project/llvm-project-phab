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
// 5. Update DOM
//
// TODO: Const correctness.
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/Scalar/LoopRotation.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/BasicAliasAnalysis.h"
#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/Analysis/CodeMetrics.h"
#include "llvm/Analysis/InstructionSimplify.h"
#include "llvm/Analysis/GlobalsModRef.h"
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
using namespace llvm;

#define DEBUG_TYPE "loop-rotate"

static cl::opt<unsigned> RotationMaxHeaderSize(
    "rotation-max-header-size", cl::init(16), cl::Hidden,
    cl::desc("The default maximum header size for automatic loop rotation"));

static cl::opt<unsigned> RotationMaxSize(
    "rotation-max-size", cl::init(100), cl::Hidden,
    cl::desc("The default maximum loop size for automatic loop rotation"));

static cl::opt<int> MaxLoopsRotated(
    "max-loops-rotated", cl::init(-1), cl::Hidden,
    cl::desc("The maximum loops to be rotated (-1 means no limit)"));

static cl::opt<unsigned> MaxExits("lr-max-exits", cl::init(10), cl::Hidden,
    cl::desc("The maximum exits to be cloned"));

static int LoopsRotated = 0;

STATISTIC(NumRotated, "Number of loops rotated");

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
  for (BasicBlock::iterator I = PhiBB->begin(),
       E = PhiBB->end(); I != E; ++I) {
    PHINode *PN = dyn_cast<PHINode>(I);
    if (!PN)
      break;
    PN->removeIncomingValue(PN->getBasicBlockIndex(From));
  }
}

/// A simple loop rotation transformation.
class LoopRotate {
  const unsigned MaxHeaderSize;
  LoopInfo *LI;
  const TargetTransformInfo *TTI;
  AssumptionCache *AC;
  DominatorTree *DT;
  ScalarEvolution *SE;
  Loop *L;
public:
  LoopRotate(unsigned MaxHeaderSize, LoopInfo *LI,
             const TargetTransformInfo *TTI, AssumptionCache *AC,
             DominatorTree *DT, ScalarEvolution *SE, Loop *L)
      : MaxHeaderSize(MaxHeaderSize), LI(LI), TTI(TTI), AC(AC), DT(DT),
        SE(SE), L(L) {}
  bool processLoop();

private:
  void rotateLoop(BasicBlock *NewH, const SmallVectorImpl<BasicBlock *> &Blocks,
                  const SmallPtrSetImpl<BasicBlock *> &Exits);

  void addPhis(SmallVectorImpl<BasicBlock *> &Blocks, ValueToValueMapTy &VMap,
               const Twine &NameSuffix, BasicBlock *NewHeader,
               BasicBlock *NewPreheader, BasicBlock *NewLatch);

  bool isProfitableToRotate(const BasicBlock *OrigH,
                            const SmallVectorImpl<BasicBlock *> &Blocks,
                            const SmallPtrSetImpl<BasicBlock *> &Exits);

  bool isLegalToRotate();

  void adjustNewHeaderPhis(ValueToValueMapTy &VMap, BasicBlock *NewH,
                           BasicBlock *NewPH);

  BasicBlock *collectSEMEBlocks(BasicBlock *OrigH, BasicBlock *OrigLatch,
                                SmallVectorImpl<BasicBlock *> &Blocks,
                                SmallPtrSetImpl<BasicBlock *> &Exits);

  void addPhisToNewHeader(const SmallVectorImpl<BasicBlock *> &Blocks,
                          BasicBlock *NewHeader, BasicBlock *NewPreheader,
                          BasicBlock *NewLatch, ValueToValueMapTy &VMap);
};

bool LoopRotate::isLegalToRotate() {
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

  if (!isSEME(OrigH, LoopLatch, DT))
    return false;

  return true;
}

bool LoopRotate::isProfitableToRotate(const BasicBlock *OrigH,
                                      const SmallVectorImpl<BasicBlock *> &Blocks,
                                      const SmallPtrSetImpl<BasicBlock *> &Exits) {
  // Check size of original header and reject loop if it is very big or we can't
  // duplicate blocks inside it.
  if (Exits.size() > MaxExits)
    return false;
  SmallPtrSet<const Value *, 32> EphValues;
  CodeMetrics::collectEphemeralValues(L, AC, EphValues);
  unsigned LoopSize = MaxHeaderSize;
  for(const BasicBlock *BB : Blocks) {
    CodeMetrics Metrics;
    Metrics.analyzeBasicBlock(BB, *TTI, EphValues);
    // TODO: Modify this because there might be blocks with
    // indirectbr, invoke in the loop but we can cut the cloning part at that point
    // and that will be the last exit BB
    if(Metrics.notDuplicatable) {
      DEBUG(dbgs() << "LoopRotation: NOT rotating - contains non-duplicatable"
            << " instructions: ";
          L->dump());
      return false;
    }

    if(Metrics.convergent) {
      DEBUG(dbgs() << "LoopRotation: NOT rotating - contains convergent "
                      "instructions: ";
          L->dump());
      return false;
    }

    if((BB == OrigH) && Metrics.NumInsts > MaxHeaderSize)
      return false;

    // TODO: Once this is crossed we can copy until the prev. BB.
    if(LoopSize + Metrics.NumInsts >= RotationMaxSize)
      return false;
    LoopSize += Metrics.NumInsts;
  }
  return true;
}

// Add phis to the new header and adjust the phi nodes from the OrigHeader.
void LoopRotate::addPhisToNewHeader(const SmallVectorImpl<BasicBlock *> &Blocks,
                                    BasicBlock *NewHeader,
                                    BasicBlock *NewPreheader,
                                    BasicBlock *NewLatch,
                                    ValueToValueMapTy &VMap) {
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

        PHINode *PN = PHINode::Create(Inst.getType(), 2, "phi.nh",
                                      &*NewHeader->begin());
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
                                     BasicBlock *NewPH) {
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

BasicBlock *
LoopRotate::collectSEMEBlocks(BasicBlock *OrigH, BasicBlock *OrigLatch,
                              SmallVectorImpl<BasicBlock *> &Blocks,
                              SmallPtrSetImpl<BasicBlock *> &Exits) {
  BasicBlock *NewH = nullptr;
  for (auto I = df_begin(OrigH), E = df_end(OrigH); I != E;) {
    if (!L->contains(*I)) {
      I.skipChildren();
      continue;
    }

    // Copy until any BB where the branch does not exit loop, or the loop-latch.
    if (OrigLatch == *I || !L->isLoopExiting(*I)
        || !isa<BranchInst>((*I)->getTerminator())) {
      // This will become the new header.
      NewH = *I;
      I.skipChildren();
    } else {
      Blocks.push_back(*I);

      BranchInst *BI = cast<BranchInst>((*I)->getTerminator());
      for (unsigned B = 0, E = BI->getNumSuccessors(); B < E; ++B) {
        BasicBlock *Succ = BI->getSuccessor(B);
        if (!L->contains(Succ))
          Exits.insert(Succ);
      }
      ++I;
    }
  }
  return NewH;
}

/// Rotate loop L.
/// TODO: arrange newly inserted bbs.
void LoopRotate::rotateLoop(BasicBlock *NewH,
                            const SmallVectorImpl<BasicBlock *> &Blocks,
                            const SmallPtrSetImpl<BasicBlock *> &Exits) {
  BasicBlock *OrigH = L->getHeader();
  BasicBlock *OrigLatch = L->getLoopLatch();
  BasicBlock *OrigPH = L->getLoopPreheader();

  DEBUG(dbgs() << "LoopRotation: rotating "; L->dump());

  ValueToValueMapTy VMap;
  copySEME(Blocks, Exits, VMap, ".lr", DT, LI);

  // Redirect original preheader to the entry of SEME.
  BranchInst *PBI = dyn_cast<BranchInst>(OrigPH->getTerminator());
  assert(PBI && (1 == PBI->getNumSuccessors()));

  BasicBlock *CopyOrigH = cast<BasicBlock>(VMap[OrigH]);
  PBI->setSuccessor(0, CopyOrigH);
  DT->changeImmediateDominator(CopyOrigH, OrigPH);
  L->moveToHeader(NewH);

  BasicBlock *BeforeLoop = nullptr;
  for (BasicBlock *BB: predecessors(NewH))
    if (!L->contains(BB)) {
      BeforeLoop = BB;
      break;
    }
  assert(BeforeLoop && "No entry point to the loop from New Header.");

  BasicBlock *NewPH = BasicBlock::Create(NewH->getContext(),
                                         NewH->getName() + ".lr.ph",
                                         NewH->getParent(), BeforeLoop);
  Loop *OuterLoop = LI->getLoopFor(OrigPH);
  if (OuterLoop)
    OuterLoop->addBasicBlockToLoop(NewPH, *LI);

  // Move NewH physically to the beginning of the loop.
  Function *F = BeforeLoop->getParent();
  F->getBasicBlockList().splice(OrigH->getIterator(), F->getBasicBlockList(),
                                  NewH);
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
    std::vector<DTNode*>::iterator I = IDom->begin();
    for (; I != IDom->end();) {
      BasicBlock *ExitBB = (*I)->getBlock();
      if (L->contains(ExitBB)) {
        ++I;
        continue;
      }

      BasicBlock *StaleIDom = DT->getNode(ExitBB)->getIDom()->getBlock();
      //assert (BB == StaleIDom);
      BasicBlock *NewBB = cast<BasicBlock>(VMap[BB]);
      // VERIFY: NewIDom will be correct because this part of CFG is up-to-date.
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

  addPhisToNewHeader(Blocks, NewH, NewPH, NewLatch, VMap);

  // Discard incoming values in the CopyOrigHeader, which are coming from
  // OrigLatch since it has only one predecessor.
  discardIncomingValues(CopyOrigH, OrigLatch);
  discardIncomingValues(OrigH, OrigPH);

  LI->verify();
  DT->verifyDomTree();
  assert(L->isLCSSAForm(*DT) && "Loop is not in LCSSA form.");
  DEBUG(verifyFunction(*F));
  DEBUG(dbgs() << "\nLoopRotation: rotated "; L->dumpVerbose());

  assert (isSEME(L->getHeader(), NewLatch, DT));
  assert (isSEME(CopyOrigH, NewPH, DT));

  ++NumRotated;
}

/// Rotate \c L, and return true if any modification was made.
bool LoopRotate::processLoop() {
  if (!isLegalToRotate())
    return false;

  BasicBlock *OrigH = L->getHeader();
  BasicBlock *LoopLatch = L->getLoopLatch();

  // Basic blocks to be copied.
  SmallVector<BasicBlock *, 4> Blocks;
  SmallPtrSet<BasicBlock *, 4> Exits;
  // Collect all nodes of the loop from header to latch.
  BasicBlock *NewH = collectSEMEBlocks(OrigH, LoopLatch, Blocks, Exits);
  assert(NewH);

  if (!isProfitableToRotate(OrigH, Blocks, Exits))
    return false;

  if (MaxLoopsRotated != -1) {
    if (LoopsRotated >= MaxLoopsRotated)
      return false;
    ++LoopsRotated;
  }

  // Save the loop metadata.
  MDNode *LoopMD = L->getLoopID();

  // Make sure the latch has only one successor.
  if (!LoopLatch->getSingleSuccessor()) {
    DEBUG(dbgs() << "\nSplitting the edge of Loop:"; L->dumpVerbose(););
    LoopLatch = SplitEdge(LoopLatch, OrigH, DT, LI);
    DEBUG(dbgs() << "\nLoop in canonical form:"; L->dumpVerbose(););
    // After splitting the blocks need to be recollected.
    Blocks.clear();
    Exits.clear();
    NewH = collectSEMEBlocks(OrigH, LoopLatch, Blocks, Exits);
    assert(NewH);
  }

  assert(LoopLatch->getSingleSuccessor());

  // Anything ScalarEvolution may know about this loop or the PHI nodes
  // in its header will soon be invalidated.
  if (SE)
    SE->forgetLoop(L);

  rotateLoop(NewH, Blocks, Exits);
  assert(L->isLoopExiting(L->getLoopLatch()) &&
         "Loop latch should be exiting after loop-rotate.");

  // Restore the loop metadata.
  // NB! We presume LoopRotation DOESN'T ADD its own metadata.
  if (LoopMD)
    L->setLoopID(LoopMD);

  return true;
}

LoopRotatePass::LoopRotatePass() {}

PreservedAnalyses LoopRotatePass::run(Loop &L, AnalysisManager<Loop> &AM) {
  auto &FAM = AM.getResult<FunctionAnalysisManagerLoopProxy>(L).getManager();
  Function *F = L.getHeader()->getParent();

  auto *LI = FAM.getCachedResult<LoopAnalysis>(*F);
  const auto *TTI = FAM.getCachedResult<TargetIRAnalysis>(*F);
  auto *AC = FAM.getCachedResult<AssumptionAnalysis>(*F);
  assert((LI && TTI && AC) && "Analyses for loop rotation not available");

  // Optional analyses.
  auto *DT = FAM.getCachedResult<DominatorTreeAnalysis>(*F);
  auto *SE = FAM.getCachedResult<ScalarEvolutionAnalysis>(*F);
  LoopRotate LR(RotationMaxHeaderSize, LI, TTI, AC, DT, SE, &L);

  bool Changed = LR.processLoop();
  if (!Changed)
    return PreservedAnalyses::all();
  return getLoopPassPreservedAnalyses();
}

namespace {

class LoopRotateLegacyPass : public LoopPass {
  unsigned MaxHeaderSize;

public:
  static char ID; // Pass ID, replacement for typeid
  LoopRotateLegacyPass(int SpecifiedMaxHeaderSize = -1) : LoopPass(ID) {
    initializeLoopRotateLegacyPassPass(*PassRegistry::getPassRegistry());
    if (SpecifiedMaxHeaderSize == -1)
      MaxHeaderSize = RotationMaxHeaderSize;
    else
      MaxHeaderSize = unsigned(SpecifiedMaxHeaderSize);
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
    LoopRotate LR(MaxHeaderSize, LI, TTI, AC, DT, SE, L);
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

Pass *llvm::createLoopRotatePass(int MaxHeaderSize) {
  return new LoopRotateLegacyPass(MaxHeaderSize);
}
