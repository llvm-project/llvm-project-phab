//===- InstCombinePatterns.cpp --------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the instruction pattern combiner classes.
// Currently, it handles pattern expressions for:
//  * Truncate instruction
//
//===----------------------------------------------------------------------===//

#include "InstCombineInternal.h"
#include "llvm/ADT/MapVector.h"
#include "llvm/Analysis/ConstantFolding.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/IR/DataLayout.h"
using namespace llvm;

#define DEBUG_TYPE "instcombine"

static cl::opt<unsigned> MaxDepth("instcombine-pattern-depth",
                                  cl::desc("InstCombine Pattern Depth."),
                                  cl::init(20), cl::Hidden);

/// \brief Inserts an instruction \p New before instruction \p Old and update
/// \p New instruction DebugLoc and Name with those of \p Old instruction.
static void InsertAndUpdateNewInstWith(Instruction &New, Instruction &Old) {
  assert(!New.getParent() &&
         "New instruction already inserted into a basic block!");
  BasicBlock *BB = Old.getParent();
  BB->getInstList().insert(Old.getIterator(), &New); // Insert inst
  New.setDebugLoc(Old.getDebugLoc());
  New.takeName(&Old);
}

//===----------------------------------------------------------------------===//
// TruncInstCombine - looks for expression dags dominated by trunc instructions
// and for each eligible dag, it will create a reduced bit-width expression and
// replace the old expression with this new one and remove the old one.
// Eligible expression dag is such that:
//   1. Contains only supported instructions.
//   2. Supported leaves: ZExtInst, SExtInst, TruncInst and Constant value.
//   3. Can be evaluated into type with reduced legal bit-width (or Trunc type).
//   4. All instructions in the dag must not have users outside the dag.
//      Only exception is for {ZExt, SExt}Inst with operand type equal to the
//      new reduced type chosen in (3).
//
// The motivation for this optimization is that evaluating and expression using
// smaller bit-width is preferable, especially for vectorization where we can
// fit more values in one vectorized instruction. In addition, this optimization
// may decrease the number of cast instructions, but will not increase it.
//===----------------------------------------------------------------------===//

namespace {
class TruncInstCombine {
  TargetLibraryInfo &TLI;
  const DataLayout &DL;
  AssumptionCache *AC;
  const DominatorTree *DT;

  /// List of all TruncInst instructions to be processed.
  SmallVector<TruncInst *, 4> Worklist;

  /// Current processed TruncInst instruction.
  TruncInst *CurrentTruncInst;

  /// Information per each instruction in the expression dag.
  struct Info {
    /// Number of LSBs that are needed to generate a valid expression.
    unsigned ValidBitWidth = 0;
    /// Minimum number of LSBs needed to generate the ValidBitWidth.
    unsigned MinBitWidth = 0;
    /// The reduced value generated to replace the old instruction.
    Value *NewValue = nullptr;
  };
  /// An ordered map representing expression dag dominated by current processed
  /// TruncInst. It maps each instruction in the dag to its Info structure.
  /// The map is ordered such that each instruction appears before all other
  /// instructions in the dag that uses it.
  MapVector<Instruction *, Info> InstInfoMap;

public:
  TruncInstCombine(TargetLibraryInfo &TLI, const DataLayout &DL,
                   AssumptionCache *AC, const DominatorTree *DT)
      : TLI(TLI), DL(DL), AC(AC), DT(DT), CurrentTruncInst(nullptr) {}
  ~TruncInstCombine() {}

  /// Perform TruncInst pattern optimization on given function.
  bool run(Function &F);

private:
  /// Build expression dag dominated by the given /p V value and append it to
  /// the InstInfoMap container. This function is a recursive function gaurded
  /// by a depth limit. It will be called first with TruncInst operand.
  ///
  /// \param V value node to generate its sub expression dag.
  /// \param Depth current depth of the recursion.
  /// \return true only if succeed to generate an eligible sub expression dag.
  bool buildTruncExpressionDag(Value *V, unsigned Depth);

