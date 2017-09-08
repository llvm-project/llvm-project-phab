//===- llvm/unittest/IR/BasicBlockRemoveEdgeTest.cpp - BBremoveEdge tests -===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/MDBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/NoFolder.h"
#include "llvm/IR/Verifier.h"
#include "gtest/gtest.h"

using namespace llvm;

namespace {

class BBremoveEdgeTest : public testing::Test {
protected:
  void SetUp() override {
    M.reset(new Module("MyModule", Ctx));
    FunctionType *FTy = FunctionType::get(Type::getVoidTy(Ctx),
                                          /*isVarArg=*/false);
    F = Function::Create(FTy, Function::ExternalLinkage, "MyFunc", M.get());
    for (int i = 0; i < 5; ++i) {
      BasicBlock *B;
      if (i == 0) {
        B = BasicBlock::Create(Ctx, "entry", F);
      } else {
        B = BasicBlock::Create(Ctx, "bb" + std::to_string(i), F);
        IRBuilder<> Builder(B);
        Builder.CreateRetVoid();
      }
      BB.push_back(B);
    }
    GV = new GlobalVariable(*M, Type::getFloatTy(Ctx), true,
                            GlobalValue::ExternalLinkage, nullptr);
    DT = new DominatorTree(*F);
    ParentPad = llvm::ConstantTokenNone::get(Ctx);
  }

  void TearDown() override {
    BB.clear();
    F = nullptr;
    GV = nullptr;
    DT = nullptr;
    ParentPad = nullptr;
    M.reset();
  }

  std::unique_ptr<Module> M;
  std::vector<BasicBlock *> BB;
  LLVMContext Ctx;
  Function *F;
  DominatorTree *DT;
  GlobalVariable *GV;
  ConstantTokenNone *ParentPad;
};

TEST_F(BBremoveEdgeTest, CleanupReturn) {
  IRBuilder<> Builder(BB[0]);
  CleanupPadInst *CleanPI = Builder.CreateCleanupPad(ParentPad);
  Builder.CreateCleanupRet(CleanPI, BB[1]);
  DT->insertEdge(BB[0], BB[1]);
  EXPECT_TRUE(DT->verify());
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[1]));
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 1u);
  BB[0]->removeEdge(BB[1], DT);
  EXPECT_TRUE(DT->verify());
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 0u);
  UnreachableInst *UI = dyn_cast<UnreachableInst>(BB[0]->getTerminator());
  EXPECT_NE(UI, nullptr);
}

TEST_F(BBremoveEdgeTest, CatchReturn) {
  IRBuilder<> Builder(BB[0]);
  CatchPadInst *CatchPI = Builder.CreateCatchPad(ParentPad, nullptr);
  Builder.CreateCatchRet(CatchPI, BB[1]);
  DT->insertEdge(BB[0], BB[1]);
  EXPECT_TRUE(DT->verify());
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[1]));
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 1u);
  BB[0]->removeEdge(BB[1], DT);
  EXPECT_TRUE(DT->verify());
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 0u);
  UnreachableInst *UI = dyn_cast<UnreachableInst>(BB[0]->getTerminator());
  EXPECT_NE(UI, nullptr);
}

TEST_F(BBremoveEdgeTest, Invoke) {
  IRBuilder<> Builder(BB[0]);
  Builder.CreateInvoke(F, BB[1], BB[2]);
  DT->recalculate(*F); // cannot insert two edges at once with insertEdge()
  EXPECT_TRUE(DT->verify());
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[1]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[2]));
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 2u);
  BB[0]->removeEdge(BB[1], DT, /* removeInvoke */ true);
  EXPECT_TRUE(DT->verify());
  // Both invoke successors are removed with the same call due to the atomicity
  // of the instruction. We no longer have any successors after a single
  // removeEdge() call.
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 0u);
  UnreachableInst *UI = dyn_cast<UnreachableInst>(BB[0]->getTerminator());
  EXPECT_NE(UI, nullptr);
}

TEST_F(BBremoveEdgeTest, ConditionalBranch1) {
  // Both conditions connect to the same successor.
  IRBuilder<> Builder(BB[0]);
  Builder.CreateCondBr(
      ConstantInt::get(IntegerType::get(M->getContext(), 1), 0), BB[1], BB[1]);
  DT->insertEdge(BB[0], BB[1]);
  EXPECT_TRUE(DT->verify());
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[1]));
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 2u);
  BB[0]->removeEdge(BB[1], DT);
  EXPECT_TRUE(DT->verify());
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 0u);
  UnreachableInst *UI = dyn_cast<UnreachableInst>(BB[0]->getTerminator());
  EXPECT_NE(UI, nullptr);
}

