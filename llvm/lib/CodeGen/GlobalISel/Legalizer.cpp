//===-- llvm/CodeGen/GlobalISel/Legalizer.cpp -----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
/// \file This file implements the LegalizerHelper class to legalize individual
/// instructions and the LegalizePass wrapper pass for the primary
/// legalization.
//
//===----------------------------------------------------------------------===//

#include "llvm/CodeGen/GlobalISel/Legalizer.h"
#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/CodeGen/GlobalISel/LegalizerCombiner.h"
#include "llvm/CodeGen/GlobalISel/LegalizerHelper.h"
#include "llvm/CodeGen/GlobalISel/Utils.h"
#include "llvm/CodeGen/MachineOptimizationRemarkEmitter.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/Support/Debug.h"
#include "llvm/Target/TargetInstrInfo.h"
#include "llvm/Target/TargetSubtargetInfo.h"

#include <iterator>

#define DEBUG_TYPE "legalizer"

using namespace llvm;

char Legalizer::ID = 0;
INITIALIZE_PASS_BEGIN(Legalizer, DEBUG_TYPE,
                      "Legalize the Machine IR a function's Machine IR", false,
                      false)
INITIALIZE_PASS_DEPENDENCY(TargetPassConfig)
INITIALIZE_PASS_END(Legalizer, DEBUG_TYPE,
                    "Legalize the Machine IR a function's Machine IR", false,
                    false)

Legalizer::Legalizer() : MachineFunctionPass(ID) {
  initializeLegalizerPass(*PassRegistry::getPassRegistry());
}

void Legalizer::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<TargetPassConfig>();
  MachineFunctionPass::getAnalysisUsage(AU);
}

void Legalizer::init(MachineFunction &MF) {
}

bool Legalizer::runOnMachineFunction(MachineFunction &MF) {
  // If the ISel pipeline failed, do not bother running that pass.
  if (MF.getProperties().hasProperty(
          MachineFunctionProperties::Property::FailedISel))
    return false;
  DEBUG(dbgs() << "Legalize Machine IR for: " << MF.getName() << '\n');
  init(MF);
  const TargetPassConfig &TPC = getAnalysis<TargetPassConfig>();
  MachineOptimizationRemarkEmitter MORE(MF, /*MBFI=*/nullptr);
  LegalizerHelper Helper(MF);

  // FIXME: We could introduce new blocks and will need to fix the outer loop.
  // Until then, keep track of the number of blocks to assert that we don't.
  const size_t NumBlocks = MF.size();
  MachineRegisterInfo &MRI = MF.getRegInfo();

  // FIXME: an instruction may need more than one pass before it is legal. For
  // example on most architectures <3 x i3> is doubly-illegal. It would
  // typically proceed along a path like: <3 x i3> -> <3 x i8> -> <8 x i8>. We
  // probably want a worklist of instructions rather than naive iterate until
  // convergence for performance reasons.
  bool Changed = false;
  using VecType = SmallSetVector<MachineInstr *, 8>;
  VecType WorkList;
  for (auto *MBB : post_order(&MF)) {
    if (MBB->empty())
      continue;
    bool ReachedBegin = false;
    for (auto MII = std::prev(MBB->end()), Begin = MBB->begin();
         !ReachedBegin;) {

      MachineInstr &MI = *MII;

      // And have our iterator point to the prev instruction, if there is one.
      if (MII == Begin)
        ReachedBegin = true;
      else
        --MII;

      if (isTriviallyDead(MI, MRI)) {
        DEBUG(dbgs() << MI << "Is dead; erasing.\n");
        MI.eraseFromParentAndMarkDBGValuesForRemoval();
        continue;
      }

      // Only legalize pre-isel generic instructions: others don't have types
      // and are assumed to be legal.
      if (!isPreISelGenericOpcode(MI.getOpcode()))
        continue;
      unsigned NumNewInsns = 0;
      WorkList.clear();
      Helper.MIRBuilder.recordInsertions([&](MachineInstr *MI) {
        // Only legalize pre-isel generic instructions.
        // Legalization process could generate Target specific pseudo
        // instructions with generic types. Don't record them
        if (isPreISelGenericOpcode(MI->getOpcode())) {
          ++NumNewInsns;
          WorkList.insert(MI);
        }
        DEBUG(dbgs() << ".. .. New MI: " << *MI;);
      });
      WorkList.insert(&MI);
      LegalizerCombiner C(Helper.MIRBuilder, MF.getRegInfo(),
                          Helper.getLegalizerInfo());
      bool Changed = false;
      LegalizerHelper::LegalizeResult Res;
      while (!WorkList.empty()) {
        NumNewInsns = 0;
        MachineInstr *CurrInst = WorkList.pop_back_val();
        SmallVector<MachineInstr *, 4> DeadInstructions;
        if (C.tryCombineInstruction(*CurrInst, DeadInstructions)) {
          for (auto *DeadMI : DeadInstructions) {
            DEBUG(dbgs() << ".. Erasing Dead Instruction " << *DeadMI);
            WorkList.remove(DeadMI);
            // Actually mark the instruction for deletion.
            MachineInstr *PrevMI = &*MII;
            if (PrevMI == DeadMI)
              --MII;
            DeadMI->eraseFromParent();
          }
          Changed = true;
          continue;
        }
        Res = Helper.legalizeInstrStep(*CurrInst);
        // Error out if we couldn't legalize this instruction. We may want to
        // fall back to DAG ISel instead in the future.
        if (Res == LegalizerHelper::UnableToLegalize) {
          Helper.MIRBuilder.stopRecordingInsertions();
          if (Res == LegalizerHelper::UnableToLegalize) {
            reportGISelFailure(MF, TPC, MORE, "gisel-legalize",
                               "unable to legalize instruction", *CurrInst);
            return false;
          }
        }
        Changed |= Res == LegalizerHelper::Legalized;

      }

      Helper.MIRBuilder.stopRecordingInsertions();
    }
  }

  MachineIRBuilder MIRBuilder(MF);
  LegalizerCombiner C(MIRBuilder, MRI, Helper.getLegalizerInfo());
  MachineBasicBlock::iterator NextMI;
  for (auto &MBB : MF) {
    for (auto MI = MBB.begin(); MI != MBB.end(); MI = NextMI) {
      // Get the next Instruction before we try to legalize, because there's a
      // good chance MI will be deleted.
      // TOOD: Perhaps move this to a combiner pass later?.
      NextMI = std::next(MI);
      SmallVector<MachineInstr *, 4> DeadInsts;
      Changed |= C.tryCombineMerges(*MI, DeadInsts);
      for (auto *DeadMI : DeadInsts)
        DeadMI->eraseFromParent();
    }
  }
  // For now don't support if new blocks are inserted - we would need to fix the
  // outerloop for that.
  if (MF.size() != NumBlocks) {
    MachineOptimizationRemarkMissed R("gisel-legalize", "GISelFailure",
                                      MF.getFunction()->getSubprogram(),
                                      /*MBB=*/nullptr);
    R << "inserting blocks is not supported yet";
    reportGISelFailure(MF, TPC, MORE, R);
    return false;
  }

  return Changed;
}
