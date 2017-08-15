//===-------------- PPCMIPeephole.cpp - MI Peephole Cleanups -------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------===//
//
// This pass performs peephole optimizations to clean up ugly code
// sequences at the MachineInstruction layer.  It runs at the end of
// the SSA phases, following VSX swap removal.  A pass of dead code
// elimination follows this one for quick clean-up of any dead
// instructions introduced here.  Although we could do this as callbacks
// from the generic peephole pass, this would have a couple of bad
// effects:  it might remove optimization opportunities for VSX swap
// removal, and it would miss cleanups made possible following VSX
// swap removal.
//
//===---------------------------------------------------------------------===//

#include "PPC.h"
#include "PPCInstrBuilder.h"
#include "PPCInstrInfo.h"
#include "PPCTargetMachine.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/CodeGen/MachineDominators.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/Support/Debug.h"

using namespace llvm;

#define DEBUG_TYPE "ppc-mi-peepholes"

STATISTIC(NumOptADDLIs, "Number of optimized ADD instruction fed by LI");

namespace llvm {
  void initializePPCMIPeepholePass(PassRegistry&);
}

namespace {

struct PPCMIPeephole : public MachineFunctionPass {

  static char ID;
  const PPCInstrInfo *TII;
  MachineFunction *MF;
  MachineRegisterInfo *MRI;

  PPCMIPeephole() : MachineFunctionPass(ID) {
    initializePPCMIPeepholePass(*PassRegistry::getPassRegistry());
  }

private:
  MachineDominatorTree *MDT;

  // Initialize class variables.
  void initialize(MachineFunction &MFParm);

  // Perform peepholes.
  bool simplifyCode(void);

  // Find the "true" register represented by SrcReg (following chains
  // of copies and subreg_to_reg operations).
  unsigned lookThruCopyLike(unsigned SrcReg);

public:

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<MachineDominatorTree>();
    MachineFunctionPass::getAnalysisUsage(AU);
  }

  // Main entry point for this pass.
  bool runOnMachineFunction(MachineFunction &MF) override {
    MDT = &getAnalysis<MachineDominatorTree>();
    if (skipFunction(*MF.getFunction()))
      return false;
    initialize(MF);
    return simplifyCode();
  }
};

// Initialize class variables.
void PPCMIPeephole::initialize(MachineFunction &MFParm) {
  MF = &MFParm;
  MRI = &MF->getRegInfo();
  TII = MF->getSubtarget<PPCSubtarget>().getInstrInfo();
  DEBUG(dbgs() << "*** PowerPC MI peephole pass ***\n\n");
  DEBUG(MF->dump());
}