TEST_F(BBremoveEdgeTest, ConditionalBranch2) {
  // Remove the first successor then the second.
  IRBuilder<> Builder(BB[0]);
  Builder.CreateCondBr(
      ConstantInt::get(IntegerType::get(M->getContext(), 1), 0), BB[1], BB[2]);
  DT->recalculate(*F); // cannot insert two edges at once with insertEdge()
  EXPECT_TRUE(DT->verify());
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[1]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[2]));
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 2u);
  BB[0]->removeEdge(BB[1], DT);
  EXPECT_TRUE(DT->verify());
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[2]));
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 1u);
  BranchInst *BI = dyn_cast<BranchInst>(BB[0]->getTerminator());
  EXPECT_NE(BI, nullptr);
  if (BI)
    EXPECT_FALSE(BI->isConditional());
  BB[0]->removeEdge(BB[2], DT);
  EXPECT_TRUE(DT->verify());
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 0u);
  UnreachableInst *UI = dyn_cast<UnreachableInst>(BB[0]->getTerminator());
  EXPECT_NE(UI, nullptr);
}

TEST_F(BBremoveEdgeTest, ConditionalBranch3) {
  // Remove the second successor then the first.
  IRBuilder<> Builder(BB[0]);
  Builder.CreateCondBr(
      ConstantInt::get(IntegerType::get(M->getContext(), 1), 0), BB[1], BB[2]);
  DT->recalculate(*F); // cannot insert two edges at once with insertEdge()
  EXPECT_TRUE(DT->verify());
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[1]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[2]));
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 2u);
  BB[0]->removeEdge(BB[2], DT);
  EXPECT_TRUE(DT->verify());
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[1]));
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 1u);
  BranchInst *BI = dyn_cast<BranchInst>(BB[0]->getTerminator());
  EXPECT_NE(BI, nullptr);
  if (BI)
    EXPECT_FALSE(BI->isConditional());
  BB[0]->removeEdge(BB[1], DT);
  EXPECT_TRUE(DT->verify());
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 0u);
  UnreachableInst *UI = dyn_cast<UnreachableInst>(BB[0]->getTerminator());
  EXPECT_NE(UI, nullptr);
}

TEST_F(BBremoveEdgeTest, UnconditionalBranch) {
  IRBuilder<> Builder(BB[0]);
  Builder.CreateBr(BB[1]);
  DT->insertEdge(BB[0], BB[1]);
  EXPECT_TRUE(DT->verify());
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[1]));
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 1u);
  BB[0]->removeEdge(BB[1], DT);
  EXPECT_TRUE(DT->verify());
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 0u);
  UnreachableInst *UI = dyn_cast<UnreachableInst>(BB[0]->getTerminator());
  EXPECT_NE(UI, nullptr);
}

TEST_F(BBremoveEdgeTest, IndirectBranch1) {
  // There is exactly one branch to remove.
  IRBuilder<> Builder(BB[0]);
  IndirectBrInst *IBI = Builder.CreateIndirectBr(F, 1);
  IBI->addDestination(BB[1]);
  DT->insertEdge(BB[0], BB[1]);
  EXPECT_TRUE(DT->verify());
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[1]));
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 1u);
  BB[0]->removeEdge(BB[1], DT);
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 0u);
  UnreachableInst *UI = dyn_cast<UnreachableInst>(BB[0]->getTerminator());
  EXPECT_NE(UI, nullptr);
}

TEST_F(BBremoveEdgeTest, IndirectBranch2) {
  // Remove only one successor (single instance).
  IRBuilder<> Builder(BB[0]);
  IndirectBrInst *IBI = Builder.CreateIndirectBr(F, 3);
  IBI->addDestination(BB[1]);
  DT->insertEdge(BB[0], BB[1]);
  IBI->addDestination(BB[2]);
  DT->insertEdge(BB[0], BB[2]);
  IBI->addDestination(BB[3]);
  DT->insertEdge(BB[0], BB[3]);
  EXPECT_TRUE(DT->verify());
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[1]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[2]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[3]));
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 3u);
  BB[0]->removeEdge(BB[2], DT);
  EXPECT_TRUE(DT->verify());
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[1]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[3]));
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 2u);
  IBI = dyn_cast<IndirectBrInst>(BB[0]->getTerminator());
  EXPECT_NE(IBI, nullptr);
}

