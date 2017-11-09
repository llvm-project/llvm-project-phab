//===- CallSitePromotion.cpp - Utilities for call promotion -----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements utilities useful for promoting indirect call sites to
// direct call sites.
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/Utils/CallSitePromotion.h"
#include "llvm/IR/CallSite.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

using namespace llvm;

#define DEBUG_TYPE "call-site-promotion"

bool llvm::canPromoteCallSite(CallSite CS, Function *Callee) {
  assert(!CS.getCalledFunction() && "Only indirect call sites can be promoted");

  // The callee's return value type must be bitcast compatible with the call
  // site's type. It's not safe to promote the call site, otherwise.
  if (!CastInst::castIsValid(Instruction::BitCast, CS.getInstruction(),
                             Callee->getReturnType()))
    return false;

  // Additionally, the callee and call site must agree on the number of
  // arguments.
  if (CS.arg_size() != Callee->getFunctionType()->getNumParams())
    return false;

  // And the callee's formal argument types must be bitcast compatible with the
  // corresponding actual argument types of the call site.
  for (Use &U : CS.args()) {
    unsigned ArgNo = CS.getArgumentNo(&U);
    Type *FormalTy = Callee->getFunctionType()->getParamType(ArgNo);
    if (!CastInst::castIsValid(Instruction::BitCast, U.get(), FormalTy))
      return false;
  }

  return true;
}

void llvm::makeCallSiteDirect(CallSite CS, Function *Callee,
                              SmallVectorImpl<CastInst *> *Casts) {
  assert(!CS.getCalledFunction() && "Only indirect call sites can be promoted");

  // Set the called function of the call site to be the given callee.
  CS.setCalledFunction(Callee);

  // If the function type of the call site matches that of the callee, no
  // additional work is required.
  if (CS.getFunctionType() == Callee->getFunctionType())
    return;

  // Save the return types of the call site and callee.
  Type *CallSiteRetTy = CS.getInstruction()->getType();
  Type *CalleeRetTy = Callee->getReturnType();

  // Change the function type of the call site the match that of the callee.
  CS.mutateFunctionType(Callee->getFunctionType());

  // Inspect the arguments of the call site. If an argument's type doesn't
  // match the corresponding formal argument's type in the callee, bitcast it
  // to the correct type.
  for (Use &U : CS.args()) {
    unsigned ArgNo = CS.getArgumentNo(&U);
    Type *FormalTy = Callee->getFunctionType()->getParamType(ArgNo);
    Type *ActualTy = U.get()->getType();
    if (FormalTy != ActualTy) {
      auto *Cast = CastInst::Create(Instruction::BitCast, U.get(), FormalTy, "",
                                    CS.getInstruction());
      CS.setArgument(ArgNo, Cast);
      if (Casts)
        Casts->push_back(Cast);
    }
  }

  // If the return type of the call site matches that of the callee, no
  // additional work is required.
  if (CallSiteRetTy->isVoidTy() || CallSiteRetTy == CalleeRetTy)
    return;

  // Save the users of the calling instruction. These uses will be changed to
  // use the bitcast after we create it.
  SmallVector<User *, 16> UsersToUpdate;
  for (User *U : CS.getInstruction()->users())
    UsersToUpdate.push_back(U);

  // Determine an appropriate location to create the bitcast for the return
  // value. The location depends on if we have a call or invoke instruction.
  Instruction *InsertBefore = nullptr;
  if (auto *Invoke = dyn_cast<InvokeInst>(CS.getInstruction()))
    InsertBefore =
        &SplitEdge(Invoke->getParent(), Invoke->getNormalDest())->front();
  else
    InsertBefore = &*std::next(CS.getInstruction()->getIterator());

  // Bitcast the return value to the correct type.
  auto *Cast = CastInst::Create(Instruction::BitCast, CS.getInstruction(),
                                CallSiteRetTy, "", InsertBefore);
  if (Casts)
    Casts->push_back(Cast);

  // Replace all the original uses of the calling instruction with the bitcast.
  for (User *U : UsersToUpdate)
    U->replaceUsesOfWith(CS.getInstruction(), Cast);
}

void llvm::makeCallSiteIndirect(CallSite CS, Value *CalledValue,
                                ArrayRef<CastInst *> Casts) {
  assert(CS.getCalledFunction() && "Only direct call sites can be demoted");

  // For each cast instruction, replace all of its uses with its source
  // operand.
  for (CastInst *Cast : Casts) {
    while (!Cast->user_empty())
      Cast->user_back()->replaceUsesOfWith(Cast, Cast->getOperand(0));
    Cast->eraseFromParent();
  }

  // Set the called value of the call site, and mutate its function type.
  CS.setCalledFunction(CalledValue);
  CS.mutateFunctionType(cast<FunctionType>(
      cast<PointerType>(CalledValue->getType())->getElementType()));
}

