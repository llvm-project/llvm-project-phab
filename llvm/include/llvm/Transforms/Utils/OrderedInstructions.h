//===- llvm/Transforms/Utils/OrderedInstructions.h -------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines an efficient way to check for dominance relation between 2
// instructions.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_TRANSFORMS_UTILS_ORDEREDINSTRUCTIONS_H
#define LLVM_TRANSFORMS_UTILS_ORDEREDINSTRUCTIONS_H

#include "llvm/ADT/DenseMap.h"
#include "llvm/Analysis/OrderedBasicBlock.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Operator.h"

namespace llvm {

/// This interface dispatches to appropriate dominance check given 2 instructions,
/// i.e. in case the instructions are in the same basic block, OrderedBasicBlock (with instruction numbering and caching) are used. Otherwise, dominator tree is used.
/// This interface relies on the transformations to invalidate the basic blocks in case instructions in it are changed.
class OrderedInstructions {
  /// Used to check dominance for instructions in same basic block.
  mutable DenseMap<const BasicBlock *, std::unique_ptr<OrderedBasicBlock>>
      OBBMap;

  /// The dominator tree of the parent function.
  DominatorTree *DT;

public:
  /// Constructors.
  OrderedInstructions() = default;
  OrderedInstructions(DominatorTree *DT) : DT(DT) {}

  /// Return true if first instruction dominates the second. Use the class
  /// member dominator tree.
  bool dominates(const Instruction *, const Instruction *) const;

  /// Return true if first instruction dominates the second. Use the passed
  /// dominator tree.
  bool dominates(const Instruction *, const Instruction *,
                 const DominatorTree *) const;

  /// Invalidate the OrderedBasicBlock cache when its basic block changes.
  void invalidateBlock(BasicBlock *BB) { OBBMap.erase(BB); }
};

} // end namespace llvm

#endif // LLVM_TRANSFORMS_UTILS_ORDEREDINSTRUCTIONS_H