TEST_F(BBremoveEdgeTest, IndirectBranch3) {
  // Remove only one successor (multiple instances).
  IRBuilder<> Builder(BB[0]);
  IndirectBrInst *IBI = Builder.CreateIndirectBr(F, 5);
  IBI->addDestination(BB[1]);
  DT->insertEdge(BB[0], BB[1]);
  IBI->addDestination(BB[2]);
  IBI->addDestination(BB[2]);
  DT->insertEdge(BB[0], BB[2]);
  IBI->addDestination(BB[3]);
  DT->insertEdge(BB[0], BB[3]);
  IBI->addDestination(BB[2]);
  EXPECT_TRUE(DT->verify());
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[1]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[2]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[3]));
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 5u);
  BB[0]->removeEdge(BB[2], DT);
  EXPECT_TRUE(DT->verify());
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[1]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[3]));
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 2u);
  IBI = dyn_cast<IndirectBrInst>(BB[0]->getTerminator());
  EXPECT_NE(IBI, nullptr);
}

TEST_F(BBremoveEdgeTest, IndirectBranch4) {
  // Remove all successors.
  IRBuilder<> Builder(BB[0]);
  IndirectBrInst *IBI = Builder.CreateIndirectBr(F, 5);
  IBI->addDestination(BB[1]);
  DT->insertEdge(BB[0], BB[1]);
  IBI->addDestination(BB[2]);
  IBI->addDestination(BB[2]);
  DT->insertEdge(BB[0], BB[2]);
  IBI->addDestination(BB[3]);
  DT->insertEdge(BB[0], BB[3]);
  IBI->addDestination(BB[2]);
  EXPECT_TRUE(DT->verify());
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[1]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[2]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[3]));
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 5u);
  BB[0]->removeEdge(BB[3], DT);
  EXPECT_TRUE(DT->verify());
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[1]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[2]));
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 4u);
  BB[0]->removeEdge(BB[1], DT);
  EXPECT_TRUE(DT->verify());
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[2]));
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 3u);
  BB[0]->removeEdge(BB[2], DT);
  EXPECT_TRUE(DT->verify());
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 0u);
  UnreachableInst *UI = dyn_cast<UnreachableInst>(BB[0]->getTerminator());
  EXPECT_NE(UI, nullptr);
}

TEST_F(BBremoveEdgeTest, Switch1) {
  // Switch only has a default case.
  IRBuilder<> Builder(BB[0]);
  Builder.CreateSwitch(
      ConstantInt::get(IntegerType::get(M->getContext(), 8), 0), BB[1]);
  DT->insertEdge(BB[0], BB[1]);
  EXPECT_TRUE(DT->verify());
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[1]));
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 1u);
  BB[0]->removeEdge(BB[1], DT);
  EXPECT_TRUE(DT->verify());
  UnreachableInst *UI = dyn_cast<UnreachableInst>(BB[0]->getTerminator());
  EXPECT_NE(UI, nullptr);
}

TEST_F(BBremoveEdgeTest, Switch2) {
  // Remove default case with non-default cases remaining.
  IRBuilder<> Builder(BB[0]);
  SwitchInst *SI = Builder.CreateSwitch(
      ConstantInt::get(IntegerType::get(M->getContext(), 8), 0), BB[1]);
  DT->insertEdge(BB[0], BB[1]);
  SI->addCase(ConstantInt::get(IntegerType::get(M->getContext(), 8), 1), BB[2]);
  DT->insertEdge(BB[0], BB[2]);
  SI->addCase(ConstantInt::get(IntegerType::get(M->getContext(), 8), 2), BB[3]);
  DT->insertEdge(BB[0], BB[3]);
  EXPECT_TRUE(DT->verify());
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[1]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[2]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[3]));
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 3u);
  BB[0]->removeEdge(BB[1], DT);
  EXPECT_TRUE(DT->verify());
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[2]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[3]));
  // We still have 3 successors: removeEdge() added a replacement block as the
  // default destination to preserve the integrity of the instruction.
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 3u);
  SI = dyn_cast<SwitchInst>(BB[0]->getTerminator());
  EXPECT_NE(SI, nullptr);
}

