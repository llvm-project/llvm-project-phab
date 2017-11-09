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

protected:
  bool basicBlockRequiresInsertionPoint(
      const MCCodePaddingContext &Context) override;

  bool usePoliciesForBasicBlock(const MCCodePaddingContext &Context) override;

public:
  X86MCCodePadder(StringRef CPU);
  virtual ~X86MCCodePadder() {}
};

/// A padding policy that handles brach instructions (all types of jmps and
/// calls) and the first instruction after a branch (i.e. first instruction in a
/// basic block reachable by branch).
/// This policy tries to enforce that:
/// 1. Branch instructions and first instructions in basic blocks won't cross a
///    16B aligned window.
/// 2. Branch instructions will end at a 0mod16 address.
///
/// Note that this is also the order of importance implemented in the policy.
///
/// This policy essentially implements part of rule 12 of section 3.4.1.5 ("Code
/// alignment") of Intel's Architectures Optimization Reference Manual:
/// "When executing code from the legacy decode pipeline, direct branches that
/// are mostly taken should have all their instruction bytes in a 16B aligned
/// chunk of memory and nearer the end of that 16B aligned chunk."
class BranchInstructionAndTargetAlignmentPolicy : public MCCodePaddingPolicy {
  BranchInstructionAndTargetAlignmentPolicy(
      const BranchInstructionAndTargetAlignmentPolicy &) = delete;
  void operator=(const BranchInstructionAndTargetAlignmentPolicy &) = delete;

protected:
  /// Computes the penalty weight caused by having branch instruction or the
  /// instruction after a branch (i.e. first instruction in a basic block
  /// reachable by branch) being splitted over more than one instruction window,
  /// and a branch instruction not being adjacent to the end of its 16B code
  /// chunk.
  ///
  /// \param Window The instruction window.
  /// \param Offset The offset of the parent section.
  /// \param Layout Code layout information.
  ///
  /// \returns the penalty weight caused by having branch instruction or
  /// instruction after a branch being splitted over more than one instruction
  /// window, and a branch instruction not being adjacent to the end of its 16B
  /// code chunk.
  double computeWindowPenaltyWeight(const MCPFRange &Window, uint64_t Offset,
                                    MCAsmLayout &Layout) const override;

public:
  BranchInstructionAndTargetAlignmentPolicy();
  virtual ~BranchInstructionAndTargetAlignmentPolicy() {}

  /// Determines if a basic block may cause the case of first instruction after
  /// a branch (i.e. first instruction in a basic block reachable by branch)
  /// being splitted over more than one instruction window.
  ///
  /// A basic block will be considered hazardous by this policy if it is
  /// reachable by a branch (and not only via fallthrough).
  ///
  /// \param Context the context of the padding, Embeds the basic block's
  /// parameters.
  ///
  /// \returns true iff \p Context indicates that the basic block is reachable
  /// via branch.
  bool basicBlockRequiresPaddingFragment(
      const MCCodePaddingContext &Context) const override;

  /// Determines if an instruction may cause the case of a branch instrucion
  /// being splitted over more than one instruction window or a branch not
  /// being adjacent to the end of its 16B code chunk.
  ///
  /// An instruction will be considered hazardous by this policy if it is
  /// a branch (all types of jmps and calls).
  ///
  /// \param Inst Instruction to examine.
  ///
  /// \returns true iff \p Inst is a branch (all types of jmps and calls).
  bool instructionRequiresPaddingFragment(const MCInst &Inst) const override;
};

} // namespace X86
} // namespace llvm

#endif // LLVM_LIB_TARGET_X86_MCTARGETDESC_X86MCCODEPADDER_H
