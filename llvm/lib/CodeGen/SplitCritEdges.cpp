//=== SplitCritEdges.cpp - Split critical edges for loop entry and exit ---===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This pass performs critical edges splitting for edges at the loop entry or
// exits. It also creates a loop preheader if the loop header has multiple
// predecessors. It is necessary to enable register splitting around loops,
// and it is also helpful for better register coalescing and shrinkwrapping.
//
//===----------------------------------------------------------------------===//
#include "llvm/ADT/Statistic.h"
#include "llvm/CodeGen/MachineDominators.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineLoopInfo.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/Target/TargetInstrInfo.h"
#include "llvm/Target/TargetSubtargetInfo.h"

#define DEBUG_TYPE "critedges-split"

using namespace llvm;

namespace {
class SplitCritEdges : public MachineFunctionPass {
  MachineDominatorTree *MDT;
  MachineLoopInfo *MLI;

public:
  static char ID;

  SplitCritEdges() : MachineFunctionPass(ID) {
    initializeSplitCritEdgesPass(*PassRegistry::getPassRegistry());
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<MachineDominatorTree>();
    AU.addRequired<MachineLoopInfo>();
    AU.addPreserved<MachineDominatorTree>();
    AU.addPreserved<MachineLoopInfo>();
    MachineFunctionPass::getAnalysisUsage(AU);
  }

  const char *getPassName() const override {
    return "Split Critical Edges for RegAlloc";
  }
  bool createLoopPreheader(MachineFunction &MF, MachineLoop *Loop);
  bool runOnMachineFunction(MachineFunction &MF) override;
};
} // End anonymous namespace.

char SplitCritEdges::ID = 0;
char &llvm::SplitCritEdgesID = SplitCritEdges::ID;

INITIALIZE_PASS_BEGIN(SplitCritEdges, "critedges-split",
                      "Critical Edges Split Pass", false, false)
INITIALIZE_PASS_DEPENDENCY(MachineDominatorTree)
INITIALIZE_PASS_DEPENDENCY(MachineLoopInfo)
INITIALIZE_PASS_END(SplitCritEdges, "critedges-split",
                    "Critical Edges Split Pass", false, false)

// If the header of the Loop has multiple predecessors outside of the loop,
// a new Loop preheader will be inserted and it will become the only predecessor
// of the header block outside of the loop.
bool SplitCritEdges::createLoopPreheader(MachineFunction &MF,
                                         MachineLoop *Loop) {
  MachineBasicBlock *Header = Loop->getHeader();
  MachineDomTreeNode *IDom = MDT->getNode(Header)->getIDom();

  // Predecessors of Header inside of loop.
  SmallPtrSet<MachineBasicBlock *, 8> PredsIn;
  // Predecessors of Header outside of loop.
  SmallPtrSet<MachineBasicBlock *, 8> PredsOut;
  MachineBasicBlock *TBB, *FBB;
  SmallVector<MachineOperand, 4> Cond;
  const TargetInstrInfo *TII = MF.getSubtarget().getInstrInfo();
  for (auto Pred : Header->predecessors()) {
    TBB = nullptr;
    FBB = nullptr;
    Cond.clear();
    if (TII->AnalyzeBranch(*Pred, TBB, FBB, Cond))
      return false;
    if (!Loop->contains(Pred))
      PredsOut.insert(Pred);
    else
      PredsIn.insert(Pred);
  }

  MachineBasicBlock *NMBB = MF.CreateMachineBasicBlock();
  MF.insert(MachineFunction::iterator(Header), NMBB);

  // Update CFG and terminator for predecessor blocks outside loop.
  for (auto Pred : PredsOut) {
    Pred->ReplaceUsesOfBlockWith(Header, NMBB);
    Pred->updateTerminator();
  }
  // Update terminator for predecessor blocks inside loop.
  for (auto Pred : PredsIn)
    Pred->updateTerminator();
  // Update CFG and terminator for the new block.
  DebugLoc DL;
  NMBB->addSuccessor(Header);
  if (!NMBB->isLayoutSuccessor(Header)) {
    Cond.clear();
    TII->InsertBranch(*NMBB, Header, nullptr, Cond, DL);
  }

  // Update PHI nodes:
  // Original PHI node in Header looks like:
  //   r1_0 = PHI(r1_1, PB_in, r1_2, PB_out1, r1_3, PB_out2)
  // PB_in is the Predecessor Block inside loop. PB_outX is the Xth
  // Predecessor Block outside loop.
  // After the update, a new PHI is generated in NMBB:
  //   r1_4 = PHI(r1_2, PB_out1, r1_3, PB_out2)
  // And the old PHI in Header Block is changed to:
  //   r1_0 = PHI(r1_1, PB_in, r1_4, NMBB)
  MachineRegisterInfo &MRI = MF.getRegInfo();
  for (MachineBasicBlock::instr_iterator I = Header->instr_begin(),
                                         E = Header->instr_end();
       I != E && I->isPHI(); ++I) {
    MachineInstr *OldPHI = &*I;
    MachineInstr *NewPHI = TII->duplicate(OldPHI, MF);
    MachineOperand *MO = &NewPHI->getOperand(0);
    assert(MO->isDef() && "NewPHI->Operand(0) should be a Def");
    unsigned Reg = MO->getReg();
    const TargetRegisterClass *RC = MRI.getRegClass(Reg);
    unsigned NewReg = MRI.createVirtualRegister(RC);
    MO->setReg(NewReg);
    // Remove operands of which the source BB is not in PredsOut from NewPHI.
    for (unsigned i = NewPHI->getNumOperands() - 1; i >= 2; i -= 2) {
      MO = &NewPHI->getOperand(i);
      if (!PredsOut.count(MO->getMBB())) {
        NewPHI->RemoveOperand(i);
        NewPHI->RemoveOperand(i - 1);
      }
    }
    // Remove operands of which the source BB is in PredsOut from OldPHI.
    for (unsigned i = OldPHI->getNumOperands() - 1; i >= 2; i -= 2) {
      MO = &OldPHI->getOperand(i);
      if (PredsOut.count(MO->getMBB())) {
        OldPHI->RemoveOperand(i);
        OldPHI->RemoveOperand(i - 1);
      }
    }
    MachineInstrBuilder MIB(MF, OldPHI);
    MIB.addReg(NewReg).addMBB(NMBB);
    NMBB->insert(NMBB->SkipPHIsAndLabels(NMBB->begin()), NewPHI);
  }

  // Update MDT. NMBB becomes the new immediate dominator node of Header.
  MDT->addNewBlock(NMBB, IDom->getBlock());
  MDT->changeImmediateDominator(Header, NMBB);

  // Update MLI.
  if (MachineLoop *P = Loop->getParentLoop())
    P->addBasicBlockToLoop(NMBB, MLI->getBase());
  return true;
}