// Perform peephole optimizations.
bool PPCMIPeephole::simplifyCode(void) {
  bool Simplified = false;
  MachineInstr* ToErase = nullptr;

  for (MachineBasicBlock &MBB : *MF) {
    for (MachineInstr &MI : MBB) {

      // If the previous instruction was marked for elimination,
      // remove it now.
      if (ToErase) {
        ToErase->eraseFromParent();
        ToErase = nullptr;
      }

      // Ignore debug instructions.
      if (MI.isDebugValue())
        continue;

      // Per-opcode peepholes.
      switch (MI.getOpcode()) {

      default:
        break;

      case PPC::XXPERMDI: {
        // Perform simplifications of 2x64 vector swaps and splats.
        // A swap is identified by an immediate value of 2, and a splat
        // is identified by an immediate value of 0 or 3.
        int Immed = MI.getOperand(3).getImm();

        if (Immed != 1) {

          // For each of these simplifications, we need the two source
          // regs to match.  Unfortunately, MachineCSE ignores COPY and
          // SUBREG_TO_REG, so for example we can see
          //   XXPERMDI t, SUBREG_TO_REG(s), SUBREG_TO_REG(s), immed.
          // We have to look through chains of COPY and SUBREG_TO_REG
          // to find the real source values for comparison.
          unsigned TrueReg1 = lookThruCopyLike(MI.getOperand(1).getReg());
          unsigned TrueReg2 = lookThruCopyLike(MI.getOperand(2).getReg());

          if (TrueReg1 == TrueReg2
              && TargetRegisterInfo::isVirtualRegister(TrueReg1)) {
            MachineInstr *DefMI = MRI->getVRegDef(TrueReg1);
            unsigned DefOpc = DefMI ? DefMI->getOpcode() : 0;

            // If this is a splat fed by a splatting load, the splat is
            // redundant. Replace with a copy. This doesn't happen directly due
            // to code in PPCDAGToDAGISel.cpp, but it can happen when converting
            // a load of a double to a vector of 64-bit integers.
            auto isConversionOfLoadAndSplat = [=]() -> bool {
              if (DefOpc != PPC::XVCVDPSXDS && DefOpc != PPC::XVCVDPUXDS)
                return false;
              unsigned DefReg = lookThruCopyLike(DefMI->getOperand(1).getReg());
              if (TargetRegisterInfo::isVirtualRegister(DefReg)) {
                MachineInstr *LoadMI = MRI->getVRegDef(DefReg);
                if (LoadMI && LoadMI->getOpcode() == PPC::LXVDSX)
                  return true;
              }
              return false;
            };
            if (DefMI && (Immed == 0 || Immed == 3)) {
              if (DefOpc == PPC::LXVDSX || isConversionOfLoadAndSplat()) {
                DEBUG(dbgs()
                      << "Optimizing load-and-splat/splat "
                      "to load-and-splat/copy: ");
                DEBUG(MI.dump());
                BuildMI(MBB, &MI, MI.getDebugLoc(), TII->get(PPC::COPY),
                        MI.getOperand(0).getReg())
                    .add(MI.getOperand(1));
                ToErase = &MI;
                Simplified = true;
              }
            }

            // If this is a splat or a swap fed by another splat, we
            // can replace it with a copy.
            if (DefOpc == PPC::XXPERMDI) {
              unsigned FeedImmed = DefMI->getOperand(3).getImm();
              unsigned FeedReg1
                = lookThruCopyLike(DefMI->getOperand(1).getReg());
              unsigned FeedReg2
                = lookThruCopyLike(DefMI->getOperand(2).getReg());

              if ((FeedImmed == 0 || FeedImmed == 3) && FeedReg1 == FeedReg2) {
                DEBUG(dbgs()
                      << "Optimizing splat/swap or splat/splat "
                      "to splat/copy: ");
                DEBUG(MI.dump());
                BuildMI(MBB, &MI, MI.getDebugLoc(), TII->get(PPC::COPY),
                        MI.getOperand(0).getReg())
                    .add(MI.getOperand(1));
                ToErase = &MI;
                Simplified = true;
              }

              // If this is a splat fed by a swap, we can simplify modify
              // the splat to splat the other value from the swap's input
              // parameter.
              else if ((Immed == 0 || Immed == 3)
                       && FeedImmed == 2 && FeedReg1 == FeedReg2) {
                DEBUG(dbgs() << "Optimizing swap/splat => splat: ");
                DEBUG(MI.dump());
                MI.getOperand(1).setReg(DefMI->getOperand(1).getReg());
                MI.getOperand(2).setReg(DefMI->getOperand(2).getReg());
                MI.getOperand(3).setImm(3 - Immed);
                Simplified = true;
              }

              // If this is a swap fed by a swap, we can replace it
              // with a copy from the first swap's input.
              else if (Immed == 2 && FeedImmed == 2 && FeedReg1 == FeedReg2) {
                DEBUG(dbgs() << "Optimizing swap/swap => copy: ");
                DEBUG(MI.dump());
                BuildMI(MBB, &MI, MI.getDebugLoc(), TII->get(PPC::COPY),
                        MI.getOperand(0).getReg())
                    .add(DefMI->getOperand(1));
                ToErase = &MI;
                Simplified = true;
              }
            } else if ((Immed == 0 || Immed == 3) && DefOpc == PPC::XXPERMDIs &&
                       (DefMI->getOperand(2).getImm() == 0 ||
                        DefMI->getOperand(2).getImm() == 3)) {
              // Splat fed by another splat - switch the output of the first
              // and remove the second.
              DefMI->getOperand(0).setReg(MI.getOperand(0).getReg());
              ToErase = &MI;
              Simplified = true;
              DEBUG(dbgs() << "Removing redundant splat: ");
              DEBUG(MI.dump());
            }
          }
        }
        break;
      }
      case PPC::VSPLTB:
      case PPC::VSPLTH:
      case PPC::XXSPLTW: {
        unsigned MyOpcode = MI.getOpcode();
        unsigned OpNo = MyOpcode == PPC::XXSPLTW ? 1 : 2;
        unsigned TrueReg = lookThruCopyLike(MI.getOperand(OpNo).getReg());
        if (!TargetRegisterInfo::isVirtualRegister(TrueReg))
          break;
        MachineInstr *DefMI = MRI->getVRegDef(TrueReg);
        if (!DefMI)
          break;
        unsigned DefOpcode = DefMI->getOpcode();
        auto isConvertOfSplat = [=]() -> bool {
          if (DefOpcode != PPC::XVCVSPSXWS && DefOpcode != PPC::XVCVSPUXWS)
            return false;
          unsigned ConvReg = DefMI->getOperand(1).getReg();
          if (!TargetRegisterInfo::isVirtualRegister(ConvReg))
            return false;
          MachineInstr *Splt = MRI->getVRegDef(ConvReg);
          return Splt && (Splt->getOpcode() == PPC::LXVWSX ||
            Splt->getOpcode() == PPC::XXSPLTW);
        };
        bool AlreadySplat = (MyOpcode == DefOpcode) ||
          (MyOpcode == PPC::VSPLTB && DefOpcode == PPC::VSPLTBs) ||
          (MyOpcode == PPC::VSPLTH && DefOpcode == PPC::VSPLTHs) ||
          (MyOpcode == PPC::XXSPLTW && DefOpcode == PPC::XXSPLTWs) ||
          (MyOpcode == PPC::XXSPLTW && DefOpcode == PPC::LXVWSX) ||
          (MyOpcode == PPC::XXSPLTW && DefOpcode == PPC::MTVSRWS)||
          (MyOpcode == PPC::XXSPLTW && isConvertOfSplat());
        // If the instruction[s] that feed this splat have already splat
        // the value, this splat is redundant.
        if (AlreadySplat) {
          DEBUG(dbgs() << "Changing redundant splat to a copy: ");
          DEBUG(MI.dump());
          BuildMI(MBB, &MI, MI.getDebugLoc(), TII->get(PPC::COPY),
                  MI.getOperand(0).getReg())
              .add(MI.getOperand(OpNo));
          ToErase = &MI;
          Simplified = true;
        }
        // Splat fed by a shift. Usually when we align value to splat into
        // vector element zero.
        if (DefOpcode == PPC::XXSLDWI) {
          unsigned ShiftRes = DefMI->getOperand(0).getReg();
          unsigned ShiftOp1 = DefMI->getOperand(1).getReg();
          unsigned ShiftOp2 = DefMI->getOperand(2).getReg();
          unsigned ShiftImm = DefMI->getOperand(3).getImm();
          unsigned SplatImm = MI.getOperand(2).getImm();
          if (ShiftOp1 == ShiftOp2) {
            unsigned NewElem = (SplatImm + ShiftImm) & 0x3;
            if (MRI->hasOneNonDBGUse(ShiftRes)) {
              DEBUG(dbgs() << "Removing redundant shift: ");
              DEBUG(DefMI->dump());
              ToErase = DefMI;
            }
            Simplified = true;
            DEBUG(dbgs() << "Changing splat immediate from " << SplatImm <<
                  " to " << NewElem << " in instruction: ");
            DEBUG(MI.dump());
            MI.getOperand(1).setReg(ShiftOp1);
            MI.getOperand(2).setImm(NewElem);
          }
        }
        break;
      }
      case PPC::XVCVDPSP: {
        // If this is a DP->SP conversion fed by an FRSP, the FRSP is redundant.
        unsigned TrueReg = lookThruCopyLike(MI.getOperand(1).getReg());
        if (!TargetRegisterInfo::isVirtualRegister(TrueReg))
          break;
        MachineInstr *DefMI = MRI->getVRegDef(TrueReg);

        // This can occur when building a vector of single precision or integer
        // values.
        if (DefMI && DefMI->getOpcode() == PPC::XXPERMDI) {
          unsigned DefsReg1 = lookThruCopyLike(DefMI->getOperand(1).getReg());
          unsigned DefsReg2 = lookThruCopyLike(DefMI->getOperand(2).getReg());
          if (!TargetRegisterInfo::isVirtualRegister(DefsReg1) ||
              !TargetRegisterInfo::isVirtualRegister(DefsReg2))
            break;
          MachineInstr *P1 = MRI->getVRegDef(DefsReg1);
          MachineInstr *P2 = MRI->getVRegDef(DefsReg2);

          if (!P1 || !P2)
            break;

          // Remove the passed FRSP instruction if it only feeds this MI and
          // set any uses of that FRSP (in this MI) to the source of the FRSP.
          auto removeFRSPIfPossible = [&](MachineInstr *RoundInstr) {
            if (RoundInstr->getOpcode() == PPC::FRSP &&
                MRI->hasOneNonDBGUse(RoundInstr->getOperand(0).getReg())) {
              Simplified = true;
              unsigned ConvReg1 = RoundInstr->getOperand(1).getReg();
              unsigned FRSPDefines = RoundInstr->getOperand(0).getReg();
              MachineInstr &Use = *(MRI->use_instr_begin(FRSPDefines));
              for (int i = 0, e = Use.getNumOperands(); i < e; ++i)
                if (Use.getOperand(i).isReg() &&
                    Use.getOperand(i).getReg() == FRSPDefines)
                  Use.getOperand(i).setReg(ConvReg1);
              DEBUG(dbgs() << "Removing redundant FRSP:\n");
              DEBUG(RoundInstr->dump());
              DEBUG(dbgs() << "As it feeds instruction:\n");
              DEBUG(MI.dump());
              DEBUG(dbgs() << "Through instruction:\n");
              DEBUG(DefMI->dump());
              RoundInstr->eraseFromParent();
            }
          };

          // If the input to XVCVDPSP is a vector that was built (even
          // partially) out of FRSP's, the FRSP(s) can safely be removed
          // since this instruction performs the same operation.
          if (P1 != P2) {
            removeFRSPIfPossible(P1);
            removeFRSPIfPossible(P2);
            break;
          }
          removeFRSPIfPossible(P1);
        }
        break;
      }

      case PPC::ADD4:
      case PPC::ADD8: {
        auto getVRegDefMI = [&](MachineOperand *Op, MachineRegisterInfo *MRI) {
          assert(Op && "Invalid Operand!");
          if (!Op->isReg())
            return (MachineInstr *)nullptr;

          unsigned Reg = Op->getReg();
          if (!TargetRegisterInfo::isVirtualRegister(Reg))
            return (MachineInstr *)nullptr;

          return MRI->getVRegDef(Reg);
        };

        auto replaceLiWithAddi = [&](MachineOperand *DominatorOp,
                                     MachineOperand *PhiOp) {
          assert(PhiOp && DominatorOp && "Invalid Operand!");
          MachineInstr *DefPhiMI = getVRegDefMI(PhiOp, MRI);
          MachineInstr *DefDomMI = getVRegDefMI(DominatorOp, MRI);
          if (!DefPhiMI || !DefDomMI)
            return false;

          if (DefPhiMI->getOpcode() != PPC::PHI)
            return false;

          if (!MRI->hasOneNonDBGUse(DefPhiMI->getOperand(0).getReg()))
            return false;

          // Note: the vregs only show up at odd indices position of PHI Node,
          // the even indices position save the BB info.
          for (unsigned i = 1; i < DefPhiMI->getNumOperands(); i += 2) {
            MachineInstr *LiMI = getVRegDefMI(&DefPhiMI->getOperand(i), MRI);
            if (!LiMI || !MRI->hasOneNonDBGUse(LiMI->getOperand(0).getReg()) ||
                !MDT->dominates(DefDomMI, LiMI) ||
                (LiMI->getOpcode() != PPC::LI && LiMI->getOpcode() != PPC::LI8))
              return false;
          }

          // Note: we already known DominatorOp is virtual register above
          unsigned DominatorReg = DominatorOp->getReg();

          const TargetRegisterClass *TRC =
              MI.getOpcode() == PPC::ADD8 ? &PPC::G8RC_and_G8RC_NOX0RegClass
                                          : &PPC::GPRC_and_GPRC_NOR0RegClass;
          MRI->setRegClass(DominatorReg, TRC);

          // replace LIs with ADDIs
          for (unsigned i = 1; i < DefPhiMI->getNumOperands(); i += 2) {
            MachineInstr *LiMI = getVRegDefMI(&DefPhiMI->getOperand(i), MRI);
            DEBUG(dbgs() << "Optimizing LI to ADDI: ");
            DEBUG(LiMI->dump());

            // There could be repeated registers in the PHI, e.g: %vreg1<def> =
            // PHI %vreg6, <BB#2>, %vreg8, <BB#3>, %vreg8, <BB#6>; In such case,
            // only replace the first one (in this specific example just replace
            // the first %vreg8 from <BB#3>
            if (LiMI->getOpcode() == PPC::ADDI ||
                LiMI->getOpcode() == PPC::ADDI8)
              continue;

            assert((LiMI->getOpcode() == PPC::LI ||
                   LiMI->getOpcode() == PPC::LI8) && "Invalid Opcode!");
            auto LiImm = LiMI->getOperand(1).getImm(); // save the imm of LI
            LiMI->RemoveOperand(1);                    // remove the imm of LI
            LiMI->setDesc(TII->get(LiMI->getOpcode() == PPC::LI ? PPC::ADDI
                                                                : PPC::ADDI8));
            MachineInstrBuilder(*LiMI->getParent()->getParent(), *LiMI)
                .addReg(DominatorReg)
                .addImm(LiImm); // restore the imm of LI
            DEBUG(LiMI->dump()); // LiMI is actually AddiMI now
          }

          return true;
        };

        auto replaceAddWithCopy = [&](MachineOperand &PhiOp) {
          DEBUG(dbgs() << "Optimizing ADD to COPY: ");
          DEBUG(MI.dump());
          BuildMI(MBB, &MI, MI.getDebugLoc(), TII->get(PPC::COPY),
                  MI.getOperand(0).getReg())
              .add(PhiOp);
          ToErase = &MI;
          Simplified = true;
          NumOptADDLIs++;
        };

        MachineOperand Op1 = MI.getOperand(1);
        MachineOperand Op2 = MI.getOperand(2);
        if (replaceLiWithAddi(&Op2, &Op1))
          replaceAddWithCopy(Op1);
        else if (replaceLiWithAddi(&Op1, &Op2))
          replaceAddWithCopy(Op2);
        break;
      }
      }
    }
    // If the last instruction was marked for elimination,
    // remove it now.
    if (ToErase) {
      ToErase->eraseFromParent();
      ToErase = nullptr;
    }
  }

  return Simplified;
}