  /// Calculate the minimum number of LSBs needed to generate the given number
  /// of /p ValidBitWidth, by the given /p V value.
  ///
  /// \param V value to be evaluated.
  /// \param ValidBitWidth number of LSBs needed to be preserved.
  /// \return minimum number of LSBs needed to preserve \p ValidBitWidth.
  unsigned getMinBitWidth(Value *V, unsigned ValidBitWidth);

  /// Build an expression dag dominated by the current processed TruncInst and
  /// Check if it is eligible to be reduced to a smaller type.
  ///
  /// \return the scalar version of the new type to be used for the reduced
  ///         expression dag, or nullptr if the expression dag is not eligible
  ///         to be reduced.
  Type *getBestTruncatedType();

  /// Given a \p V value and a \p SclTy scalar type return the generated reduced
  /// value of \p V based on the type \p SclTy.
  ///
  /// \param V value to be reduced.
  /// \param SclTy scalar version of new type to reduce to.
  /// \return the new reduced value.
  Value *getReducedOperand(Value *V, Type *SclTy);

  /// Create a new expression dag using the reduced /p SclTy type and replace
  /// the old expression dag with it. Also erase all instructions in the old
  /// dag, except those that are still needed outside the dag.
  ///
  /// \param SclTy scalar version of new type to reduce expression dag into.
  void ReduceExpressionDag(Type *SclTy);
};
} // namespace

/// Given an instruction and a container, it fills all the relevant operands of
/// that instruction, with respect to the Trunc expression dag optimizaton.
static void getRelevantOperands(Instruction *I, SmallVectorImpl<Value *> &Ops) {
  unsigned Opc = I->getOpcode();
  switch (Opc) {
  case Instruction::Trunc:
  case Instruction::ZExt:
  case Instruction::SExt:
    // trunc(trunc(x)) -> trunc(x)
    // trunc(ext(x)) -> ext(x) if the source type is smaller than the new dest
    // trunc(ext(x)) -> trunc(x) if the source type is larger than the new dest
    break;
  case Instruction::Add:
  case Instruction::Sub:
  case Instruction::Mul:
  case Instruction::And:
  case Instruction::Or:
  case Instruction::Xor:
    Ops.push_back(I->getOperand(0));
    Ops.push_back(I->getOperand(1));
    break;
  default:
    llvm_unreachable("Unreachable!");
  }
}

bool TruncInstCombine::buildTruncExpressionDag(Value *V, unsigned Depth) {
  if (Depth > MaxDepth)
    return false;

  if (isa<Constant>(V))
    return true;

  Instruction *I = dyn_cast<Instruction>(V);
  if (!I)
    return false;

  if (InstInfoMap.count(I))
    return true;

  unsigned Opc = I->getOpcode();
  switch (Opc) {
  case Instruction::Trunc:
  case Instruction::ZExt:
  case Instruction::SExt:
  case Instruction::Add:
  case Instruction::Sub:
  case Instruction::Mul:
  case Instruction::And:
  case Instruction::Or:
  case Instruction::Xor: {
    SmallVector<Value *, 2> Operands;
    getRelevantOperands(I, Operands);
    for (Value *Operand : Operands)
      if (!buildTruncExpressionDag(Operand, Depth + 1))
        return false;
    break;
  }
  default:
    // TODO: Can handle more cases here.
    return false;
  }

  // Insert I to the Info map.
  InstInfoMap.insert(std::make_pair(I, Info()));
  return true;
}

unsigned TruncInstCombine::getMinBitWidth(Value *V, unsigned ValidBitWidth) {
  if (isa<Constant>(V))
    return ValidBitWidth;

  // Otherwise, it must be an instruction.
  Instruction *I = cast<Instruction>(V);
  auto &Info = InstInfoMap[I];

  // If we already calculated the minimum bit-width for this valid bit-width, or
  // for a smaler valid bit-width, then just return the answer we already have.
  if (Info.ValidBitWidth >= ValidBitWidth)
    return Info.MinBitWidth;

  SmallVector<Value *, 2> Operands;
  getRelevantOperands(I, Operands);

  // Must update both valid bit-width and minimum bit-width at this point, to
  // prevent infinit loop, when this instruction is part of a loop in the dag.
  Info.ValidBitWidth = ValidBitWidth;
  Info.MinBitWidth = std::max(Info.MinBitWidth, ValidBitWidth);

  unsigned MinBitWidth = Info.MinBitWidth;
  for (auto *Operand : Operands)
    MinBitWidth = std::max(MinBitWidth, getMinBitWidth(Operand, ValidBitWidth));

  // Update minimum bit-width.
  Info.MinBitWidth = MinBitWidth;
  return MinBitWidth;
}

