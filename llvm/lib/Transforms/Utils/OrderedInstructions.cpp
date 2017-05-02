//===-- OrderedInstructions.cpp - Instruction dominance function
//-----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines utility to check dominance information for 2 instructions.
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/Utils/OrderedInstructions.h"

using namespace llvm;

#define DEBUG_TYPE "ordered-insts"

bool OrderedInstructions::dominates(const Instruction *InstA,
                                    const Instruction *InstB,
                                    const DominatorTree *DT) {
  const BasicBlock *IBB = InstA->getParent();
  if (IBB == InstB->getParent()) {
    auto OBB = OBBMap.find(IBB);
    if (OBB == OBBMap.end())
      OBB = OBBMap.insert({IBB, make_unique<OrderedBasicBlock>(IBB)}).first;
    return OBB->second->dominates(InstA, InstB);
  } else
    return DT->dominates(InstA->getParent(), InstB->getParent());
}
