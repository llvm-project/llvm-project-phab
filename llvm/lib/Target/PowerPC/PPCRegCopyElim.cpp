//===----- PPCRegCopyElim.cpp -- Redundant Register Copy Elimination -----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------===//
//
// This pass aims to eliminate redundancy related to physical registers
// after the register allocation.
// So far, we eliminate the following two types of redundant register copys.
//
// 1) intra-BB redundant register copy
//   li Y, 0          li X, 0
//   mr X, Y    =>    (erase mr)
//   ..               ..
// 2) inter-BB partially redundant register copy
//            BB1--------                             BB1--------
//            | ..      |                             | ..      |
//            | mr Y, X |                             | (erase) |
//            | ..      |                             | ..      |
//    with    -----------                    with     -----------
//    1 pred /     |                         1 pred  /     |
//  BB--------     |      BB--------      BB---------      |      BB---------
//  | ..     |     |      | ..     |  =>  | mr Y, X |      |      | ..      |
//  | ..     |     |      | ..     |      | ..      |      |      | mr X, Y |
//  ----------     |      ----------      -----------      |      -----------
//                 |     /  with                           |     /  with
//            BB2--------   1 succ                    BB2--------   1 succ
//            | ..      |                             | ..      |
//            | mr X, Y |                             | (erase) |
//            | ..      |                             | ..      |
//            -----------                             -----------
//
//===---------------------------------------------------------------------===//

#include "PPCTargetMachine.h"

using namespace llvm;

#define DEBUG_TYPE "ppc-regcopy-elim"

namespace {

struct PPCRegCopyElim : public MachineFunctionPass {

  static char ID;
  const PPCInstrInfo *TII;
  MachineFunction *MF;

  PPCRegCopyElim() : MachineFunctionPass(ID) {
    initializePPCRegCopyElimPass(*PassRegistry::getPassRegistry());
  }

private:
  // Initialize class variables.
  void initialize(MachineFunction &MFParm);