// This is used to find the "true" source register for an
// XXPERMDI instruction, since MachineCSE does not handle the
// "copy-like" operations (Copy and SubregToReg).  Returns
// the original SrcReg unless it is the target of a copy-like
// operation, in which case we chain backwards through all
// such operations to the ultimate source register.  If a
// physical register is encountered, we stop the search.
unsigned PPCMIPeephole::lookThruCopyLike(unsigned SrcReg) {

  while (true) {

    MachineInstr *MI = MRI->getVRegDef(SrcReg);
    if (!MI->isCopyLike())
      return SrcReg;

    unsigned CopySrcReg;
    if (MI->isCopy())
      CopySrcReg = MI->getOperand(1).getReg();
    else {
      assert(MI->isSubregToReg() && "bad opcode for lookThruCopyLike");
      CopySrcReg = MI->getOperand(2).getReg();
    }

    if (!TargetRegisterInfo::isVirtualRegister(CopySrcReg))
      return CopySrcReg;

    SrcReg = CopySrcReg;
  }
}

} // end default namespace

INITIALIZE_PASS_BEGIN(PPCMIPeephole, DEBUG_TYPE,
                      "PowerPC MI Peephole Optimization", false, false)
INITIALIZE_PASS_END(PPCMIPeephole, DEBUG_TYPE,
                    "PowerPC MI Peephole Optimization", false, false)

char PPCMIPeephole::ID = 0;
FunctionPass*
llvm::createPPCMIPeepholePass() { return new PPCMIPeephole(); }