TEST_F(BBremoveEdgeTest, Switch3) {
  // Remove one non-default case, no non-defaults remaining.
  IRBuilder<> Builder(BB[0]);
  SwitchInst *SI = Builder.CreateSwitch(
      ConstantInt::get(IntegerType::get(M->getContext(), 8), 0), BB[1]);
  DT->insertEdge(BB[0], BB[1]);
  SI->addCase(ConstantInt::get(IntegerType::get(M->getContext(), 8), 1), BB[2]);
  DT->insertEdge(BB[0], BB[2]);
  EXPECT_TRUE(DT->verify());
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[1]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[2]));
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 2u);
  BB[0]->removeEdge(BB[2], DT);
  EXPECT_TRUE(DT->verify());
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[1]));
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 1u);
  SI = dyn_cast<SwitchInst>(BB[0]->getTerminator());
  EXPECT_NE(SI, nullptr);
}

TEST_F(BBremoveEdgeTest, Switch4) {
  // Remove multiple non-default cases, no non-defaults remaining.
  IRBuilder<> Builder(BB[0]);
  SwitchInst *SI = Builder.CreateSwitch(
      ConstantInt::get(IntegerType::get(M->getContext(), 8), 0), BB[1]);
  DT->insertEdge(BB[0], BB[1]);
  SI->addCase(ConstantInt::get(IntegerType::get(M->getContext(), 8), 1), BB[2]);
  DT->insertEdge(BB[0], BB[2]);
  SI->addCase(ConstantInt::get(IntegerType::get(M->getContext(), 8), 2), BB[2]);
  SI->addCase(ConstantInt::get(IntegerType::get(M->getContext(), 8), 3), BB[2]);
  EXPECT_TRUE(DT->verify());
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[1]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[2]));
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 4u);
  BB[0]->removeEdge(BB[2], DT);
  EXPECT_TRUE(DT->verify());
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[1]));
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 1u);
  SI = dyn_cast<SwitchInst>(BB[0]->getTerminator());
  EXPECT_NE(SI, nullptr);
}

TEST_F(BBremoveEdgeTest, Switch5) {
  // Remove one non-default case with non-defaults remaining.
  IRBuilder<> Builder(BB[0]);
  SwitchInst *SI = Builder.CreateSwitch(
      ConstantInt::get(IntegerType::get(M->getContext(), 8), 0), BB[1]);
  DT->insertEdge(BB[0], BB[1]);
  SI->addCase(ConstantInt::get(IntegerType::get(M->getContext(), 8), 1), BB[1]);
  SI->addCase(ConstantInt::get(IntegerType::get(M->getContext(), 8), 2), BB[2]);
  DT->insertEdge(BB[0], BB[2]);
  SI->addCase(ConstantInt::get(IntegerType::get(M->getContext(), 8), 3), BB[3]);
  DT->insertEdge(BB[0], BB[3]);
  EXPECT_TRUE(DT->verify());
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[1]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[2]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[3]));
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 4u);
  BB[0]->removeEdge(BB[2], DT);
  EXPECT_TRUE(DT->verify());
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[1]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[3]));
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 3u);
  SI = dyn_cast<SwitchInst>(BB[0]->getTerminator());
  EXPECT_NE(SI, nullptr);
}


TEST_F(BBremoveEdgeTest, Switch6) {
  // Remove multiple non-default cases with non-defaults remaining.
  IRBuilder<> Builder(BB[0]);
  SwitchInst *SI = Builder.CreateSwitch(
      ConstantInt::get(IntegerType::get(M->getContext(), 8), 0), BB[1]);
  DT->insertEdge(BB[0], BB[1]);
  SI->addCase(ConstantInt::get(IntegerType::get(M->getContext(), 8), 1), BB[1]);
  SI->addCase(ConstantInt::get(IntegerType::get(M->getContext(), 8), 2), BB[2]);
  DT->insertEdge(BB[0], BB[2]);
  SI->addCase(ConstantInt::get(IntegerType::get(M->getContext(), 8), 3), BB[3]);
  DT->insertEdge(BB[0], BB[3]);
  SI->addCase(ConstantInt::get(IntegerType::get(M->getContext(), 8), 4), BB[2]);
  SI->addCase(ConstantInt::get(IntegerType::get(M->getContext(), 8), 5), BB[2]);
  EXPECT_TRUE(DT->verify());
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[1]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[2]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[3]));
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 6u);
  BB[0]->removeEdge(BB[2], DT);
  EXPECT_TRUE(DT->verify());
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[1]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[3]));
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 3u);
  SI = dyn_cast<SwitchInst>(BB[0]->getTerminator());
  EXPECT_NE(SI, nullptr);
}

