//===- llvm/Transforms/Utils/OrderedInstructions.h ---------------*- C++
//-*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines an efficient way to check for dominance between 2
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

class OrderedInstructions {
  /// Used to check dominance for instructions in same basic block.
  DenseMap<const BasicBlock *, std::unique_ptr<OrderedBasicBlock>> OBBMap;

public:
  /// Return true if first instruction dominates the second.
  bool dominates(const Instruction *, const Instruction *,
                 const DominatorTree *DT);

  /// Invalidate the OrderedBasicBlock cache when its basic block changes.
  void invalidateBlock(BasicBlock *BB) { OBBMap.erase(BB); }
};

} // end namespace llvm

#endif // LLVM_TRANSFORMS_UTILS_ORDEREDINSTRUCTIONS_H