  // Perform peepholes.
  bool eliminateRedundantRegCopy(void);

public:
  // Main entry point for this pass.
  bool runOnMachineFunction(MachineFunction &MF) override {
    if (skipFunction(*MF.getFunction()))
      return false;
    initialize(MF);
    bool Simplified = false;

    Simplified |= eliminateRedundantRegCopy();
    return Simplified;
  }
};

// Initialize class variables.
void PPCRegCopyElim::initialize(MachineFunction &MFParm) {
  MF = &MFParm;
  TII = MF->getSubtarget<PPCSubtarget>().getInstrInfo();
  DEBUG(dbgs() <<
        "*** PowerPC Redundant Register Copy Elimination pass ***\n\n");
}

// If MI is a register copy, this method returns true and set
// source and destination operands in SrcOperand and DstOperand.
// When isKill is true, this method returns true only if the src operand
// has kill flag set.
static bool isRegCopy(MachineInstr &MI, MachineOperand* &SrcOperand,
                      MachineOperand* &DstOperand, bool isKill = false) {
  // Refer PPCInstrInfo::copyPhysReg to find opcodes used for copying
  // the value in a physical register for each register class.
  // Currently we do not optimize VSX registers (copied by xxlor),
  // which may involve different register types, i.e. FPR and VRF.
  unsigned Opc = MI.getOpcode();
  if (Opc == PPC::FMR) {
    if (isKill && !MI.getOperand(1).isKill())
      return false;
    DstOperand = &MI.getOperand(0);
    SrcOperand = &MI.getOperand(1);
    return true;
  }
  if ((Opc == PPC::OR8 || Opc == PPC::OR || Opc == PPC::VOR) &&
      MI.getOperand(1).getReg() == MI.getOperand(2).getReg()) {
    if (isKill && !(MI.getOperand(1).isKill() || MI.getOperand(2).isKill()))
      return false;
    DstOperand = &MI.getOperand(0);
    SrcOperand = &MI.getOperand(1);
    return true;
  }
  return false;
}

bool PPCRegCopyElim::eliminateRedundantRegCopy(void) {
  const PPCSubtarget *PPCSubTarget = &MF->getSubtarget<PPCSubtarget>();
  const TargetRegisterInfo *TRI = PPCSubTarget->getRegisterInfo();
  bool Simplified = false;
  for (MachineBasicBlock &MBB : *MF) {
    bool CurrentInstrErased = false;
    do {
      CurrentInstrErased = false;
      for (MachineInstr &CopyMI : MBB) {
        if (CopyMI.isDebugValue())
          continue;

        MachineOperand *SrcMO = nullptr, *DstMO = nullptr;
        if (isRegCopy(CopyMI, SrcMO, DstMO, true)) {
          unsigned DstReg = DstMO->getReg();
          unsigned SrcReg = SrcMO->getReg();

          MachineInstr *DefMI = nullptr;
          bool Found = false;
          SmallVector<MachineOperand*, 8> OperandsToRewrite1;

          // We iterate instructions backward to find the def of the source
          // of register copy or instructions conflicting with register copy
          // (e.g. instructions that use destination reg of copy).
          MachineBasicBlock::reverse_iterator MBBRI = CopyMI,
                                              MBBRIE = MBB.rend();
          for (MBBRI++; MBBRI != MBBRIE && !Found; MBBRI++) {
            // We conservatively assume function call may crobber all regs.
            if (MBBRI->isCall()) {
              Found = true;
              break;
            }

            // If this instruction defines source reg of copy, this is the
            // target for our optimization.
            for (auto DefMO: MBBRI->defs())
              if (TRI->isSuperOrSubRegisterEq(DefMO.getReg(), SrcReg)) {
                if (!DefMO.isTied() && !DefMO.isImplicit())
                  DefMI = &(*MBBRI);
                Found = true;
                break;
              }
            if (Found)
              break;

            // If this instr conflicts with the reg copy, we give up here.
            for (auto MO: MBBRI->operands())
              if (MO.isReg() &&
                  TRI->isSuperOrSubRegisterEq(MO.getReg(), DstReg)) {
                Found = true;
                break;
              }

            // If this instruction uses source register of register copy,
            // we need to remember the operand to replace it with destination
            // register of copy. For example:
            //   li r4, 1            li r3, 1
            //   cmplwi r4, 0   ->   cmplwi r3, 0
            //   mr r3, r4           (erase mr)
            for (unsigned I = 0; I < MBBRI->getNumOperands(); I++) {
              MachineOperand *MO = &MBBRI->getOperand(I);
              if (!(MO->isReg() && MO->isUse()))
                continue;
              if (MO->getReg() == SrcReg)
                OperandsToRewrite1.push_back(MO);
              else if (TRI->isSuperOrSubRegisterEq(MO->getReg(), SrcReg)) {
                Found = true;
                break;
              }
            }
          }

          // Def for the source of the register copy is found in the BB
          // and can be optimized within this block.
          if (DefMI && DefMI->getOperand(0).getReg() == SrcReg) {
            DEBUG(dbgs()
                  << "Eliminating redundant register copy:\n");
            DEBUG(DefMI->dump());
            DEBUG(CopyMI.dump());
            DefMI->getOperand(0).setReg(DstReg);
            CopyMI.eraseFromParent();
            for (auto &MO: OperandsToRewrite1)
              MO->setReg(DstReg);
            CurrentInstrErased = true;
            break;
          }

          // If there is conflicting instruction (e.g. uses source reg of copy)
          // between def and reg copy, we cannot optimize.
          if (Found)
            continue;

          // If no def and no conflicting instruction is found in this BB,
          // we check the predecessors.
          // So far, we do not optimize if this BB has more than two
          // predecessors not to increase the number of total instructions.
          if (MBB.pred_size() != 2) continue;

          // From here, we handle redundancy among multiple BBs.
          // We currently support following CFGs.
          //
          //         Pred1MBB
          //          /  |    /
          //   (SideMBB) | Pred2MBB
          //        /    |  /
          //           MBB (including CopyMI)
          //
          // - MBB doesn't have more than two predecessors.
          // - Pred1MBB doesn't have more than two successors.
          // - SideMBB is optional.
          // - SideMBB has only one predecessor (Pred1MBB).
          // - Pred2MBB has only one successor (MBB).
          // - SideMBB and Pred2MBB may be the same BB.
          auto FirstPredBB = MBB.pred_begin();
          MachineBasicBlock *Pred2MBB = nullptr, *SideMBB = nullptr;
          for (auto &Pred1MBB: MBB.predecessors()) {
            // If the BBs does not match the above patterns we support,
            // we give up before iterating instructions.
            Pred2MBB = (Pred1MBB == *FirstPredBB) ? *(FirstPredBB + 1):
                                                    * FirstPredBB;
            if (Pred1MBB->succ_size() > 2 || Pred2MBB->succ_size() > 1)
              continue;

            if (Pred1MBB->succ_size() == 2) {
              auto FirstSuccBB = Pred1MBB->succ_begin();
              SideMBB = (&MBB == *FirstSuccBB) ? *(FirstSuccBB + 1):
                                                 * FirstSuccBB;
              if (SideMBB->pred_size() > 1)
                continue;
            }

            MachineInstr *DefMI = nullptr;
            bool Found = false;
            SmallVector<MachineOperand*, 8> OperandsToRewrite2;
            // As we did for the current MBB, we iterate instructions backward
            // to find the def of the source of register copy, starting from
            // the end of the BB.
            MachineBasicBlock::reverse_iterator MBBRI = Pred1MBB->rbegin(),
                                                MBBRIE = Pred1MBB->rend();
            for (; MBBRI != MBBRIE && !Found; MBBRI++) {
              if (MBBRI->isCall()) {
                Found = true;
                break;
              }

              // If this instruction defines source reg of copy, this is the
              // target for our optimization.
              for (auto DefMO: MBBRI->defs())
                if (TRI->isSuperOrSubRegisterEq(DefMO.getReg(), SrcReg)) {
                  if (!DefMO.isTied() && !DefMO.isImplicit())
                    DefMI = &(*MBBRI);
                  Found = true;
                  break;
                }
              if (Found)
                break;

              // If this instruction conflicts with the reg copy, we give up.
              for (auto DefMO: MBBRI->defs())
                if (TRI->isSuperOrSubRegisterEq(DefMO.getReg(), DstReg)) {
                  Found = true;
                  break;
                }

              for (unsigned I = 0; I < MBBRI->getNumOperands(); I++) {
                MachineOperand *MO = &MBBRI->getOperand(I);
                if (!(MO->isReg() && MO->isUse()))
                  continue;
                if (MO->getReg() == SrcReg)
                  OperandsToRewrite2.push_back(MO);
                else if (TRI->isSuperOrSubRegisterEq(MO->getReg(), SrcReg)) {
                  Found = true;
                  break;
                }
              }
            }
            // We keep going iff the def of the reg copy is another reg copy
            // with reversed src and dst.
            if (!(DefMI && isRegCopy(*DefMI, SrcMO, DstMO) &&
                  DefMI->getOpcode() == CopyMI.getOpcode() &&
                  SrcReg == DstMO->getReg() && DstReg == SrcMO->getReg()))
              continue;

            DEBUG(dbgs()
                  << "Optimizing partially redundant reg copy pair:\n");
            DEBUG(DefMI->dump());
            DEBUG(CopyMI.dump());

            // Here, we find a partially redundant register copy pair.
            // If we have SideMBB, we copy the first reg copy to the top of it.
            // Otherwise, we can erase it.
            if (SideMBB) {
              SideMBB->splice(SideMBB->getFirstNonDebugInstr(), Pred1MBB,
                              MachineBasicBlock::iterator(DefMI));
              SideMBB->removeLiveIn(SrcReg);
              SideMBB->addLiveIn(DstReg);
            }
            else
              DefMI->eraseFromParent();

            // We move the second reg copy to Pred2MBB by inserting before
            // the first terminator instructions (or at the end if no
            // terminator in the BB).
            MachineBasicBlock::iterator To = Pred2MBB->getFirstTerminator();
            if (To == Pred2MBB->end())
              Pred2MBB->push_back(MBB.remove(&CopyMI));
            else
              Pred2MBB->splice(To, &MBB, MachineBasicBlock::iterator(CopyMI));
            MBB.removeLiveIn(SrcReg);
            MBB.addLiveIn(DstReg);

            // We touch up some operands if we have found any.
            for (auto &MO: OperandsToRewrite1)
              MO->setReg(DstReg);
            for (auto &MO: OperandsToRewrite2)
              MO->setReg(DstReg);

            CurrentInstrErased = true;
            break;
          }

          Simplified = CurrentInstrErased;
          // If the current instruction has been eliminated,
          // we cannot continue iteration, so restart it.
          if (CurrentInstrErased)
            break;
        }
      }
    } while(CurrentInstrErased);
  }
  return Simplified;
}

} // end default namespace

INITIALIZE_PASS_BEGIN(PPCRegCopyElim, DEBUG_TYPE,
                   "PowerPC Redundant Register Copy Elimination", false, false)
INITIALIZE_PASS_END(PPCRegCopyElim, DEBUG_TYPE,
                   "PowerPC Redundant Register Copy Elimination", false, false)

char PPCRegCopyElim::ID = 0;
FunctionPass*
llvm::createPPCRegCopyElimPass() { return new PPCRegCopyElim(); }