TEST_F(BBremoveEdgeTest, Switch7) {
  // Remove default case and one non-default case (non-default cases remain).
  IRBuilder<> Builder(BB[0]);
  SwitchInst *SI = Builder.CreateSwitch(
      ConstantInt::get(IntegerType::get(M->getContext(), 8), 0), BB[1]);
  DT->insertEdge(BB[0], BB[1]);
  SI->addCase(ConstantInt::get(IntegerType::get(M->getContext(), 8), 1), BB[2]);
  DT->insertEdge(BB[0], BB[2]);
  SI->addCase(ConstantInt::get(IntegerType::get(M->getContext(), 8), 2), BB[1]);
  SI->addCase(ConstantInt::get(IntegerType::get(M->getContext(), 8), 3), BB[3]);
  DT->insertEdge(BB[0], BB[3]);
  EXPECT_TRUE(DT->verify());
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[1]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[2]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[3]));
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 4u);
  BB[0]->removeEdge(BB[1], DT);
  EXPECT_TRUE(DT->verify());
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[2]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[3]));
  // We have 3 successors: removeEdge() added a replacement block as the
  // default destination to preserve the integrity of the instruction. We also
  // removed a single non-default case (4 - (1 + 1) + 1).
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 3u);
  SI = dyn_cast<SwitchInst>(BB[0]->getTerminator());
  EXPECT_NE(SI, nullptr);
}

TEST_F(BBremoveEdgeTest, Switch8) {
  // Remove default case and multiple non-default cases (non-default cases
  // remain).
  IRBuilder<> Builder(BB[0]);
  SwitchInst *SI = Builder.CreateSwitch(
      ConstantInt::get(IntegerType::get(M->getContext(), 8), 0), BB[1]);
  DT->insertEdge(BB[0], BB[1]);
  SI->addCase(ConstantInt::get(IntegerType::get(M->getContext(), 8), 1), BB[1]);
  SI->addCase(ConstantInt::get(IntegerType::get(M->getContext(), 8), 2), BB[2]);
  DT->insertEdge(BB[0], BB[2]);
  SI->addCase(ConstantInt::get(IntegerType::get(M->getContext(), 8), 3), BB[1]);
  SI->addCase(ConstantInt::get(IntegerType::get(M->getContext(), 8), 4), BB[3]);
  SI->addCase(ConstantInt::get(IntegerType::get(M->getContext(), 8), 5), BB[1]);
  DT->insertEdge(BB[0], BB[3]);
  EXPECT_TRUE(DT->verify());
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[1]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[2]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[3]));
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 6u);
  BB[0]->removeEdge(BB[1], DT);
  EXPECT_TRUE(DT->verify());
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[2]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[3]));
  // We have 3 successors: removeEdge() added a replacement block as the
  // default destination to preserve the integrity of the instruction. We also
  // removed 3 non-default cases (6 - (1 + 3) + 1).
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 3u);
  SI = dyn_cast<SwitchInst>(BB[0]->getTerminator());
  EXPECT_NE(SI, nullptr);
}

TEST_F(BBremoveEdgeTest, Switch9) {
  // Remove all cases, don't delete default case last.
  IRBuilder<> Builder(BB[0]);
  SwitchInst *SI = Builder.CreateSwitch(
      ConstantInt::get(IntegerType::get(M->getContext(), 8), 0), BB[1]);
  DT->insertEdge(BB[0], BB[1]);
  SI->addCase(ConstantInt::get(IntegerType::get(M->getContext(), 8), 1), BB[1]);
  SI->addCase(ConstantInt::get(IntegerType::get(M->getContext(), 8), 2), BB[2]);
  DT->insertEdge(BB[0], BB[2]);
  SI->addCase(ConstantInt::get(IntegerType::get(M->getContext(), 8), 3), BB[1]);
  SI->addCase(ConstantInt::get(IntegerType::get(M->getContext(), 8), 4), BB[3]);
  SI->addCase(ConstantInt::get(IntegerType::get(M->getContext(), 8), 5), BB[1]);
  DT->insertEdge(BB[0], BB[3]);
  EXPECT_TRUE(DT->verify());
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[1]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[2]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[3]));
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 6u);
  BB[0]->removeEdge(BB[1], DT);
  EXPECT_TRUE(DT->verify());
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[2]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[3]));
  // We have 3 successors: removeEdge() added a replacement block as the
  // default destination to preserve the integrity of the instruction. We also
  // removed 3 non-default cases (6 - (1 + 3) + 1).
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 3u);
  BB[0]->removeEdge(BB[2], DT);
  EXPECT_TRUE(DT->verify());
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[3]));
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 2u);
  BB[0]->removeEdge(BB[3], DT);
  EXPECT_TRUE(DT->verify());
  // The previously-inserted default basic block is the only remaining
  // successor.
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 1u);
  SI = dyn_cast<SwitchInst>(BB[0]->getTerminator());
  EXPECT_NE(SI, nullptr);
}

