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
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineLoopInfo.h"
#include "llvm/MC/MCAsmLayout.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCObjectStreamer.h"
#include "llvm/Target/TargetMachine.h"
#include <unordered_set>

namespace llvm {
namespace X86 {

enum PerfNopFragmentKind {
  BranchesWithSameTargetAvoidance = MCPaddingFragment::FirstTargetPerfNopFragmentKind,
  TooManyWaysAvoidance,
  SplitInstInBranchTargetAvoidance
};

//---------------------------------------------------------------------------
// X86MCCodePadder
//

X86MCCodePadder::X86MCCodePadder(StringRef CPU)
    : IsPaddingFunction(false), IsPaddingBasicBlock(false) {

  if (CPU != "sandybridge" && CPU != "corei7-avx" && CPU != "ivybridge" &&
      CPU != "core-avx-i" && CPU != "haswell" && CPU != "core-avx2" &&
      CPU != "broadwell" && CPU != "skylake")
    return;

  addPolicy(new BranchesWithSameTargetAvoidancePolicy());
}

void X86MCCodePadder::handleFunctionStart(const MachineFunction &MF,
                                          MCObjectStreamer *OS,
                                          const TargetMachine &TM,
                                          const MachineLoopInfo &LI) {
  IsPaddingFunction =
      !MF.hasInlineAsm() &&
      !MF.getFunction()->optForSize() && TM.getOptLevel() != CodeGenOpt::None;
  ArePoliciesActive = IsPaddingFunction && IsPaddingBasicBlock;

  MCCodePadder::handleFunctionStart(MF, OS, TM, LI);
}

void X86MCCodePadder::handleBasicBlockStart(const MachineBasicBlock &MBB) {
  assert(LI != nullptr &&
         "A call to handleFunctionStart with valid parameters must be made "
         "before a call to handleBasicBlockStart");
  const MachineLoop *CurrentLoop = LI->getLoopFor(&MBB);
  IsPaddingBasicBlock =
      CurrentLoop != nullptr && CurrentLoop->getSubLoops().empty();
  ArePoliciesActive = IsPaddingFunction && IsPaddingBasicBlock;

  MCCodePadder::handleBasicBlockStart(MBB);
}

bool X86MCCodePadder::requiresInsertionPoint(const MachineBasicBlock &MBB) {
  bool MBBNotReachableViaFallthrough =
      std::find(MBB.pred_begin(), MBB.pred_end(), MBB.getPrevNode()) ==
      MBB.pred_end();
  // Insertion points are places that, if contain padding, then this padding
  // will never be executed (unreachable code).
  return MBBNotReachableViaFallthrough &&
         OS->getCurrentFragment()->getKind() != MCFragment::FT_Align;
}

//---------------------------------------------------------------------------
// Utility functions
//

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

static bool isJump(const MCInst &Inst) {
  unsigned int opcode = Inst.getOpcode();
  return
      // Immidiate conditional jmps
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
      opcode == JS_4 ||
      // immidiate unconditional jmps
      opcode == JMP_1 || opcode == JMP_2 || opcode == JMP_4 ||
      // Other unconditional jmps
      isFarOrIndirectUncoditionalJump(Inst);
}

static const MCSymbol *getBranchLabel(const MCInst &Inst) {
  if (isFarOrIndirectUncoditionalJump(Inst))
    return nullptr;

  if (Inst.getNumOperands() != 1)
    return nullptr;

  const MCOperand &FirstOperand = Inst.getOperand(0);
  if (!FirstOperand.isExpr())
    return nullptr;

  if (FirstOperand.getExpr()->getKind() != MCExpr::SymbolRef)
    return nullptr;

  const MCSymbolRefExpr *RefExpr =
      static_cast<const MCSymbolRefExpr *>(FirstOperand.getExpr());
  const MCSymbol *RefSymbol = &RefExpr->getSymbol();

  if (RefSymbol->isCommon() || RefSymbol->isVariable())
    // not an offset symbol
    return nullptr;

  return RefSymbol;
}

static bool computeBranchTargetAddress(const MCInst &Inst, MCAsmLayout const &Layout,
                                uint64_t &TargetAddress) {
  const MCSymbol *RefSymbol = getBranchLabel(Inst);
  if (RefSymbol == nullptr)
    return false;
  return Layout.getSymbolOffset(*RefSymbol, TargetAddress);
}

//---------------------------------------------------------------------------
// BranchesWithSameTargetAvoidancePolicy
//

BranchesWithSameTargetAvoidancePolicy::BranchesWithSameTargetAvoidancePolicy()
    : MCCodePaddingPolicy(BranchesWithSameTargetAvoidance, UINT64_C(16), true) {
}

bool BranchesWithSameTargetAvoidancePolicy::requiresPaddingFragment(
    const MCInst &Inst) const {
  if (!isJump(Inst))
    return false;
  // label must be computable in compilation time
  return getBranchLabel(Inst) != nullptr;
}

double BranchesWithSameTargetAvoidancePolicy::computeWindowPenaltyWeight(
    const MCPFRange &Window, uint64_t Offset, MCAsmLayout &Layout) const {

  static const double COLLISION_WEIGHT = 1.0;

  double Weight = 0.0;

  std::unordered_set<const MCSymbol *> BranchTargetLabels;
  std::unordered_set<uint64_t> BranchTargetAddresses;
  for (const MCPaddingFragment *Fragment : Window) {
    const MCSymbol *TargetLabel = getBranchLabel(Fragment->getInst());
    assert(TargetLabel != nullptr && "Label must be computable");

    if (BranchTargetLabels.find(TargetLabel) != BranchTargetLabels.end()) {
      // There's already a branch pointing to that label in this window
      Weight += COLLISION_WEIGHT;
      continue;
    }
    BranchTargetLabels.insert(TargetLabel);

    uint64_t TargetAddress = UINT64_C(0);
    if (!computeBranchTargetAddress(Fragment->getInst(), Layout, TargetAddress))
      continue;
    if (BranchTargetAddresses.find(TargetAddress) !=
        BranchTargetAddresses.end())
      // There's already a branch pointing to that address in this window
      Weight += COLLISION_WEIGHT;
    else
      BranchTargetAddresses.insert(TargetAddress);
  }

  return Weight;
}

} // namespace X86
} // namespace llvm
