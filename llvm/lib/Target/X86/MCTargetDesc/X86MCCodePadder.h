//===-- X86MCCodePadder.h - X86 Specific Code Padding Handling --*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_X86_MCTARGETDESC_X86MCCODEPADDER_H
#define LLVM_LIB_TARGET_X86_MCTARGETDESC_X86MCCODEPADDER_H

#include "llvm/ADT/StringRef.h"
#include "llvm/MC/MCCodePadder.h"

namespace llvm {

class MCPaddingFragment;
class MCAsmLayout;

namespace X86 {

/// The X86-specific class incharge of all code padding decisions for the X86
/// target.
class X86MCCodePadder : public MCCodePadder {
  X86MCCodePadder() = delete;
  X86MCCodePadder(const X86MCCodePadder &) = delete;
  void operator=(const X86MCCodePadder &) = delete;

  bool IsPaddingFunction;
  bool IsPaddingBasicBlock;

protected:
  bool requiresInsertionPoint(const MachineBasicBlock &MBB) override;

public:
  X86MCCodePadder(StringRef CPU);
  virtual ~X86MCCodePadder() {}

  void handleFunctionStart(const MachineFunction &MF, MCObjectStreamer *OS,
                           const TargetMachine &TM,
                           const MachineLoopInfo &LI) override;
  void handleBasicBlockStart(const MachineBasicBlock &MBB) override;
};

/// A padding policy designed to avoid the case of too branches with the same
/// target address in the same instruction window.
///
/// In the Intelｮ Architectures Optimization Reference Manual, under clause
/// 3.4.1, Branch Prediction Optimization, the following optimization is
/// suggested: "Avoid putting two conditional branch instructions in a loop so
/// that both have the same branch target address and, at the same time, belong
/// to (i.e.have their last bytes' addresses within) the same 16-byte aligned
/// code block.".
/// This policy helps avoid this by inserting MCPaddingFragments before
/// hazardous instructions (i.e. jmps whose target address is computable at
/// compilation time) and returning positive penalty weight for 16B windows that
/// contain this situation.
class BranchesWithSameTargetAvoidancePolicy : public MCCodePaddingPolicy {
  BranchesWithSameTargetAvoidancePolicy(
      const BranchesWithSameTargetAvoidancePolicy &) = delete;
  void operator=(const BranchesWithSameTargetAvoidancePolicy &) = delete;

protected:
  /// Computes the penalty weight caused by having branches with the same target
  /// in a given instruction windows.
  /// The weight will increase for each two or more branches with the same
  /// target.
  ///
  /// \param Window The instruction window.
  /// \param Offset The offset of the parent section.
  /// \param Layout Code layout information.
  ///
  /// \returns the penalty weight caused by having branches with the same target
  /// in \p Window
  double computeWindowPenaltyWeight(const MCPFRange &Window, uint64_t Offset,
                                    MCAsmLayout &Layout) const override;

public:
  BranchesWithSameTargetAvoidancePolicy();
  virtual ~BranchesWithSameTargetAvoidancePolicy() {}

  /// Determines if an instruction may cause the case of branches with the same
  /// target in a window.
  ///
  /// An instruction is considered hazardous by this policy if it a jmp whose
  /// target address is computable at compilation time, since two or more such
  /// jmps to the same target address will cause performance penalty.
  ///
  /// \param Inst Instruction to examine.
  ///
  /// \returns true iff \p Inst is a jmp whose target address is computable at
  /// compilation time.
  bool requiresPaddingFragment(const MCInst &Inst) const override;
};

} // namespace X86
} // namespace llvm

#endif // LLVM_LIB_TARGET_X86_MCTARGETDESC_X86MCCODEPADDER_H