Type *TruncInstCombine::getBestTruncatedType() {
  Value *Src = CurrentTruncInst->getOperand(0);

  // Clear old expression dag.
  InstInfoMap.clear();
  if (!buildTruncExpressionDag(Src, 0))
    return nullptr;

  Type *DstTy = CurrentTruncInst->getType();
  unsigned OrigBitWidth = Src->getType()->getScalarSizeInBits();
  unsigned ValidBitWidth = DstTy->getScalarSizeInBits();

  // Calculate minimum bit-width needed to calculate the valid bit-bidth.
  unsigned MinBitWidth = getMinBitWidth(Src, ValidBitWidth);

  // If minimum bit-width is equal to valid bit-width, and original bit-width is
  // not legal integer, then use the target destenation type. Otherwise, use
  // the smallest integer type in the range [MinBitWidth, OrigBitWidth).
  Type *NewDstSclTy = DstTy->getScalarType();
  if (DL.isLegalInteger(OrigBitWidth) || MinBitWidth > ValidBitWidth) {
    NewDstSclTy = DL.getSmallestLegalIntType(DstTy->getContext(), MinBitWidth);
    if (!NewDstSclTy)
      return nullptr;
    // update minimum bit-width with the new destenation type bit-width.
    MinBitWidth = NewDstSclTy->getScalarSizeInBits();
  }

  // We don't want to duplicate instructions, which isn't profitable. Thus, we
  // can't shrink something that has multiple users, unless all users are
  // dominated by the trunc instruction, i.e., were visited during the
  // expresion evaluation.
  for (auto Itr : InstInfoMap) {
    Instruction *I = Itr.first;
    if (I->hasOneUse())
      continue;
    // If this is an extension from the dest type, we can eliminate it, even if
    // it has multiple users.
    if ((isa<ZExtInst>(I) || isa<SExtInst>(I)) &&
        I->getOperand(0)->getType()->getScalarSizeInBits() == MinBitWidth)
      continue;
    for (auto *U : I->users())
      if (auto *UI = dyn_cast<Instruction>(U))
        if (UI != CurrentTruncInst && !InstInfoMap.count(UI))
          return nullptr;
  }

  return NewDstSclTy;
}

/// Given a reduced scalar type \p Ty and a \p V value, return a reduced type
/// for \p V, according to its type, if it vector type, return the vector
/// version of \p Ty, otherwise return \p Ty.
static Type *getReducedType(Value *V, Type *Ty) {
  assert(Ty && !Ty->isVectorTy() && "Expect Scalar Type");
  if (auto *VTy = dyn_cast<VectorType>(V->getType()))
    return VectorType::get(Ty, VTy->getNumElements());
  return Ty;
}

Value *TruncInstCombine::getReducedOperand(Value *V, Type *SclTy) {
  Type *Ty = getReducedType(V, SclTy);
  if (Constant *C = dyn_cast<Constant>(V)) {
    C = ConstantExpr::getIntegerCast(C, Ty, false);
    // If we got a constantexpr back, try to simplify it with DL info.
    if (Constant *FoldedC = ConstantFoldConstant(C, DL, &TLI))
      C = FoldedC;
    return C;
  }

  Instruction *I = cast<Instruction>(V);
  assert(InstInfoMap[I].NewValue);
  return InstInfoMap[I].NewValue;
}