bool SplitCritEdges::runOnMachineFunction(MachineFunction &MF) {
  MDT = &getAnalysis<MachineDominatorTree>();
  MLI = &getAnalysis<MachineLoopInfo>();

  SmallVector<MachineLoop *, 8> Worklist(MLI->begin(), MLI->end());
  while (!Worklist.empty()) {
    MachineLoop *CurLoop = Worklist.pop_back_val();

    MachineBasicBlock *Preheader = CurLoop->getLoopPreheader();
    if (!Preheader) {
      MachineBasicBlock *Pred = CurLoop->getLoopPredecessor();
      if (Pred && Pred->SplitCriticalEdge(CurLoop->getHeader(), this)) {
        DEBUG(dbgs() << "Split critical entry edge for " << *CurLoop);
        DEBUG(dbgs() << "BB#" << Pred->getNumber() << " --> "
                     << CurLoop->getHeader()->getNumber() << "\n");
      } else if (createLoopPreheader(MF, CurLoop)) {
        DEBUG(dbgs() << "Create a new loop header for " << *CurLoop);
        DEBUG(dbgs() << "new preheader BB#"
                     << CurLoop->getLoopPreheader()->getNumber() << "\n");
      }
    }

    SmallVector<MachineBasicBlock *, 8> ExitBlocks;
    SmallPtrSet<MachineBasicBlock *, 8> UniqExitBlocks;
    SmallPtrSet<MachineBasicBlock *, 8> PredBlocks;
    CurLoop->getExitBlocks(ExitBlocks);
    UniqExitBlocks.insert(ExitBlocks.begin(), ExitBlocks.end());
    for (auto ExitBlock : UniqExitBlocks) {
      bool ToSplit = false;
      for (auto PredBB : ExitBlock->predecessors()) {
        if (!CurLoop->contains(PredBB)) {
          ToSplit = true;
          break;
        }
      }
      if (ToSplit) {
        // When splitting critical edges, ExitBlock->predecessors() will
        // be changed, so save ExitBlock->predecessors() in PredBlocks
        // before doing any splitting.
        PredBlocks.insert(ExitBlock->predecessors().begin(),
                          ExitBlock->predecessors().end());
        for (auto PredBB : PredBlocks) {
          if (CurLoop->contains(PredBB) &&
              PredBB->SplitCriticalEdge(ExitBlock, this)) {
            DEBUG(dbgs() << "Split critical exit edge for " << *CurLoop);
            DEBUG(dbgs() << "BB#" << PredBB->getNumber() << " --> "
                         << ExitBlock->getNumber() << "\n");
          }
        }
        PredBlocks.clear();
      }
    }

    Worklist.append(CurLoop->begin(), CurLoop->end());
  }
  return false;
}
