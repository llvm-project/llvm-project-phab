//===- SSAUpdaterNew.cpp - Unstructured SSA Update Tool
//----------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the SSAUpdaterNew class.
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/Utils/SSAUpdaterNew.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/TinyPtrVector.h"
#include "llvm/Analysis/InstructionSimplify.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Use.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/ValueHandle.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include <cassert>
#include <utility>

using namespace llvm;

#define DEBUG_TYPE "SSAUpdaterNew"

SSAUpdaterNew::SSAUpdaterNew(SmallVectorImpl<PHINode *> *NewPHI)
    : InsertedPHIs(NewPHI) {}

SSAUpdaterNew::~SSAUpdaterNew() {}
void SSAUpdaterNew::setType(unsigned Var, Type *Ty) { CurrentType[Var] = Ty; }
void SSAUpdaterNew::setName(unsigned Var, StringRef Name) {
  CurrentName[Var] = Name;
}

void SSAUpdaterNew::writeVariable(unsigned Var, BasicBlock *BB, Value *V) {
  CurrentDef[{Var, BB}] = V;
}

Value *SSAUpdaterNew :: readVariableAfterDef(unsigned Var, BasicBlock *BB) {
  auto CDI = CurrentDef.find({Var, BB});
  if (CDI != CurrentDef.end())
    return CDI->second;
  return readVariableAfterDef(Var, BB);
}

Value *SSAUpdaterNew::readVariableBeforeDef(unsigned Var, BasicBlock *BB) {
  Value *Result;

  // Single predecessor case, just recurse
  if (BasicBlock *Pred = BB->getSinglePredecessor()) {
    Result = readVariableAfterDef(Var, Pred);
  } else if (VisitedBlocks.count({Var, BB})) {
    // We hit our node again, meaning we had a cycle, we must insert a phi
    // node
    Result = PHINode::Create(CurrentType.lookup(Var), 0,
                             CurrentName.lookup(Var), &BB->front());
  } else {
    // Mark us visited so we can detect a cycle
    VisitedBlocks.insert({Var, BB});
    Value *Same = nullptr;
    bool AllSame = true;

    // Detect equal arguments or no arguments
    for (auto *Pred : predecessors(BB)) {
      Value *PredVal = readVariableAfterDef(Var, Pred);
      if (PredVal == Same)
        continue;
      // See if we see ourselves, due to a self-cycle
      if (CurrentDef.lookup({Var, BB}) == PredVal)
        continue;

      // If we haven't set Same,set it out. Otherwise, the args are not all the
      // same.
      if (!Same) {
        Same = PredVal;
      } else {
        Same = nullptr;
        AllSame = false;
      }
    }
    PHINode *Phi = dyn_cast_or_null<PHINode>(CurrentDef.lookup({Var, BB}));
    // We never went through the loop, our phi is undefined
    if (!Same && AllSame) {
      Result = UndefValue::get(CurrentType.lookup(Var));
      Phi->replaceAllUsesWith(Result);
      Phi->eraseFromParent();
    } else if (!AllSame) {
      if (!Phi)
        Phi = PHINode::Create(CurrentType.lookup(Var), 0,
                                 CurrentName.lookup(Var), &BB->front());
      // This must be filled in by the recursive read
      for (auto *Pred : predecessors(BB))
        Phi->addIncoming(CurrentDef.lookup({Var, Pred}), Pred);
      Result = Phi;
    } else {
      assert(Same && AllSame && "Should have set both!");
      Result = Same;
      Phi->replaceAllUsesWith(Same);
      Phi->eraseFromParent();
    }
    if (isa<PHINode>(Result))
      InsertedPHIs->push_back(Result);
    VisitedBlocks.erase({Var, BB});
  }
  writeVariable(Var, BB, Result);
  return Result;
}
