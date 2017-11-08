//===--------- PPCPreEmitPeephole.cpp - Late peephole optimizations -------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// A pre-emit peephole for catching opportunities introduced by late passes such
// as MachineBlockPlacement.
//
//===----------------------------------------------------------------------===//

#include "PPC.h"
#include "PPCInstrInfo.h"
#include "PPCSubtarget.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/CodeGen/LivePhysRegs.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/Debug.h"

using namespace llvm;

#define DEBUG_TYPE "ppc-pre-emit-peephole"

STATISTIC(NumRRConvertedInPreEmit,
          "Number of r+r instructions converted to r+i in pre-emit peephole");
STATISTIC(NumRemovedInPreEmit,
          "Number of instructions deleted in pre-emit peephole");
STATISTIC(NumRedundantPairsFound,
          "Number of redundant pair instructions found in pre-emit peephole");

static cl::opt<bool>
RunPreEmitPeephole("ppc-late-peephole", cl::Hidden, cl::init(false),
                   cl::desc("Run pre-emit peephole optimizations."));

namespace {
  class PPCPreEmitPeephole : public MachineFunctionPass {
    MachineInstr *hasRedundantPair(MachineInstr &MI) const;
  public:
    static char ID;
    PPCPreEmitPeephole() : MachineFunctionPass(ID) {
      initializePPCPreEmitPeepholePass(*PassRegistry::getPassRegistry());
    }

    void getAnalysisUsage(AnalysisUsage &AU) const override {
      MachineFunctionPass::getAnalysisUsage(AU);
    }

    MachineFunctionProperties getRequiredProperties() const override {
      return MachineFunctionProperties().set(
          MachineFunctionProperties::Property::NoVRegs);
    }

    bool runOnMachineFunction(MachineFunction &MF) override {
      if (skipFunction(*MF.getFunction()) || !RunPreEmitPeephole)
        return false;
      bool Changed = false;
      const PPCInstrInfo *TII = MF.getSubtarget<PPCSubtarget>().getInstrInfo();
      SmallVector<MachineInstr *, 4> InstrsToErase;
      for (MachineBasicBlock &MBB : MF) {
        for (MachineInstr &MI : MBB) {
          MachineInstr *DefMIToErase = nullptr;
          MachineInstr *RedundantPairMI = nullptr;
          if (TII->convertToImmediateForm(MI, &DefMIToErase)) {
            Changed = true;
            NumRRConvertedInPreEmit++;
            DEBUG(dbgs() << "Converted instruction to imm form: ");
            DEBUG(MI.dump());
            if (DefMIToErase) {
              InstrsToErase.push_back(DefMIToErase);
            }
          } else if ((RedundantPairMI = hasRedundantPair(MI))) {
            InstrsToErase.push_back(RedundantPairMI);
            NumRedundantPairsFound++;
          }
        }
      }
      for (MachineInstr *MI : InstrsToErase) {
        DEBUG(dbgs() << "PPC pre-emit peephole: erasing instruction: ");
        DEBUG(MI->dump());
        assert(MI->getParent() &&
               "The same instruction marked redundant multiple times?");
        MI->eraseFromParent();
        NumRemovedInPreEmit++;
      }
      return Changed;
    }
  };

  // For now, only handle mr X, Y -> mr Y, X with no clobber of Y in between
  // as that is the only pattern known to come up.
  MachineInstr *PPCPreEmitPeephole::hasRedundantPair(MachineInstr &MI) const {
    unsigned Opc = MI.getOpcode();

    // If this isn't an mr, return null.
    if ((Opc != PPC::OR && Opc != PPC::OR8) ||
        MI.getOperand(1).getReg() != MI.getOperand(2).getReg())
      return nullptr;

    unsigned DestReg = MI.getOperand(0).getReg();
    unsigned SrcReg = MI.getOperand(1).getReg();
    const MachineRegisterInfo *MRI = &MI.getParent()->getParent()->getRegInfo();
    const TargetRegisterInfo *TRI = MRI->getTargetRegisterInfo();

    MachineBasicBlock::iterator StartIt = MI, EndIt = MI.getParent()->end();
    StartIt++;
    for (; StartIt != EndIt; ++StartIt) {
      // If we encounter an instruction that clobbers the destination,
      // any subsequent copy of the original destination won't be copying
      // the same register, return null.
      if (StartIt->modifiesRegister(DestReg, TRI))
        return nullptr;

      // If we find an instruction that clobbers the source register, it is
      // redundant if it's a copy from the original destination and we haven't
      // exited above.
      if (StartIt->modifiesRegister(SrcReg, TRI)) {
        if (StartIt->getOpcode() != Opc ||
            StartIt->getOperand(1).getReg() != DestReg ||
            StartIt->getOperand(2).getReg() != DestReg)
          return nullptr;

        // Clear the kill flags from the original instruction.
        MI.getOperand(1).setIsKill(false);
        MI.getOperand(2).setIsKill(false);
        return &*StartIt;
      }
    }
    return nullptr;
  }
}

INITIALIZE_PASS(PPCPreEmitPeephole, DEBUG_TYPE, "PowerPC Pre-Emit Peephole",
                false, false)
char PPCPreEmitPeephole::ID = 0;

FunctionPass *llvm::createPPCPreEmitPeepholePass() {
  return new PPCPreEmitPeephole();
}