Instruction *llvm::versionCallSite(CallSite CS, CmpInst::Predicate Pred,
                                   Value *LHS, Value *RHS) {
  IRBuilder<> Builder(CS.getInstruction());
  Instruction *OrigInst = CS.getInstruction();
  BasicBlock *OrigBlock = OrigInst->getParent();

  // Create the compare. LHS and RHS must have the same type to be compared. If
  // the don't, cast RHS to have LHS's type.
  if (LHS->getType() != RHS->getType())
    RHS = Builder.CreateBitCast(RHS, LHS->getType());
  auto *Cond = Builder.CreateICmp(Pred, LHS, RHS);

  // Create an if-then-else structure. The original instruction is moved into
  // the "then" block, and a clone of the original instruction is placed in the
  // "else" block.
  TerminatorInst *ThenTerm = nullptr;
  TerminatorInst *ElseTerm = nullptr;
  SplitBlockAndInsertIfThenElse(Cond, CS.getInstruction(), &ThenTerm,
                                &ElseTerm);
  BasicBlock *ThenBlock = ThenTerm->getParent();
  BasicBlock *ElseBlock = ElseTerm->getParent();
  BasicBlock *MergeBlock = OrigInst->getParent();
  Instruction *NewInst = OrigInst->clone();
  OrigInst->moveBefore(ThenTerm);
  NewInst->insertBefore(ElseTerm);

  // If the original call site is an invoke instruction, we have extra work to
  // do since invoke instructions are terminating. We have to fix-up phi nodes
  // in the invoke's normal and unwind destinations.
  if (auto *OrigInvoke = dyn_cast<InvokeInst>(OrigInst)) {
    auto *NewInvoke = cast<InvokeInst>(NewInst);

    // Invoke instructions are terminating, so we don't need the terminator
    // instructions that were just created.
    ThenTerm->eraseFromParent();
    ElseTerm->eraseFromParent();

    // Branch from the "merge" block to the original normal destination.
    Builder.SetInsertPoint(MergeBlock);
    Builder.CreateBr(OrigInvoke->getNormalDest());

    // Fix-up phi nodes in the normal destination. Values coming from the
    // original block will now be coming from the "merge" block.
    for (auto &I : *OrigInvoke->getNormalDest()) {
      auto *Phi = dyn_cast<PHINode>(&I);
      if (!Phi)
        break;
      int Idx = Phi->getBasicBlockIndex(OrigBlock);
      if (Idx == -1)
        continue;
      Phi->setIncomingBlock(Idx, MergeBlock);
    }

    // Now set the normal destinations of the invoke instructions to be the
    // "merge" block.
    OrigInvoke->setNormalDest(MergeBlock);
    NewInvoke->setNormalDest(MergeBlock);

    // Fix-up phi nodes in the unwind destination. Values coming from the
    // original block will now be coming from either the "then" block or the
    // "else" block.
    for (auto &I : *OrigInvoke->getUnwindDest()) {
      auto *Phi = dyn_cast<PHINode>(&I);
      if (!Phi)
        break;
      int Idx = Phi->getBasicBlockIndex(OrigBlock);
      if (Idx == -1)
        continue;
      auto *V = Phi->getIncomingValue(Idx);
      Phi->setIncomingBlock(Idx, ThenBlock);
      Phi->addIncoming(V, ElseBlock);
    }
  }

  // If the callee returns a value, we now have to merge the value of the
  // original instruction and its clone. Create a new phi node at the front of
  // the "merge" block, and replace uses of the original instruction with this
  // phi.
  if (!OrigInst->getType()->isVoidTy()) {
    Builder.SetInsertPoint(&MergeBlock->front());
    PHINode *Phi = Builder.CreatePHI(NewInst->getType(), 0);
    SmallVector<User *, 16> UsersToUpdate;
    for (User *U : OrigInst->users())
      UsersToUpdate.push_back(U);
    for (User *U : UsersToUpdate)
      U->replaceUsesOfWith(OrigInst, Phi);
    Phi->addIncoming(OrigInst, ThenBlock);
    Phi->addIncoming(NewInst, ElseBlock);
  }
  return NewInst;
}

#undef DEBUG_TYPE