TEST_F(BBremoveEdgeTest, Switch10) {
  // Remove all cases and delete default case last.
  IRBuilder<> Builder(BB[0]);
  SwitchInst *SI = Builder.CreateSwitch(
      ConstantInt::get(IntegerType::get(M->getContext(), 8), 0), BB[1]);
  DT->insertEdge(BB[0], BB[1]);
  SI->addCase(ConstantInt::get(IntegerType::get(M->getContext(), 8), 1), BB[1]);
  SI->addCase(ConstantInt::get(IntegerType::get(M->getContext(), 8), 2), BB[2]);
  DT->insertEdge(BB[0], BB[2]);
  SI->addCase(ConstantInt::get(IntegerType::get(M->getContext(), 8), 3), BB[1]);
  SI->addCase(ConstantInt::get(IntegerType::get(M->getContext(), 8), 4), BB[3]);
  SI->addCase(ConstantInt::get(IntegerType::get(M->getContext(), 8), 5), BB[1]);
  DT->insertEdge(BB[0], BB[3]);
  EXPECT_TRUE(DT->verify());
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[1]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[2]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[3]));
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 6u);
  BB[0]->removeEdge(BB[3], DT);
  EXPECT_TRUE(DT->verify());
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[1]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[2]));
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 5u);
  BB[0]->removeEdge(BB[2], DT);
  EXPECT_TRUE(DT->verify());
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[1]));
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 4u);
  BB[0]->removeEdge(BB[1], DT);
  EXPECT_TRUE(DT->verify());
  // All remaining cases (default and non-default) were to BB[1]. We can safely
  // remove the entire instruction without replacing the default case to a new
  // BasicBlock.
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 0u);
  UnreachableInst *UI = dyn_cast<UnreachableInst>(BB[0]->getTerminator());
  EXPECT_NE(UI, nullptr);
}

TEST_F(BBremoveEdgeTest, CatchSwitch1) {
  // CatchSwitch only has unwind case.
  IRBuilder<> Builder(BB[0]);
  Builder.CreateCatchSwitch(ParentPad, BB[1], 0);
  DT->insertEdge(BB[0], BB[1]);
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[1]));
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 1u);
  BB[0]->removeEdge(BB[1], DT);
  EXPECT_TRUE(DT->verify());
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 0u);
  UnreachableInst *UI = dyn_cast<UnreachableInst>(BB[0]->getTerminator());
  EXPECT_NE(UI, nullptr);
}

TEST_F(BBremoveEdgeTest, CatchSwitch2) {
  // Remove the unwind case with handlers remaining.
  IRBuilder<> Builder(BB[0]);
  CatchSwitchInst *CI = Builder.CreateCatchSwitch(ParentPad, BB[1], 2);
  DT->insertEdge(BB[0], BB[1]);
  CI->addHandler(BB[2]);
  DT->insertEdge(BB[0], BB[2]);
  CI->addHandler(BB[3]);
  DT->insertEdge(BB[0], BB[3]);
  EXPECT_TRUE(DT->verify());
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[1]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[2]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[3]));
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 3u);
  BB[0]->removeEdge(BB[1], DT);
  EXPECT_TRUE(DT->verify());
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[2]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[3]));
  // We have 3 successors: removeEdge() added a replacement block as the
  // Unwind destination to preserve the integrity of the instruction
  // (3 - 1 + 1).
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 3u);
  CI = dyn_cast<CatchSwitchInst>(BB[0]->getTerminator());
  EXPECT_NE(CI, nullptr);
}

TEST_F(BBremoveEdgeTest, CatchSwitch3) {
  // Remove one handler, no handlers remaining.
  IRBuilder<> Builder(BB[0]);
  CatchSwitchInst *CI = Builder.CreateCatchSwitch(ParentPad, BB[1], 1);
  DT->insertEdge(BB[0], BB[1]);
  CI->addHandler(BB[2]);
  DT->insertEdge(BB[0], BB[2]);
  EXPECT_TRUE(DT->verify());
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[1]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[2]));
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 2u);
  BB[0]->removeEdge(BB[2], DT);
  EXPECT_TRUE(DT->verify());
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[1]));
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 1u);
  CI = dyn_cast<CatchSwitchInst>(BB[0]->getTerminator());
  EXPECT_NE(CI, nullptr);
}

