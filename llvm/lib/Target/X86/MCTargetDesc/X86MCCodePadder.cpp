///=== X86MCCodePadder.cpp - X86 Specific Code Padding Handling -*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/X86MCCodePadder.h"
#include "MCTargetDesc/X86BaseInfo.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineLoopInfo.h"
#include "llvm/MC/MCAsmLayout.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCObjectStreamer.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {
namespace X86 {

enum PerfNopFragmentKind {
  BranchInstructionAndTargetAlignment =
      MCPaddingFragment::FirstTargetPerfNopFragmentKind
};

//---------------------------------------------------------------------------
// X86MCCodePadder
//

X86MCCodePadder::X86MCCodePadder(StringRef CPU) {

  if (CPU != "sandybridge" && CPU != "corei7-avx" && CPU != "ivybridge" &&
      CPU != "core-avx-i" && CPU != "haswell" && CPU != "core-avx2" &&
      CPU != "broadwell" && CPU != "skylake")
    return;

  addPolicy(new BranchInstructionAndTargetAlignmentPolicy());
}

bool X86MCCodePadder::basicBlockRequiresInsertionPoint(
    const MCCodePaddingContext &Context) {
  // Insertion points are places that, if contain padding, then this padding
  // will never be executed (unreachable code).
  bool BasicBlockHasAlignment =
      OS->getCurrentFragment() == nullptr ||
      OS->getCurrentFragment()->getKind() == MCFragment::FT_Align;
  return MCCodePadder::basicBlockRequiresInsertionPoint(Context) ||
         (!Context.IsBasicBlockReachableViaFallthrough &&
          !BasicBlockHasAlignment);
}

bool X86MCCodePadder::usePoliciesForBasicBlock(
    const MCCodePaddingContext &Context) {
  return MCCodePadder::usePoliciesForBasicBlock(Context) &&
         Context.IsBasicBlockInsideInnermostLoop;
}

//---------------------------------------------------------------------------
// Utility functions
//

static bool isConditionalJump(const MCInst &Inst) {
  unsigned int opcode = Inst.getOpcode();
  return
      // Immidiate jmps
      opcode == JAE_1 || opcode == JAE_2 || opcode == JAE_4 || opcode == JA_1 ||
      opcode == JA_2 || opcode == JA_4 || opcode == JBE_1 || opcode == JBE_2 ||
      opcode == JBE_4 || opcode == JB_1 || opcode == JB_2 || opcode == JB_4 ||
      opcode == JCXZ || opcode == JECXZ || opcode == JE_1 || opcode == JE_2 ||
      opcode == JE_4 || opcode == JGE_1 || opcode == JGE_2 || opcode == JGE_4 ||
      opcode == JG_1 || opcode == JG_2 || opcode == JG_4 || opcode == JLE_1 ||
      opcode == JLE_2 || opcode == JLE_4 || opcode == JL_1 || opcode == JL_2 ||
      opcode == JL_4 || opcode == JNE_1 || opcode == JNE_2 || opcode == JNE_4 ||
      opcode == JNO_1 || opcode == JNO_2 || opcode == JNO_4 ||
      opcode == JNP_1 || opcode == JNP_2 || opcode == JNP_4 ||
      opcode == JNS_1 || opcode == JNS_2 || opcode == JNS_4 || opcode == JO_1 ||
      opcode == JO_2 || opcode == JO_4 || opcode == JP_1 || opcode == JP_2 ||
      opcode == JP_4 || opcode == JRCXZ || opcode == JS_1 || opcode == JS_2 ||
      opcode == JS_4;
}

static bool isFarOrIndirectUncoditionalJump(const MCInst &Inst) {
  unsigned int opcode = Inst.getOpcode();
  return
      // Far jmps
      opcode == FARJMP16i || opcode == FARJMP16m || opcode == FARJMP32i ||
      opcode == FARJMP32m || opcode == FARJMP64 ||
      // Memory and register jmps
      opcode == JMP16m || opcode == JMP16r || opcode == JMP32m ||
      opcode == JMP32r || opcode == JMP64m || opcode == JMP64r;
}

static bool isUnconditionalJump(const MCInst &Inst) {
  unsigned int opcode = Inst.getOpcode();
  return isFarOrIndirectUncoditionalJump(Inst) || opcode == JMP_1 ||
         opcode == JMP_2 || opcode == JMP_4;
}

static bool isJump(const MCInst &Inst) {
  return isConditionalJump(Inst) || isUnconditionalJump(Inst);
}

static bool isCall(const MCInst &Inst) {
  unsigned int opcode = Inst.getOpcode();
  return
      // PC relative calls
      opcode == CALL64pcrel32 || opcode == CALLpcrel16 ||
      opcode == CALLpcrel32 ||
      // Far calls
      opcode == FARCALL16i || opcode == FARCALL16m || opcode == FARCALL32i ||
      opcode == FARCALL32m || opcode == FARCALL64 ||
      // Memory and register calls
      opcode == CALL16m || opcode == CALL16r || opcode == CALL32m ||
      opcode == CALL32r || opcode == CALL64m || opcode == CALL64r;
}

//---------------------------------------------------------------------------
// BranchInstructionAndTargetAlignmentPolicy
//

BranchInstructionAndTargetAlignmentPolicy::
    BranchInstructionAndTargetAlignmentPolicy()
    : MCCodePaddingPolicy(BranchInstructionAndTargetAlignment, UINT64_C(16),
                          false) {}

bool BranchInstructionAndTargetAlignmentPolicy::
    basicBlockRequiresPaddingFragment(
        const MCCodePaddingContext &Context) const {
  return Context.IsBasicBlockReachableViaBranch;
}

bool BranchInstructionAndTargetAlignmentPolicy::
    instructionRequiresPaddingFragment(const MCInst &Inst) const {
  return isJump(Inst) || isCall(Inst);
}

double BranchInstructionAndTargetAlignmentPolicy::computeWindowPenaltyWeight(
    const MCPFRange &Window, uint64_t Offset, MCAsmLayout &Layout) const {

  static const double SPLIT_INST_WEIGHT = 10.0;
  static const double BRANCH_NOT_AT_CHUNK_END_WEIGHT = 1.0;

  double Weight = 0.0;
  for (const MCPaddingFragment *Fragment : Window) {
    if (!Fragment->isInstructionInitialized())
      continue;
    uint64_t InstructionStartAddress = getNextFragmentOffset(Fragment, Layout);
    uint64_t InstructionSecondByteAddress =
        InstructionStartAddress + UINT64_C(1);
    uint64_t InstructionEndAddress =
        InstructionStartAddress + Fragment->getInstSize();
    // Checking if the instruction pointed by the fragment splits over more than
    // one window.
    if (alignTo(InstructionSecondByteAddress, WindowSize) !=
        alignTo(InstructionEndAddress, WindowSize))
      Weight += SPLIT_INST_WEIGHT;
    if (instructionRequiresPaddingFragment(Fragment->getInst()) &&
        (InstructionEndAddress & UINT64_C(0xF)) != UINT64_C(0))
      Weight += BRANCH_NOT_AT_CHUNK_END_WEIGHT;
  }
  return Weight;
}

} // namespace X86
} // namespace llvm
