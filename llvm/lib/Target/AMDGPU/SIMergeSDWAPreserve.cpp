//===-- SIMergeSDWAPreserve.cpp - Merge SDWA dst_unused:PRESERVE operand --===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
/// \file This pass merges SDWA dst_unused:PRESERVE operand
///
/// E.g. original:
///   %vreg2 = V_ADD_F16 %vreg0, %vreg1
///   %vreg3 = V_ADD_F16_sdwa %vreg0, %vreg1
///     dst_sel:WORD_1 dst_unused:UNUSED_RESERVE src0_sel:WORD_1 src1_sel:WORD_1
///     %vreg2<imp-use>
///
/// Replace:
///   %vreg2 = V_ADD_F16 %vreg0, %vreg1
///   %vreg2 = V_ADD_F16_sdwa %vreg0, %vreg1
///     dst_sel:WORD_1 dst_unused:UNUSED_RESERVE src0_sel:WORD_1 src1_sel:WORD_1
///     %vreg3<imp-use>
///   %vreg3 = V_MOV_B32 %vreg2
///
/// This pass should be placed after PHI elimination because it can't be
/// implemented in SSA form
//
//===----------------------------------------------------------------------===//

#include "AMDGPU.h"
#include "AMDGPUSubtarget.h"
#include "SIDefines.h"
#include "SIInstrInfo.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"

using namespace llvm;

#define DEBUG_TYPE "si-merge-sdwa-preserve"

STATISTIC(NumMergeSDWAPreserve,
          "Number of merges of SDWA dst_unused:PRESERVE.");

namespace {

  class SIMergeSDWAPreserve : public MachineFunctionPass {
  public:
    static char ID;

    SIMergeSDWAPreserve() : MachineFunctionPass(ID) {
      initializeSIMergeSDWAPreservePass(*PassRegistry::getPassRegistry());
    }

    bool runOnMachineFunction(MachineFunction &MF) override;

    StringRef getPassName() const override { return "SI Merge SDWA Preserve"; }

    void getAnalysisUsage(AnalysisUsage &AU) const override {
      // Should preserve the same set that TwoAddressInstructions does.
      AU.addPreserved<SlotIndexes>();
      AU.addPreserved<LiveIntervals>();
      AU.addPreservedID(LiveVariablesID);
      AU.addPreservedID(MachineLoopInfoID);
      AU.addPreservedID(MachineDominatorsID);
      AU.setPreservesCFG();
      MachineFunctionPass::getAnalysisUsage(AU);
    }
  };

} // End anonymous namespace.

INITIALIZE_PASS(SIMergeSDWAPreserve, DEBUG_TYPE, "SI Merge SDWA Preserve", false, false)

char SIMergeSDWAPreserve::ID = 0;

char &llvm::SIMergeSDWAPreserveID = SIMergeSDWAPreserve::ID;

FunctionPass *llvm::createSIMergeSDWAPreservePass() {
  return new SIMergeSDWAPreserve();
}

bool SIMergeSDWAPreserve::runOnMachineFunction(MachineFunction &MF) {
  LiveIntervals *LIS = getAnalysisIfAvailable<LiveIntervals>();

  const SISubtarget &ST = MF.getSubtarget<SISubtarget>();

  if (!ST.hasSDWA())
    return false;

  bool Merged = false;

  const MachineRegisterInfo *MRI = &MF.getRegInfo();
  const SIRegisterInfo *TRI = ST.getRegisterInfo();
  const SIInstrInfo *TII = ST.getInstrInfo();


  for (MachineBasicBlock &MBB : MF) {
    for (MachineInstr &MI : MBB) {
      if (!TII->isSDWA(MI))
        continue;

      auto *DstUnused = TII->getNamedOperand(MI, AMDGPU::OpName::dst_unused);

      if (!DstUnused || DstUnused->getImm() != AMDGPU::SDWA::UNUSED_PRESERVE)
        continue;

      // VOPC doesn't support PRESERVE
      assert(!TII->isVOPC(AMDGPU::getBasicFromSDWAOp(MI.getOpcode())));

      // Operand to copy should be last implicit operand in SDWA with
      // preserve instruction
      auto &SrcReg = MI.getOperand(MI.getNumOperands() - 1);
      if (!SrcReg.isImplicit() || !SrcReg.isUse() ||
          !TRI->isVirtualRegister(SrcReg.getReg()) ||
          !TRI->hasVGPRs(TII->getOpRegClass(MI, MI.getNumOperands() - 1)))
        continue;


      auto *DstReg = TII->getNamedOperand(MI, AMDGPU::OpName::vdst);
      assert(DstReg &&
             DstReg->isReg() &&
             TRI->hasVGPRs(TII->getOpRegClass(MI, MI.getOperandNo(DstReg))));

      // Check that MI is the only instruction that uses src register
      for (MachineOperand &UseMO : MRI->use_operands(SrcReg.getReg())) {
        if (TRI->isSubregOf(SrcReg, UseMO)) {
          // TODO: in that case we should unfold this SDWA-with-PRESERVE back
          // into v_or_b32 + SDWA-without-PRESERVE
          assert(UseMO.getParent() == &MI);
        }
      }

      MachineInstrBuilder Copy = BuildMI(MBB, MI.getNextNode(), MI.getDebugLoc(),
                                         TII->get(AMDGPU::COPY),
                                         DstReg->getReg())
                                 .addReg(SrcReg.getReg());

      DstReg->setReg(SrcReg.getReg());

      if (LIS)
        LIS->repairIntervalsInRange(&MBB, &MI, Copy.getInstr(),
                                    { SrcReg.getReg(), DstReg->getReg() });

      ++NumMergeSDWAPreserve;
      Merged = true;
    }
  }

  return Merged;
}