TEST_F(BBremoveEdgeTest, CatchSwitch4) {
  // Remove multiple handlers, no handlers remaining.
  IRBuilder<> Builder(BB[0]);
  CatchSwitchInst *CI = Builder.CreateCatchSwitch(ParentPad, BB[1], 3);
  DT->insertEdge(BB[0], BB[1]);
  CI->addHandler(BB[2]);
  DT->insertEdge(BB[0], BB[2]);
  CI->addHandler(BB[2]);
  CI->addHandler(BB[2]);
  EXPECT_TRUE(DT->verify());
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[1]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[2]));
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 4u);
  BB[0]->removeEdge(BB[2], DT);
  EXPECT_TRUE(DT->verify());
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[1]));
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 1u);
  CI = dyn_cast<CatchSwitchInst>(BB[0]->getTerminator());
  EXPECT_NE(CI, nullptr);
}

TEST_F(BBremoveEdgeTest, CatchSwitch5) {
  // Remove one handler with handlers remaining.
  IRBuilder<> Builder(BB[0]);
  CatchSwitchInst *CI = Builder.CreateCatchSwitch(ParentPad, BB[1], 2);
  DT->insertEdge(BB[0], BB[1]);
  CI->addHandler(BB[2]);
  DT->insertEdge(BB[0], BB[2]);
  CI->addHandler(BB[3]);
  DT->insertEdge(BB[0], BB[3]);
  EXPECT_TRUE(DT->verify());
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[1]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[2]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[3]));
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 3u);
  BB[0]->removeEdge(BB[2], DT);
  EXPECT_TRUE(DT->verify());
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[1]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[3]));
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 2u);
  CI = dyn_cast<CatchSwitchInst>(BB[0]->getTerminator());
  EXPECT_NE(CI, nullptr);
}

TEST_F(BBremoveEdgeTest, CatchSwitch6) {
  // Remove multiple handlers with handlers remaining.
  IRBuilder<> Builder(BB[0]);
  CatchSwitchInst *CI = Builder.CreateCatchSwitch(ParentPad, BB[1], 4);
  DT->insertEdge(BB[0], BB[1]);
  CI->addHandler(BB[2]);
  DT->insertEdge(BB[0], BB[2]);
  CI->addHandler(BB[3]);
  DT->insertEdge(BB[0], BB[3]);
  CI->addHandler(BB[2]);
  CI->addHandler(BB[4]);
  DT->insertEdge(BB[0], BB[4]);
  EXPECT_TRUE(DT->verify());
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[1]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[2]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[3]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[4]));
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 5u);
  BB[0]->removeEdge(BB[2], DT);
  EXPECT_TRUE(DT->verify());
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[1]));
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 3u);
  CI = dyn_cast<CatchSwitchInst>(BB[0]->getTerminator());
  EXPECT_NE(CI, nullptr);
}

TEST_F(BBremoveEdgeTest, CatchSwitch7) {
  // Remove unwind and one handler (some handlers remain).
  IRBuilder<> Builder(BB[0]);
  CatchSwitchInst *CI = Builder.CreateCatchSwitch(ParentPad, BB[1], 3);
  DT->insertEdge(BB[0], BB[1]);
  CI->addHandler(BB[2]);
  DT->insertEdge(BB[0], BB[2]);
  CI->addHandler(BB[1]);
  CI->addHandler(BB[3]);
  DT->insertEdge(BB[0], BB[3]);
  EXPECT_TRUE(DT->verify());
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[1]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[2]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[3]));
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 4u);
  BB[0]->removeEdge(BB[1], DT);
  EXPECT_TRUE(DT->verify());
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[2]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[3]));
  // We have 3 successors: removeEdge() added a replacement block as the
  // Unwind destination to preserve the integrity of the instruction. We also
  // removed one handler (4 - (1 + 1) + 1).
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 3u);
  CI = dyn_cast<CatchSwitchInst>(BB[0]->getTerminator());
  EXPECT_NE(CI, nullptr);
}