void TruncInstCombine::ReduceExpressionDag(Type *SclTy) {
  for (auto &Itr : InstInfoMap) { // Forward
    Instruction *I = Itr.first;

    // Check if this instruction have already been evaluated.
    assert(!InstInfoMap[I].NewValue);

    Instruction *Res = nullptr;
    unsigned Opc = I->getOpcode();
    switch (Opc) {
    case Instruction::Trunc:
    case Instruction::ZExt:
    case Instruction::SExt: {
      Type *Ty = getReducedType(I, SclTy);
      // If the source type of the cast is the type we're trying for then we can
      // just return the source.  There's no need to insert it because it is not
      // new.
      if (I->getOperand(0)->getType() == Ty) {
        InstInfoMap[I].NewValue = I->getOperand(0);
        continue;
      }
      // Otherwise, must be the same type of cast, so just reinsert a new one.
      // This also handles the case of zext(trunc(x)) -> zext(x).
      Res = CastInst::CreateIntegerCast(I->getOperand(0), Ty,
                                        Opc == Instruction::SExt);
      break;
    }
    case Instruction::Add:
    case Instruction::Sub:
    case Instruction::Mul:
    case Instruction::And:
    case Instruction::Or:
    case Instruction::Xor: {
      Value *LHS = getReducedOperand(I->getOperand(0), SclTy);
      Value *RHS = getReducedOperand(I->getOperand(1), SclTy);
      Res = BinaryOperator::Create((Instruction::BinaryOps)Opc, LHS, RHS);
      break;
    }
    default:
      // TODO: Can handle more cases here.
      llvm_unreachable("Unreachable!");
    }

    // Update Worklist entries with new value if needed.
    if (auto *NewCI = dyn_cast<TruncInst>(Res)) {
      auto Entry = std::find(Worklist.begin(), Worklist.end(), I);
      if (Entry != Worklist.end())
        *Entry = NewCI;
    }
    InstInfoMap[I].NewValue = Res;
    InsertAndUpdateNewInstWith(*Res, *I);
  }

  Value *Res = getReducedOperand(CurrentTruncInst->getOperand(0), SclTy);
  Type *DstTy = CurrentTruncInst->getType();
  Instruction *NewInst = nullptr;
  if (Res->getType() != DstTy) {
    NewInst = CastInst::CreateIntegerCast(Res, DstTy, false);
    InsertAndUpdateNewInstWith(*NewInst, *CurrentTruncInst);
    Res = NewInst;
  }
  CurrentTruncInst->replaceAllUsesWith(Res);

  // Erase old expression dag, which was replaced by the reduced expression dag.
  // We iterate backward, which means we visit the instruction before we visit
  // any of its operands, this way, when we get to the operand, we already
  // removed the instructions (from the expression dag) that uses it.
  CurrentTruncInst->eraseFromParent();
  for (auto I = InstInfoMap.rbegin(), E = InstInfoMap.rend(); I != E; ++I) {
    // We still need to check that the instruction has no users before we erase
    // it, because {SExt, ZExt}Int Instruction might have other users that was
    // not reduced, in such case, we need to keep that instruction.
    if (!I->first->getNumUses())
      I->first->eraseFromParent();
  }
}

bool TruncInstCombine::run(Function &F) {
  bool MadeIRChange = false;

  // Collect all TruncInst in the function into the Worklist for evaluating.
  for (auto &BB : F)
    for (auto &I : BB)
      if (TruncInst *CI = dyn_cast<TruncInst>(&I))
        Worklist.push_back(CI);

  // Process all TruncInst in the Worklist, for each instruction:
  //   1. Check if we it domenates an eligible expression dag to be reduced.
  //   2. Create a reduced expression dag and replace the old one with it.
  while (!Worklist.empty()) {
    CurrentTruncInst = Worklist.pop_back_val();

    if (Type *NewDstSclTy = getBestTruncatedType()) {
      DEBUG(dbgs() << "ICE: TruncInstCombine reducing type of expression dag "
                      "dominated by: "
                   << CurrentTruncInst << '\n');
      ReduceExpressionDag(NewDstSclTy);
      MadeIRChange = true;
    }
  }

  return MadeIRChange;
}

bool PatternInstCombiner::run(Function &F) {
  bool MadeIRChange = false;

  // Handle TruncInst patterns
  TruncInstCombine TIC(TLI, DL, AC, DT);
  MadeIRChange |= TIC.run(F);

  // TODO: add more patterns to handle...

  return MadeIRChange;
}