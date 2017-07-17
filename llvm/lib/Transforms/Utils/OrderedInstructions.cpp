//===-- OrderedInstructions.cpp - Instruction dominance function ---------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines utility to check dominance relation of 2 instructions.
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/Utils/OrderedInstructions.h"
#include "llvm/IR/Instructions.h"
using namespace llvm;

/// Given 2 instructions, use OrderedBasicBlock to check for dominance relation
/// if the instructions are in the same basic block, Otherwise, use dominator
/// tree dominate routines.
bool OrderedInstructions::dominates(const Instruction *Def,
                                    const Instruction *User) const {
  bool OldResult = DT->dominates(Def, User);
  const auto *DefBB = Def->getParent();
  const auto *UseBB = User->getParent();
  assert(DefBB && UseBB);
  // Any unreachable use is dominated, even if Def == User.
  if (!DT->isReachableFromEntry(UseBB)) {
    assert(OldResult == true);
    return true;
  }

  // Unreachable definitions don't dominate anything.
  if (!DT->isReachableFromEntry(DefBB)) {
    assert(OldResult == false);
    return false;
  }

  // An instruction doesn't dominate a use in itself.
  if (Def == User) {
    assert(OldResult == false);
    return false;
  }

  if (isa<InvokeInst>(Def)) {
    assert(OldResult == DT->dominates(Def, UseBB));
    return DT->dominates(Def, UseBB);
  }

  if (DefBB != UseBB) {
    assert(OldResult == DT->dominates(Def->getParent(), User->getParent()));
    return DT->dominates(Def->getParent(), User->getParent());
  }

  // Use ordered basic block to do dominance check in case the 2 instructions
  // are in the same basic block.
  auto OBB = OBBMap.find(DefBB);
  if (OBB == OBBMap.end())
    OBB = OBBMap.insert({DefBB, make_unique<OrderedBasicBlock>(DefBB)}).first;
  assert(OldResult == OBB->second->dominates(Def, User));
  return OBB->second->dominates(Def, User);
}

bool OrderedInstructions::dominates(const Instruction *Def,
                                    const Use &U) const {
  bool OldResult = DT->dominates(Def, U);
  if (Def == U.getUser()) {
    assert(OldResult == false);
    return false;
  }

  // PHI node uses appear at the end of the block they come from.
  if (auto *PN = dyn_cast<PHINode>(U.getUser())) {
    auto *DefBB = Def->getParent();
    auto *UseBB = PN->getIncomingBlock(U);
    assert(DefBB && UseBB);
    if (DefBB != UseBB) {
      assert(OldResult == DT->dominates(DefBB, UseBB));
      return DT->dominates(DefBB, UseBB);
    }
    // This mirrors the behavior of DominatorTree, which assumed no ordering
    // among existing phi nodes.
    if (isa<PHINode>(U.getUser())) {
      assert(OldResult == true);
      return true;
    }
  }
  assert(OldResult == dominates(Def, cast<Instruction>(U.getUser())));
  return dominates(Def, cast<Instruction>(U.getUser()));
}