TEST_F(BBremoveEdgeTest, CatchSwitch8) {
  // Remove multiple handlers and unwind (some handlers remain).
  IRBuilder<> Builder(BB[0]);
  CatchSwitchInst *CI = Builder.CreateCatchSwitch(ParentPad, BB[1], 5);
  DT->insertEdge(BB[0], BB[1]);
  CI->addHandler(BB[2]);
  DT->insertEdge(BB[0], BB[2]);
  CI->addHandler(BB[3]);
  DT->insertEdge(BB[0], BB[3]);
  CI->addHandler(BB[1]);
  CI->addHandler(BB[4]);
  DT->insertEdge(BB[0], BB[4]);
  CI->addHandler(BB[1]);
  EXPECT_TRUE(DT->verify());
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[1]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[2]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[3]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[4]));
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 6u);
  BB[0]->removeEdge(BB[1], DT);
  EXPECT_TRUE(DT->verify());
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[2]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[3]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[4]));
  // We have 4 successors: removeEdge() added a replacement block as the
  // Unwind destination to preserve the integrity of the instruction. We also
  // removed two handlers (6 - (1 + 2) + 1).
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 4u);
  CI = dyn_cast<CatchSwitchInst>(BB[0]->getTerminator());
  EXPECT_NE(CI, nullptr);
}

TEST_F(BBremoveEdgeTest, CatchSwitch9) {
  // Remove all successors, don't delete unwind last.
  IRBuilder<> Builder(BB[0]);
  CatchSwitchInst *CI = Builder.CreateCatchSwitch(ParentPad, BB[1], 5);
  DT->insertEdge(BB[0], BB[1]);
  CI->addHandler(BB[2]);
  DT->insertEdge(BB[0], BB[2]);
  CI->addHandler(BB[3]);
  DT->insertEdge(BB[0], BB[3]);
  CI->addHandler(BB[1]);
  CI->addHandler(BB[4]);
  DT->insertEdge(BB[0], BB[4]);
  CI->addHandler(BB[1]);
  EXPECT_TRUE(DT->verify());
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[1]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[2]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[3]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[4]));
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 6u);
  BB[0]->removeEdge(BB[1], DT);
  EXPECT_TRUE(DT->verify());
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[2]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[3]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[4]));
  // We have 4 successors: removeEdge() added a replacement block as the
  // Unwind destination to preserve the integrity of the instruction. We also
  // removed two handlers (6 - (1 + 2) + 1).
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 4u);
  BB[0]->removeEdge(BB[2], DT);
  EXPECT_TRUE(DT->verify());
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[3]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[4]));
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 3u);
  BB[0]->removeEdge(BB[3], DT);
  EXPECT_TRUE(DT->verify());
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[4]));
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 2u);
  BB[0]->removeEdge(BB[4], DT);
  EXPECT_TRUE(DT->verify());
  // The previously-inserted unwind basic block is the only remaining
  // successor.
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 1u);
  CI = dyn_cast<CatchSwitchInst>(BB[0]->getTerminator());
  EXPECT_NE(CI, nullptr);
}

TEST_F(BBremoveEdgeTest, CatchSwitch10) {
  // Remove all successors and delete unwind last.
  IRBuilder<> Builder(BB[0]);
  CatchSwitchInst *CI = Builder.CreateCatchSwitch(ParentPad, BB[1], 5);
  DT->insertEdge(BB[0], BB[1]);
  CI->addHandler(BB[2]);
  DT->insertEdge(BB[0], BB[2]);
  CI->addHandler(BB[3]);
  DT->insertEdge(BB[0], BB[3]);
  CI->addHandler(BB[1]);
  CI->addHandler(BB[4]);
  DT->insertEdge(BB[0], BB[4]);
  CI->addHandler(BB[1]);
  EXPECT_TRUE(DT->verify());
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[1]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[2]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[3]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[4]));
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 6u);
  BB[0]->removeEdge(BB[4], DT);
  EXPECT_TRUE(DT->verify());
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[1]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[2]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[3]));
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 5u);
  BB[0]->removeEdge(BB[3], DT);
  EXPECT_TRUE(DT->verify());
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[1]));
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[2]));
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 4u);
  BB[0]->removeEdge(BB[2], DT);
  EXPECT_TRUE(DT->verify());
  EXPECT_TRUE(DT->properlyDominates(BB[0], BB[1]));
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 3u);
  BB[0]->removeEdge(BB[1], DT);
  EXPECT_TRUE(DT->verify());
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 0u);
  // All remaining cases (unwind and handler) were to BB[1]. We can safely
  // remove the entire instruction without replacing the unwind case to a new
  // BasicBlock.
  EXPECT_EQ(BB[0]->getTerminator()->getNumSuccessors(), 0u);
  UnreachableInst *UI = dyn_cast<UnreachableInst>(BB[0]->getTerminator());
  EXPECT_NE(UI, nullptr);
}

} // namespace
