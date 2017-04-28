//===- MemAccessShrinking.cpp - Shrink the type of mem accesses -------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
/// \file
/// The pass tries to find opportunities where we can save bit manipulation
/// operations or convert illegal type operations to legal type by shrinking
/// the type of memory accesses.
///
/// A major motivation of the optimization:
/// Consecutive bitfields are wrapped as a group and represented as a
/// large integer. To access a specific bitfield, wide load/store plus a
/// series of bit operations are needed. However, if the bitfield is of legal
/// type, it is more efficient to access it directly. Another case is the
/// bitfield group may be of illegal integer type, if we only access one
/// bitfield in the group, we may have chance to shrink the illegal type
/// to a smaller legal type, and save some bit mask operations.
/// Although the major motivation of the optimization is about bitfield
/// accesses, it is not limited to them and can be applied to general memory
/// accesses with similar patterns.
///
/// Description about the optimization:
/// The optimization have three basic categories. The first category is store
/// shrinking. If we are storing a large value, with some of its consecutive
/// bits replaced by a small value through bit manipulation, we can use two
/// stores, one for the large value and one for the small, to replace the
/// original store and bit manipulation operations. Even better if the large
/// value is got from a load with the same address as the store, the store
/// to the large value can be saved too.
///
/// The second category is another kind of store shrinking. We are looking
/// for the operations sequence like: load a large value, adjust some bits
/// of the value and then store it back to the same address as the load.
/// The essence of the operation sequence is to change some bits of the
/// large value but keep the rest part of it the same. We may shrink the
/// size of both the load and store, and the bit manipulation operations
/// in the middle of them, if only the bits to be changed are still included.
/// This is especially useful when the load and store are of illegal type.
/// Shrinking the load and store to a smaller legal type can save us many
/// extra operations to be generated in ISel legalization.
///
/// The third category is load use shrinking. If we are loading a large
/// value and using it to do a series of operations, but finally we only
/// use a small part of the result value, we can shrink the size of the
/// whole chain including the load.
///
/// Some notes about the algorithm:
/// * The algorithm scans the program and tries to recognize and transform the
///   patterns above. It runs iteratively until no more changes have happened.
/// * To prevent the optimization from blocking load/store coalescing, it is
///   invoked late in the pipeline, just before CodeGenPrepare. At this late
///   stage, both the pattern matching and related safety check become more
///   difficult because of previous optimizations introducing merged loads and
///   stores and more complex control flow. That is why MemorySSA is used here.
///   It provides scalable and precise safety checks even when we try to insert
///   a smaller access into a block which is many blocks away from the original
///   access.
//===----------------------------------------------------------------------===//

#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/MemorySSA.h"
#include "llvm/Analysis/MemorySSAUpdater.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/PatternMatch.h"
#include "llvm/Pass.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetLowering.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetSubtargetInfo.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Utils/Local.h"

#define DEBUG_TYPE "memaccessshrink"

using namespace llvm;
using namespace llvm::PatternMatch;

STATISTIC(NumStoreShrunkBySplit, "Number of stores shrunk by splitting");
STATISTIC(NumStoreShrunkToLegal, "Number of stores shrunk to legal types");
STATISTIC(NumLoadShrunk, "Number of Loads shrunk");

namespace {
/// Describe the bit range of a value.
struct BitRange {
  unsigned Shift;
  unsigned Width;
  bool isDisjoint(BitRange &BR);
};

/// \brief The memory access shrinking pass.
class MemAccessShrinkingPass : public FunctionPass {
public:
  static char ID; // Pass identification, replacement for typeid
  MemAccessShrinkingPass(const TargetMachine *TM = nullptr)
      : FunctionPass(ID), TM(TM) {
    initializeMemAccessShrinkingPassPass(*PassRegistry::getPassRegistry());
  }

  bool runOnFunction(Function &Fn) override;

  StringRef getPassName() const override { return "MemAccess Shrinking"; }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<DominatorTreeWrapperPass>();
    AU.addRequired<MemorySSAWrapperPass>();
    AU.addPreserved<MemorySSAWrapperPass>();
    AU.addRequired<TargetLibraryInfoWrapperPass>();
  }

  struct StoreShrunkInfo {
    StoreInst &SI;
    Value *LargeVal;
    Value *SmallVal;
    ConstantInt *Cst;
    BitRange &BR;
    StoreShrunkInfo(StoreInst &SI, Value *LargeVal, Value *SmallVal,
                    ConstantInt *Cst, BitRange &BR)
        : SI(SI), LargeVal(LargeVal), SmallVal(SmallVal), Cst(Cst), BR(BR) {}
  };

private:
  const DataLayout *DL = nullptr;
  const DominatorTree *DT = nullptr;
  const TargetMachine *TM = nullptr;
  const TargetLowering *TLI = nullptr;
  const TargetLibraryInfo *TLInfo = nullptr;
  LLVMContext *Context;

  MemorySSA *MSSA;
  MemorySSAWalker *MSSAWalker;
  std::unique_ptr<MemorySSAUpdater> MSSAUpdater;

  // Candidate instructions to be erased if they have no other uses.
  SmallPtrSet<Instruction *, 8> CandidatesToErase;

  // MultiUsesSeen shows a multiuse node is found on the Def-Use Chain.
  bool MultiUsesSeen;
  // SavedInsts shows how many instructions saved for a specific shrink so far.
  // It is used to evaluate whether some instructions will be saved for the
  // shrinking especially when MultiUsesSeen is true.
  unsigned SavedInsts;

  BitRange analyzeOrAndPattern(Value &SmallVal, ConstantInt &Cst,
                               unsigned BitSize, bool &DoReplacement);
  BitRange analyzeBOpPattern(Value &Val, ConstantInt &Cst, unsigned BitSize);
  bool extendBitRange(BitRange &BR, unsigned BitSize, unsigned Align);
  bool hasClobberBetween(Instruction &From, Instruction &To);
  bool needNewStore(Value &LargeVal, StoreInst &SI);
  Value *createNewPtr(Value *Ptr, unsigned StOffset, Type *NewTy,
                      IRBuilder<> &Builder);

  bool findBRWithLegalType(StoreInst &SI, BitRange &BR);
  bool isLegalToShrinkStore(LoadInst &LI, StoreInst &SI);
  bool tryShrinkStoreBySplit(StoreInst &SI);
  bool tryShrinkStoreToLegalTy(StoreInst &SI);
  bool splitStoreIntoTwo(StoreShrunkInfo &SInfo);
  bool shrinkStoreToLegalType(StoreShrunkInfo &SInfo);
  bool tryShrinkLoadUse(Instruction &IN);
  bool tryShrink(Function &Fn);
  Value *shrinkInsts(Value *NewPtr, BitRange &BR,
                     SmallVectorImpl<Instruction *> &Insts,
                     bool AllClobberInToBlock, unsigned ResShift,
                     IRBuilder<> &Builder);
  bool matchInstsToShrink(Value *Val, BitRange &BR,
                          SmallVectorImpl<Instruction *> &Insts);
  bool hasClobberBetween(Instruction &From, Instruction &To,
                         const MemoryLocation &Mem, bool &AllClobberInToBlock);
  void addSaveCandidate(Instruction &Inst, bool ReplaceAllUses = false);
  bool isBeneficial(unsigned ResShift, SmallVectorImpl<Instruction *> &Insts);
  void markCandForDeletion(Instruction &I) { CandidatesToErase.insert(&I); }
  bool removeDeadInsts();
};
} // namespace

char MemAccessShrinkingPass::ID = 0;
INITIALIZE_TM_PASS_BEGIN(MemAccessShrinkingPass, "memaccessshrink",
                         "MemAccess Shrinking", false, false)
INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
INITIALIZE_PASS_DEPENDENCY(MemorySSAWrapperPass)
INITIALIZE_PASS_DEPENDENCY(TargetLibraryInfoWrapperPass)
INITIALIZE_TM_PASS_END(MemAccessShrinkingPass, "memaccessshrink",
                       "MemAccess Shrinking", false, false)

FunctionPass *llvm::createMemAccessShrinkingPass(const TargetMachine *TM) {
  return new MemAccessShrinkingPass(TM);
}

/// Analyze pattern or(and(LargeVal, \p Cst), \p SmallVal). A common usage
/// of the pattern is to replace some consecutive bits in LargeVal with
/// nonzero bits of SmallVal, but to do the replacement as described,
/// some requirements have to be satisfied like that \p Cst has to contain
/// a consecutive run of zeros, and that all the nonzero bits of \p SmallVal
/// should be inside the range of consecutive zeros bits of \p Cst.
/// \p DoReplacement will contain the result of whether the requirements are
/// satisfied. We will also return BitRange describing the maximum range of
/// LargeVal that may be modified.
BitRange MemAccessShrinkingPass::analyzeOrAndPattern(Value &SmallVal,
                                                     ConstantInt &Cst,
                                                     unsigned BitSize,
                                                     bool &DoReplacement) {
  // Cst is the mask. Analyze the pattern of mask after sext it to uint64_t. We
  // will handle patterns like either 0..01..1 or 1..10..01..1
  APInt Mask = Cst.getValue().sextOrTrunc(BitSize);
  assert(Mask.getBitWidth() == BitSize && "Unexpected bitwidth of Mask");
  unsigned MaskLeadOnes = Mask.countLeadingOnes();
  unsigned MaskTrailOnes = Mask.countTrailingOnes();
  unsigned MaskMidZeros = !MaskLeadOnes
                              ? Mask.countLeadingZeros()
                              : Mask.ashr(MaskTrailOnes).countTrailingZeros();
  // Replace some consecutive bits in LargeVal with all the bits of SmallVal.
  DoReplacement = true;
  if (MaskLeadOnes == BitSize) {
    MaskMidZeros = 0;
    DoReplacement = false;
  } else if (MaskLeadOnes + MaskMidZeros + MaskTrailOnes != BitSize) {
    // See if we have a continuous run of zeros.
    MaskMidZeros = BitSize - MaskLeadOnes - MaskTrailOnes;
    DoReplacement = false;
  }

  // Check SmallVal only provides nonzero bits within range from lowbits
  // (MaskTrailOnes) to highbits (MaskTrailOnes + MaskMidZeros).
  APInt BitMask =
      ~APInt::getBitsSet(BitSize, MaskTrailOnes, MaskTrailOnes + MaskMidZeros);

  // Find out the range in which 1 appears in SmallVal.
  APInt KnownOne(BitSize, 0), KnownZero(BitSize, 0);
  computeKnownBits(&SmallVal, KnownZero, KnownOne, *DL, 0);

  BitRange BR;
  BR.Shift = MaskTrailOnes;
  BR.Width = MaskMidZeros;
  // Expect the bits being 1 in BitMask are all KnownZero bits in SmallVal,
  // otherwise we need to set ShrinkWithMaskedVal to false and expand BR.
  if ((KnownZero & BitMask) != BitMask) {
    DoReplacement = false;
    // Lower is the first bit for which we are not sure about the fact of
    // its being zero.
    unsigned Lower = KnownZero.countTrailingOnes();
    // Higher is the last bit for which we are not sure about the fact of
    // its being zero.
    unsigned Higher = BitSize - KnownZero.countLeadingOnes();
    BR.Shift = std::min(Lower, MaskTrailOnes);
    BR.Width = std::max(Higher, MaskTrailOnes + MaskMidZeros) - BR.Shift;
  }
  return BR;
}

/// Analyze pattern or/xor/and(load P, \p Cst). The result of the pattern
/// is a partially changed load P. The func will return the the range
/// showing where the load result are changed.
BitRange MemAccessShrinkingPass::analyzeBOpPattern(Value &Val, ConstantInt &Cst,
                                                   unsigned BitSize) {
  APInt Mask = Cst.getValue().sextOrTrunc(BitSize);
  BinaryOperator *BOP = cast<BinaryOperator>(&Val);
  if (BOP->getOpcode() == Instruction::And)
    Mask = ~Mask;

  BitRange BR;
  BR.Shift = Mask.countTrailingZeros();
  BR.Width = Mask.getBitWidth() - BR.Shift;
  if (BR.Width)
    BR.Width = BR.Width - Mask.countLeadingZeros();
  return BR;
}

/// Extend \p BR so the new BR.Width bits can form a legal type.
bool MemAccessShrinkingPass::extendBitRange(BitRange &BR, unsigned BitSize,
                                            unsigned Align) {
  BitRange NewBR;
  NewBR.Width = PowerOf2Ceil(BR.Width);
  Type *NewTy = Type::getIntNTy(*Context, NewBR.Width);
  Type *StoreTy = Type::getIntNTy(*Context, PowerOf2Ceil(BitSize));

  // Check if we can find a new Shift for the Width of NewBR, so that
  // NewBR forms a new range covering the old modified range without
  // worsening alignment.
  auto coverOldRange = [&](BitRange &NewBR, BitRange &BR) -> bool {
    unsigned MAlign = MinAlign(Align, DL->getABITypeAlignment(NewTy));
    int Shift = BR.Shift - BR.Shift % (MAlign * 8);
    while (Shift >= 0) {
      if (NewBR.Width + Shift <= BitSize &&
          NewBR.Width + Shift >= BR.Width + BR.Shift) {
        NewBR.Shift = Shift;
        return true;
      }
      Shift -= MAlign * 8;
    }
    return false;
  };
  // See whether we can store NewTy legally.
  auto isStoreLegalType = [&](Type *NewTy) -> bool {
    EVT OldEVT = TLI->getValueType(*DL, StoreTy);
    EVT NewEVT = TLI->getValueType(*DL, NewTy);
    return TLI->isOperationLegalOrCustom(ISD::STORE, NewEVT) ||
           TLI->isTruncStoreLegalOrCustom(OldEVT, NewEVT);
  };

  // Try to find the minimal NewBR.Width which can form a legal type and cover
  // all the old modified bits.
  while (NewBR.Width < BitSize &&
         (!isStoreLegalType(NewTy) ||
          TLI->isNarrowingExpensive(EVT::getEVT(StoreTy), EVT::getEVT(NewTy)) ||
          !coverOldRange(NewBR, BR))) {
    NewBR.Width = NextPowerOf2(NewBR.Width);
    NewTy = Type::getIntNTy(*Context, NewBR.Width);
  }
  BR.Width = NewBR.Width;
  BR.Shift = NewBR.Shift;

  if (BR.Width >= BitSize)
    return false;
  return true;
}

/// Compute the offset used to compute the new ptr address. It will be
/// mainly based on BR and the endian of the target.
static unsigned computeBitOffset(BitRange &BR, unsigned BitSize,
                                 const DataLayout &DL) {
  unsigned BitOffset;
  if (DL.isBigEndian())
    BitOffset = BitSize - BR.Shift - BR.Width;
  else
    BitOffset = BR.Shift;

  return BitOffset;
}

static unsigned computeByteOffset(BitRange &BR, unsigned BitSize,
                                  const DataLayout &DL) {
  return computeBitOffset(BR, BitSize, DL) / 8;
}

/// Check whether V1 and V2 has the same ptr value by looking through bitcast.
static bool hasSamePtr(Value *V1, Value *V2) {
  Value *NV1 = nullptr;
  Value *NV2 = nullptr;
  if (V1 == V2)
    return true;
  if (BitCastInst *BC1 = dyn_cast<BitCastInst>(V1))
    NV1 = BC1->getOperand(0);
  if (BitCastInst *BC2 = dyn_cast<BitCastInst>(V2))
    NV2 = BC2->getOperand(0);
  if (!NV1 && !NV2)
    return false;
  if (V1 == NV2 || V2 == NV1 || NV1 == NV2)
    return true;
  return false;
}

/// Check whether there is any clobber instruction between \p From and \p To
/// for the memory location accessed by \p To.
bool MemAccessShrinkingPass::hasClobberBetween(Instruction &From,
                                               Instruction &To) {
  assert(MSSA->getMemoryAccess(&To) && "Expect To has valid MemoryAccess");
  MemoryAccess *FromAccess = MSSA->getMemoryAccess(&From);
  assert(FromAccess && "Expect From has valid MemoryAccess");
  MemoryAccess *DefiningAccess = MSSAWalker->getClobberingMemoryAccess(&To);
  if (FromAccess != DefiningAccess &&
      MSSA->dominates(FromAccess, DefiningAccess))
    return true;
  return false;
}

/// It is possible that we already have a store of LargeVal and it is not
/// clobbered, then we can use it and don't have to generate a a new store.
bool MemAccessShrinkingPass::needNewStore(Value &LargeVal, StoreInst &SI) {
  for (User *U : LargeVal.users()) {
    StoreInst *PrevSI = dyn_cast<StoreInst>(U);
    if (!PrevSI ||
        !hasSamePtr(PrevSI->getPointerOperand(), SI.getPointerOperand()) ||
        PrevSI->getValueOperand()->getType() != SI.getValueOperand()->getType())
      continue;
    if (!hasClobberBetween(*PrevSI, SI))
      return false;
  }
  return true;
}

/// Create new address to be used by shrunk load/store based on original
/// address \p Ptr, offset \p StOffset and the new type \p NewTy.
Value *MemAccessShrinkingPass::createNewPtr(Value *Ptr, unsigned StOffset,
                                            Type *NewTy, IRBuilder<> &Builder) {
  Value *NewPtr = Ptr;
  unsigned AS = cast<PointerType>(Ptr->getType())->getAddressSpace();
  if (StOffset) {
    ConstantInt *Idx = ConstantInt::get(Type::getInt32Ty(*Context), StOffset);
    NewPtr =
        Builder.CreateBitCast(Ptr, Type::getInt8PtrTy(*Context, AS), "cast");
    NewPtr =
        Builder.CreateGEP(Type::getInt8Ty(*Context), NewPtr, Idx, "uglygep");
  }
  return Builder.CreateBitCast(NewPtr, NewTy->getPointerTo(AS), "cast");
}

/// Check if it is legal to shrink the store if the input LargeVal is a
/// LoadInst.
bool MemAccessShrinkingPass::isLegalToShrinkStore(LoadInst &LI, StoreInst &SI) {
  Value *Ptr = SI.getOperand(1);
  // LI should have the same address as SI.
  if (!hasSamePtr(LI.getPointerOperand(), Ptr))
    return false;
  if (LI.isVolatile() || !LI.isUnordered())
    return false;
  // Make sure the memory location of LI/SI is not clobbered between them.
  if (hasClobberBetween(LI, SI))
    return false;
  return true;
}

/// we try to handle is when a smaller value is "inserted" into some bits
/// of a larger value. This typically looks something like:
///
///   store(or(and(LargeVal, MaskConstant), SmallVal)), address)
///
/// Here, we try to use a narrow store of `SmallVal` rather than bit
/// operations to combine them:
///
///   store(LargeVal, address)
///   store(SmallVal, address)
///
/// Or if `LargeVal` was a load, we may not need to store it at all:
///
///   store(SmallVal, address)
///
/// We may also be able incorporate shifts of `SmallVal` by using an offset
/// of `address`.
///
/// However, this may not be valid if, for example, the size of `SmallVal`
/// isn't a valid type to store or the shift can't be modeled as a valid
/// offset from the address, then we can still try another transformation
/// which is to shrink the store to smaller legal width when the original
/// width of the store is illegal.
bool MemAccessShrinkingPass::tryShrinkStoreBySplit(StoreInst &SI) {
  Value *Val = SI.getOperand(0);
  Type *StoreTy = Val->getType();
  if (StoreTy->isVectorTy() || !StoreTy->isIntegerTy())
    return false;
  if (SI.isVolatile() || !SI.isUnordered())
    return false;
  unsigned BitSize = DL->getTypeSizeInBits(StoreTy);
  if (BitSize != DL->getTypeStoreSizeInBits(StoreTy))
    return false;

  Value *LargeVal;
  Value *SmallVal;
  ConstantInt *Cst;
  if (!(match(Val, m_Or(m_And(m_Value(LargeVal), m_ConstantInt(Cst)),
                        m_Value(SmallVal))) &&
        match(LargeVal, m_c_Or(m_And(m_Value(), m_Value()), m_Value()))) &&
      !(match(Val, m_Or(m_Value(SmallVal),
                        m_And(m_Value(LargeVal), m_ConstantInt(Cst)))) &&
        match(LargeVal, m_c_Or(m_And(m_Value(), m_Value()), m_Value()))) &&
      !match(Val, m_c_Or(m_And(m_Value(LargeVal), m_ConstantInt(Cst)),
                         m_Value(SmallVal))))
    return false;

  LoadInst *LI = dyn_cast<LoadInst>(LargeVal);
  if (LI && !isLegalToShrinkStore(*LI, SI))
    return false;

  // Return BR which indicates the range of the LargeVal that will actually
  // be modified. If the pattern does the work to replace portion of LargeVal
  // with bits from SmallVal, we can do the split store transformation.
  // Otherwise, we can only try shrink the store to legal type transformation.
  // ToSplitStore will be populated to reflect that.
  bool ToSplitStore;
  BitRange BR = analyzeOrAndPattern(*SmallVal, *Cst, BitSize, ToSplitStore);
  assert(BR.Shift + BR.Width <= BitSize && "Unexpected BitRange!");
  if (!BR.Width)
    return false;

  if (ToSplitStore) {
    // Get the offset from Ptr for the shrunk store.
    unsigned StoreBitOffset = computeBitOffset(BR, BitSize, *DL);
    if (StoreBitOffset % 8 != 0)
      ToSplitStore = false;

    // If BR.Width is not the length of legal integer type, we cannot
    // store SmallVal directly.
    if (BR.Width != 8 && BR.Width != 16 && BR.Width != 32 && BR.Width != 64)
      ToSplitStore = false;
  }

  if (!ToSplitStore) {
    if (!LI)
      return false;
    if (!findBRWithLegalType(SI, BR))
      return false;
    StoreShrunkInfo SInfo(SI, LI, SmallVal, Cst, BR);
    shrinkStoreToLegalType(SInfo);
    return true;
  }
  IntegerType *NewTy = Type::getIntNTy(*Context, BR.Width);
  if (TLI->isNarrowingExpensive(EVT::getEVT(StoreTy), EVT::getEVT(NewTy)))
    return false;

  StoreShrunkInfo SInfo(SI, LargeVal, SmallVal, Cst, BR);
  splitStoreIntoTwo(SInfo);
  return true;
}

/// The pattern we try to shrink (which may also apply to code matching
/// the tryShrinkStoreBySplit pattern when that transformation isn't valid)
/// is to shrink the inputs of basic bit operations (or, and, xor) and then
/// store the smaller width result. This is valid whenever we know that
/// shrinking the inputs and operations doesn't change the result. For example,
/// if the removed bits are known to be 0s, 1s, or undef, we may be able to
/// avoid the bitwise computation on them. This is particularly useful when
/// the type width is not a legal type, and when the inputs are loads and
/// constants.
bool MemAccessShrinkingPass::tryShrinkStoreToLegalTy(StoreInst &SI) {
  Value *Val = SI.getOperand(0);
  Type *StoreTy = Val->getType();
  if (StoreTy->isVectorTy() || !StoreTy->isIntegerTy())
    return false;
  if (SI.isVolatile() || !SI.isUnordered())
    return false;
  unsigned BitSize = DL->getTypeSizeInBits(StoreTy);
  if (BitSize != DL->getTypeStoreSizeInBits(StoreTy))
    return false;

  LoadInst *LI;
  ConstantInt *Cst;
  if (!match(Val, m_c_Or(m_Load(LI), m_ConstantInt(Cst))) &&
      !match(Val, m_c_And(m_Load(LI), m_ConstantInt(Cst))) &&
      !match(Val, m_c_Xor(m_Load(LI), m_ConstantInt(Cst))))
    return false;

  if (!isLegalToShrinkStore(*LI, SI))
    return false;

  // Find out the range of Val which is changed.
  BitRange BR = analyzeBOpPattern(*Val, *Cst, BitSize);
  assert(BR.Shift + BR.Width <= BitSize && "Unexpected BitRange!");
  if (!BR.Width)
    return false;
  if (!findBRWithLegalType(SI, BR))
    return false;

  StoreShrunkInfo SInfo(SI, LI, nullptr, Cst, BR);
  shrinkStoreToLegalType(SInfo);
  return true;
}

/// Try to extend the existing BitRange \BR to legal integer width.
bool MemAccessShrinkingPass::findBRWithLegalType(StoreInst &SI, BitRange &BR) {
  Type *StoreTy = SI.getOperand(0)->getType();
  unsigned Align = SI.getAlignment();
  unsigned BitSize = DL->getTypeSizeInBits(StoreTy);
  // Check whether the store is of illegal type. If it is not, don't bother.
  if (!TLI || TLI->isOperationLegalOrCustom(ISD::STORE,
                                            TLI->getValueType(*DL, StoreTy)))
    return false;
  // Try to find out a BR of legal type.
  if (!extendBitRange(BR, BitSize, Align))
    return false;
  return true;
}

/// The transformation to split the store into two: one is for the LargeVal
/// and one is for the SmallVal. The first store can be saved if the LargeVal
/// is got from a load or there is an existing store for the LargeVal.
bool MemAccessShrinkingPass::splitStoreIntoTwo(StoreShrunkInfo &SInfo) {
  BitRange &BR = SInfo.BR;
  StoreInst &SI = SInfo.SI;
  IRBuilder<> Builder(*Context);
  Builder.SetInsertPoint(&SI);
  Value *Val = SI.getOperand(0);
  Value *Ptr = SI.getOperand(1);

  Type *StoreTy = SI.getOperand(0)->getType();
  unsigned BitSize = DL->getTypeSizeInBits(StoreTy);
  unsigned StoreByteOffset = computeByteOffset(BR, BitSize, *DL);
  unsigned Align = SI.getAlignment();
  if (StoreByteOffset)
    Align = MinAlign(StoreByteOffset, Align);

  IntegerType *NewTy = Type::getIntNTy(*Context, BR.Width);
  Value *NewPtr = createNewPtr(Ptr, StoreByteOffset, NewTy, Builder);

  // Check if we need to split the original store and generate a new
  // store for the LargeVal.
  if (!isa<LoadInst>(SInfo.LargeVal) && needNewStore(*SInfo.LargeVal, SI)) {
    StoreInst *NewSI = cast<StoreInst>(SI.clone());
    NewSI->setOperand(0, SInfo.LargeVal);
    NewSI->setOperand(1, Ptr);
    Builder.Insert(NewSI);
    DEBUG(dbgs() << "MemShrink: Insert" << *NewSI << " before" << SI << "\n");
    // MemorySSA update for the new store.
    MemoryDef *OldMemAcc = cast<MemoryDef>(MSSA->getMemoryAccess(&SI));
    MemoryAccess *Def = OldMemAcc->getDefiningAccess();
    MemoryDef *NewMemAcc = cast<MemoryDef>(
        MSSAUpdater->createMemoryAccessBefore(NewSI, Def, OldMemAcc));
    MSSAUpdater->insertDef(NewMemAcc, false);
  }

  // Create the New Value to store.
  Value *SmallVal = nullptr;
  if (auto MVCst = dyn_cast<ConstantInt>(SInfo.SmallVal)) {
    APInt ModifiedCst = MVCst->getValue().lshr(BR.Shift).trunc(BR.Width);
    SmallVal = ConstantInt::get(*Context, ModifiedCst);
  } else {
    Value *ShiftedVal =
        BR.Shift ? Builder.CreateLShr(SInfo.SmallVal, BR.Shift, "lshr")
                 : SInfo.SmallVal;
    SmallVal = Builder.CreateTruncOrBitCast(ShiftedVal, NewTy, "trunc");
  }

  // Create the new store and remove the old one.
  StoreInst *NewSI = cast<StoreInst>(SI.clone());
  NewSI->setOperand(0, SmallVal);
  NewSI->setOperand(1, NewPtr);
  NewSI->setAlignment(Align);
  Builder.Insert(NewSI);

  DEBUG(dbgs() << "MemShrink: Replace" << SI << " with" << *NewSI << "\n");
  // MemorySSA update for the shrunk store.
  MemoryDef *OldMemAcc = cast<MemoryDef>(MSSA->getMemoryAccess(&SI));
  MemoryAccess *Def = OldMemAcc->getDefiningAccess();
  MemoryAccess *NewMemAcc =
      MSSAUpdater->createMemoryAccessBefore(NewSI, Def, OldMemAcc);
  OldMemAcc->replaceAllUsesWith(NewMemAcc);
  MSSAUpdater->removeMemoryAccess(OldMemAcc);

  markCandForDeletion(*cast<Instruction>(Val));
  SI.eraseFromParent();
  NumStoreShrunkBySplit++;
  return true;
}

/// The transformation to shrink the value of the store to smaller width.
bool MemAccessShrinkingPass::shrinkStoreToLegalType(StoreShrunkInfo &SInfo) {
  BitRange &BR = SInfo.BR;
  StoreInst &SI = SInfo.SI;
  IRBuilder<> Builder(*Context);
  Builder.SetInsertPoint(&SI);
  Value *Val = SI.getOperand(0);
  Value *Ptr = SI.getOperand(1);

  Type *StoreTy = SI.getOperand(0)->getType();
  unsigned BitSize = DL->getTypeSizeInBits(StoreTy);
  unsigned StoreByteOffset = computeByteOffset(BR, BitSize, *DL);
  unsigned Align = SI.getAlignment();
  if (StoreByteOffset)
    Align = MinAlign(StoreByteOffset, Align);

  IntegerType *NewTy = Type::getIntNTy(*Context, BR.Width);
  Value *NewPtr = createNewPtr(Ptr, StoreByteOffset, NewTy, Builder);

  LoadInst *NewLI = cast<LoadInst>(cast<LoadInst>(SInfo.LargeVal)->clone());
  NewLI->mutateType(NewTy);
  NewLI->setOperand(0, NewPtr);
  NewLI->setAlignment(Align);
  Builder.Insert(NewLI, "load.trunc");
  // MemorySSA update for the shrunk load.
  MemoryDef *OldMemAcc = cast<MemoryDef>(MSSA->getMemoryAccess(&SI));
  MemoryAccess *Def = OldMemAcc->getDefiningAccess();
  MSSAUpdater->createMemoryAccessBefore(NewLI, Def, OldMemAcc);

  // Create the SmallVal to store.
  Value *SmallVal = nullptr;
  APInt ModifiedCst = SInfo.Cst->getValue().lshr(BR.Shift).trunc(BR.Width);
  ConstantInt *NewCst = ConstantInt::get(*Context, ModifiedCst);
  if (SInfo.SmallVal) {
    // Use SInfo.SmallVal to get the SmallVal.
    Value *Trunc;
    if (auto MVCst = dyn_cast<ConstantInt>(SInfo.SmallVal)) {
      ModifiedCst = MVCst->getValue().lshr(BR.Shift).trunc(BR.Width);
      Trunc = ConstantInt::get(*Context, ModifiedCst);
    } else {
      Value *ShiftedVal =
          BR.Shift ? Builder.CreateLShr(SInfo.SmallVal, BR.Shift, "lshr")
                   : SInfo.SmallVal;
      Trunc = Builder.CreateTruncOrBitCast(ShiftedVal, NewTy, "trunc");
    }
    Value *NewAnd = Builder.CreateAnd(NewLI, NewCst, "and.trunc");
    SmallVal = Builder.CreateOr(NewAnd, Trunc, "or.trunc");
  } else {
    BinaryOperator *BOP = cast<BinaryOperator>(Val);
    switch (BOP->getOpcode()) {
    default:
      llvm_unreachable("BOP can only be And/Or/Xor");
    case Instruction::And:
      SmallVal = Builder.CreateAnd(NewLI, NewCst, "and.trunc");
      break;
    case Instruction::Or:
      SmallVal = Builder.CreateOr(NewLI, NewCst, "or.trunc");
      break;
    case Instruction::Xor:
      SmallVal = Builder.CreateXor(NewLI, NewCst, "xor.trunc");
      break;
    }
  }

  // Create the new store and remove the old one.
  StoreInst *NewSI = cast<StoreInst>(SI.clone());
  NewSI->setOperand(0, SmallVal);
  NewSI->setOperand(1, NewPtr);
  NewSI->setAlignment(Align);
  Builder.Insert(NewSI);

  DEBUG(dbgs() << "MemShrink: Replace" << SI << " with" << *NewSI << "\n");
  // MemorySSA update for the shrunk store.
  OldMemAcc = cast<MemoryDef>(MSSA->getMemoryAccess(&SI));
  Def = OldMemAcc->getDefiningAccess();
  MemoryAccess *NewMemAcc =
      MSSAUpdater->createMemoryAccessBefore(NewSI, Def, OldMemAcc);
  OldMemAcc->replaceAllUsesWith(NewMemAcc);
  MSSAUpdater->removeMemoryAccess(OldMemAcc);

  markCandForDeletion(*cast<Instruction>(Val));
  SI.eraseFromParent();
  NumStoreShrunkToLegal++;
  return true;
}

/// AI contains a consecutive run of 1. Check the range \p BR of \p AI are all
/// 1s.
static bool isRangeofAIAllOnes(BitRange &BR, APInt &AI) {
  if (BR.Shift < AI.countTrailingZeros() ||
      BR.Width + BR.Shift > (AI.getBitWidth() - AI.countLeadingZeros()))
    return false;
  return true;
}

bool BitRange::isDisjoint(BitRange &BR) {
  return (Shift > BR.Shift + BR.Width - 1) || (BR.Shift > Shift + Width - 1);
}

/// Check if there is no instruction between \p From and \p To which may
/// clobber the MemoryLocation \p Mem. However, if there are clobbers and
/// all the clobber instructions between \p From and \p To are in the same
/// block as \p To, We will set \p AllClobberInToBlock to true.
bool MemAccessShrinkingPass::hasClobberBetween(Instruction &From,
                                               Instruction &To,
                                               const MemoryLocation &Mem,
                                               bool &AllClobberInToBlock) {
  assert(DT->dominates(&From, &To) && "From doesn't dominate To");
  const BasicBlock *FBB = From.getParent();
  const BasicBlock *TBB = To.getParent();
  // Collect BasicBlocks to scan between FBB and TBB into BBSet.
  SmallPtrSet<const BasicBlock *, 4> BBSet;
  SmallVector<const BasicBlock *, 4> Worklist;
  BBSet.insert(FBB);
  BBSet.insert(TBB);
  Worklist.push_back(TBB);
  do {
    const BasicBlock *BB = Worklist.pop_back_val();
    for (const BasicBlock *Pred : predecessors(BB)) {
      if (!BBSet.count(Pred)) {
        BBSet.insert(Pred);
        Worklist.push_back(Pred);
      }
    }
  } while (!Worklist.empty());

  // Check the DefsList inside of each BB.
  bool hasClobber = false;
  AllClobberInToBlock = true;
  for (const BasicBlock *BB : BBSet) {
    const MemorySSA::DefsList *DList = MSSA->getBlockDefs(BB);
    if (!DList)
      continue;

    for (const MemoryAccess &MA : *DList) {
      if (const MemoryDef *MD = dyn_cast<MemoryDef>(&MA)) {
        Instruction *Inst = MD->getMemoryInst();
        if ((FBB == Inst->getParent() && DT->dominates(Inst, &From)) ||
            (TBB == Inst->getParent() && DT->dominates(&To, Inst)))
          continue;
        // Check whether MD is a clobber of Mem.
        if (MSSAWalker->getClobberingMemoryAccess(const_cast<MemoryDef *>(MD),
                                                  Mem) == MD) {
          if (Inst->getParent() != TBB)
            AllClobberInToBlock = false;
          hasClobber = true;
        }
      }
    }
  }
  if (!hasClobber)
    AllClobberInToBlock = false;
  return hasClobber;
}

/// Match \p Val to the pattern:
/// Bop(...Bop(V, Cst_1), ..., Cst_N) and pattern:
/// or(and(...or(and(LI, Cst_1), SmallVal_1), ..., Cse_N), SmallVal_N),
/// and find the final load to be shrunk.
/// All the Bop instructions in the first pattern and the final load will
/// be added into \p Insts and will be shrunk afterwards.
bool MemAccessShrinkingPass::matchInstsToShrink(
    Value *Val, BitRange &BR, SmallVectorImpl<Instruction *> &Insts) {
  unsigned BitSize = DL->getTypeSizeInBits(Val->getType());

  // Match the pattern:
  // Bop(...Bop(V, Cst_1), Cst_2, ..., Cst_N), Bop can be Add/Sub/And/Or/Xor.
  // Those BinaryOperator instructions are easy to be shrunk to BR range.
  Value *Opd;
  ConstantInt *Cst;
  while (match(Val, m_Add(m_Value(Opd), m_ConstantInt(Cst))) ||
         match(Val, m_Sub(m_Value(Opd), m_ConstantInt(Cst))) ||
         match(Val, m_And(m_Value(Opd), m_ConstantInt(Cst))) ||
         match(Val, m_Or(m_Value(Opd), m_ConstantInt(Cst))) ||
         match(Val, m_Xor(m_Value(Opd), m_ConstantInt(Cst)))) {
    Instruction *BOP = cast<Instruction>(Val);
    addSaveCandidate(*BOP);
    Insts.push_back(BOP);
    Val = Opd;
  }

  LoadInst *LI;
  if ((LI = dyn_cast<LoadInst>(Val))) {
    addSaveCandidate(*LI);
    Insts.push_back(LI);
    return true;
  }

  // Match the pattern:
  // or(and(...or(and(LI, Cst_1), SmallVal_1), ..., Cse_N), SmallVal_N).
  // Analyze the range of LargeVal which may be modified by the current
  // or(and(LargeVal, Cst), SmallVal)) operations. If each or(and(...))
  // pair modifies a BitRange which is disjoint with current BR, we can
  // skip all these operations when we shrink the final load, so we only
  // have to add the final load into Insts.
  Value *LargeVal;
  Value *SmallVal;
  BitRange OtherBR;
  while (match(Val, m_c_Or(m_And(m_Load(LI), m_ConstantInt(Cst)),
                           m_Value(SmallVal))) ||
         (match(Val, m_Or(m_And(m_Value(LargeVal), m_ConstantInt(Cst)),
                          m_Value(SmallVal))) &&
          match(LargeVal, m_c_Or(m_And(m_Value(), m_Value()), m_Value()))) ||
         (match(Val, m_Or(m_Value(SmallVal),
                          m_And(m_Value(LargeVal), m_ConstantInt(Cst)))) &&
          match(LargeVal, m_c_Or(m_And(m_Value(), m_Value()), m_Value())))) {
    addSaveCandidate(*cast<Instruction>(Val));
    bool DoReplacement;
    OtherBR.Shift = 0;
    OtherBR.Width = BitSize;
    // Analyze BitRange of current or(and(LargeVal, Cst), SmallVal) operations.
    OtherBR = analyzeOrAndPattern(*SmallVal, *Cst, BitSize, DoReplacement);
    // If OtherBR is not disjoint with BR, we cannot shrink load to its BR
    // portion.
    if (!BR.isDisjoint(OtherBR))
      return false;
    if (LI) {
      Insts.push_back(LI);
      return true;
    }
    Val = LargeVal;
  }

  return false;
}

/// Find the next MemoryAccess after LI in the same BB.
MemoryAccess *findNextMemoryAccess(LoadInst &LI, MemorySSA &MSSA) {
  for (auto Scan = LI.getIterator(); Scan != LI.getParent()->end(); ++Scan) {
    if (MemoryAccess *MA = MSSA.getMemoryAccess(&*Scan))
      return MA;
  }
  return nullptr;
}

/// Shrink \p Insts according to the range BR.
Value *MemAccessShrinkingPass::shrinkInsts(
    Value *NewPtr, BitRange &BR, SmallVectorImpl<Instruction *> &Insts,
    bool AllClobberInToBlock, unsigned ResShift, IRBuilder<> &Builder) {
  Value *NewVal;
  IntegerType *NewTy = Type::getIntNTy(*Context, BR.Width);
  Instruction &InsertPt = *Builder.GetInsertPoint();
  for (Instruction *I : make_range(Insts.rbegin(), Insts.rend())) {
    if (LoadInst *LI = dyn_cast<LoadInst>(I)) {
      unsigned BitSize = DL->getTypeSizeInBits(LI->getType());
      unsigned LoadByteOffset = computeByteOffset(BR, BitSize, *DL);
      unsigned Align = MinAlign(LoadByteOffset, LI->getAlignment());
      Instruction &NewInsertPt = *(InsertPt.getParent()->getFirstInsertionPt());
      if (AllClobberInToBlock && (&InsertPt != &NewInsertPt)) {
        // If we use a new insertion point, we have to recreate the NewPtr in
        // the new place.
        Builder.SetInsertPoint(&NewInsertPt);
        RecursivelyDeleteTriviallyDeadInstructions(NewPtr);
        NewPtr = createNewPtr(LI->getPointerOperand(), LoadByteOffset, NewTy,
                              Builder);
      }
      // Create shrunk load.
      LoadInst *NewLI = cast<LoadInst>(LI->clone());
      NewLI->setOperand(0, NewPtr);
      NewLI->mutateType(NewTy);
      NewLI->setAlignment(Align);
      Builder.Insert(NewLI, "load.trunc");
      NewVal = NewLI;
      // Update MemorySSA.
      MemoryUseOrDef *OldMemAcc =
          cast<MemoryUseOrDef>(MSSA->getMemoryAccess(LI));
      MemoryAccess *Def = OldMemAcc->getDefiningAccess();
      // Find a proper position to insert the new MemoryAccess.
      MemoryAccess *Next = findNextMemoryAccess(*NewLI, *MSSA);
      if (Next)
        MSSAUpdater->createMemoryAccessBefore(NewLI, Def,
                                              cast<MemoryUseOrDef>(Next));
      else
        MSSAUpdater->createMemoryAccessInBB(NewLI, Def, NewLI->getParent(),
                                            MemorySSA::End);
    } else {
      // shrink Add/Sub/And/Xor/Or in Insts.
      BinaryOperator *BOP = cast<BinaryOperator>(I);
      ConstantInt *Cst = cast<ConstantInt>(BOP->getOperand(1));
      APInt NewAPInt = Cst->getValue().extractBits(BR.Width, BR.Shift);
      ConstantInt *NewCst =
          ConstantInt::get(*Context, NewAPInt.zextOrTrunc(BR.Width));
      NewVal = Builder.CreateBinOp(BOP->getOpcode(), NewVal, NewCst);
    }
  }
  // Adjust the type to be consistent with the input instruction.
  IntegerType *UseType = cast<IntegerType>(InsertPt.getType());
  if (DL->getTypeSizeInBits(UseType) != BR.Width)
    NewVal = Builder.CreateZExt(NewVal, UseType, "trunc.zext");
  // Adjust the NewVal with ResShift.
  if (ResShift) {
    ConstantInt *NewCst = ConstantInt::get(UseType, ResShift);
    NewVal = Builder.CreateShl(NewVal, NewCst, "shl");
  }
  return NewVal;
}

/// Determine whether \p Inst can be counted as a saved instruction when we
/// are evaluate the benefit. If \p ReplaceAllUses is true, it means we are
/// going to replace all the uses of \p Inst with the shrunk result, so it
/// can still be counted as a saved instruction even if it has multiple uses.
void MemAccessShrinkingPass::addSaveCandidate(Instruction &Inst,
                                              bool ReplaceAllUses) {
  if (!MultiUsesSeen) {
    // If the Inst has multiple uses and the current shrinking cannot replace
    // all its uses, it will not regarded as a SavedInst.
    if (ReplaceAllUses || Inst.hasOneUse())
      SavedInsts += (!isa<CastInst>(Inst) && !isa<TruncInst>(Inst));
    else
      MultiUsesSeen = true;
  }
}

/// Compare \p SavedInsts with instructions we are about to create and decide
/// whether it is beneficial to do the shrinking.
bool MemAccessShrinkingPass::isBeneficial(
    unsigned ResShift, SmallVectorImpl<Instruction *> &Insts) {
  unsigned InstsToCreate = 0;
  if (ResShift)
    InstsToCreate++;
  InstsToCreate += Insts.size();
  return SavedInsts >= InstsToCreate;
}

/// When the input instruction \p IN is and(Val, Cst) or trunc, it indicates
/// only a portion of its input value has been used. We will walk through the
/// Def-Use chain, track the range of value which will be used, remember the
/// operations contributing to the used value range, and skip operations which
/// changes value range that is not to be used, until a load is found.
///
/// So we start from and or trunc operations, then try to find a sequence of
/// shl or lshr operations. Those shifts are only changing the valid range
/// of input value.
///
/// After we know the valid range of input value, we will collect the sequence
/// of binary operators, which we want to shrink their input to the same range.
/// To make it simpler, we requires one of the operands of any binary operator
/// has to be constant.
///
/// Then we look for the pattern of or(and(LargeVal, Cst), SmallVal), which
/// will only change a portion of the input LargeVal and keep the rest of it
/// the same. If the operations pattern doesn't change the valid range, we can
/// use LargeVal as input and skip all the operations here.
///
/// In the end, we look for a load which can provide the valid range directly
/// after shifting the address and shrinking the size of the load.
///
/// An example:
///   and(lshr(add(or(and(load P, -65536), SmallVal), 0x1000000000000), 48),
///   255)
///
/// The actual valid range of the load is [48, 56). The value in the range is
/// incremented by 1. The sequence above will be transformed to:
///   zext(add(load_i8(P + 48), 1), 64)
///
bool MemAccessShrinkingPass::tryShrinkLoadUse(Instruction &IN) {
  // If the instruction is actually dead, skip it.
  if (IN.use_empty())
    return false;

  Type *Ty = IN.getType();
  if (Ty->isVectorTy() || !Ty->isIntegerTy())
    return false;

  MultiUsesSeen = false;
  SavedInsts = 0;

  // Match and(Val, Cst) or Trunc(Val)
  Value *Val;
  ConstantInt *AndCst = nullptr;
  if (!match(&IN, m_Trunc(m_Value(Val))) &&
      !match(&IN, m_And(m_Value(Val), m_ConstantInt(AndCst))))
    return false;
  addSaveCandidate(IN, true);

  // Get the initial BR showing the range of Val to be used.
  BitRange BR;
  BR.Shift = 0;
  unsigned ResShift = 0;
  unsigned BitSize = DL->getTypeSizeInBits(Val->getType());
  if (BitSize % 8 != 0)
    return false;
  if (AndCst) {
    APInt AI = AndCst->getValue();
    // Check AI has consecutive one bits. The consecutive bits are the
    // range to be used. If the num of trailing zeros are not zero,
    // remember the num in ResShift and the val after shrinking needs
    // to be shifted accordingly.
    BR.Shift = AI.countTrailingZeros();
    ResShift = BR.Shift;
    if (!(AI.lshr(BR.Shift) + 1).isPowerOf2() || BR.Shift == BitSize)
      return false;
    BR.Width = AI.getActiveBits() - BR.Shift;
  } else {
    BR.Width = DL->getTypeSizeInBits(Ty);
  }

  // Match a series of LShr or Shl. Adjust BR.Shift accordingly.
  // Note we have to be careful that valid bits may be cleared during
  // back-and-force shifts. We use an all-one APInt Mask to simulate
  // the shifts and track the valid bits.
  Value *Opd;
  unsigned NewShift = BR.Shift;
  APInt Mask(BitSize, -1);
  ConstantInt *Cst;
  // Record the series of LShr or Shl in ShiftRecs.
  SmallVector<std::pair<bool, unsigned>, 4> ShiftRecs;
  bool isLShr;
  while ((isLShr = match(Val, m_LShr(m_Value(Opd), m_ConstantInt(Cst)))) ||
         match(Val, m_Shl(m_Value(Opd), m_ConstantInt(Cst)))) {
    addSaveCandidate(*cast<Instruction>(Val));
    NewShift = isLShr ? (NewShift + Cst->getZExtValue())
                      : (NewShift - Cst->getZExtValue());
    ShiftRecs.push_back({isLShr, Cst->getZExtValue()});
    Val = Opd;
  }
  // Iterate ShiftRecs in reverse order. Simulate the shifts of Mask.
  for (auto SR : make_range(ShiftRecs.rbegin(), ShiftRecs.rend())) {
    if (SR.first)
      Mask = Mask.lshr(SR.second);
    else
      Mask = Mask.shl(SR.second);
  }
  // Mask contains a consecutive run of 1s. If the range BR of Mask are all
  // 1s, BR is valid after the shifts.
  if (!isRangeofAIAllOnes(BR, Mask))
    return false;
  BR.Shift = NewShift;

  // Specify the proper BR we want to handle.
  if (BR.Shift + BR.Width > BitSize)
    return false;
  if (BR.Width != 8 && BR.Width != 16 && BR.Width != 32 && BR.Width != 64)
    return false;
  if (BR.Shift % 8 != 0)
    return false;

  // Match other Binary operators like Add/Sub/And/Xor/Or or pattern like
  // And(Or(...And(Or(LargeVal, Cst), SmallVal))) and find the final load.
  SmallVector<Instruction *, 8> Insts;
  if (!matchInstsToShrink(Val, BR, Insts))
    return false;

  // Expect the final load has been found here.
  assert(!Insts.empty() && "Expect Insts to be not empty");
  LoadInst *LI = dyn_cast<LoadInst>(Insts.back());
  assert(LI && "Last elem in Insts should be a LoadInst");
  if (LI->isVolatile() || !LI->isUnordered())
    return false;

  IntegerType *NewTy = Type::getIntNTy(*Context, BR.Width);
  if (TLI->isNarrowingExpensive(EVT::getEVT(Ty), EVT::getEVT(NewTy)))
    return false;

  // Do legality check to ensure the range of shrunk load is not clobbered
  // from *LI to IN.
  IRBuilder<> Builder(*Context);
  Builder.SetInsertPoint(&IN);
  unsigned LoadByteOffset = computeByteOffset(BR, BitSize, *DL);
  Value *NewPtr =
      createNewPtr(LI->getPointerOperand(), LoadByteOffset, NewTy, Builder);
  // There are clobbers and all the clobbers are in the same block as IN.
  bool AllClobberInToBlock;
  if (hasClobberBetween(*LI, IN, MemoryLocation(NewPtr, BR.Width / 8),
                        AllClobberInToBlock)) {
    if (!AllClobberInToBlock) {
      RecursivelyDeleteTriviallyDeadInstructions(NewPtr);
      return false;
    }
  }
  if (!isBeneficial(ResShift, Insts)) {
    RecursivelyDeleteTriviallyDeadInstructions(NewPtr);
    return false;
  }

  // Do the shrinking transformation.
  Value *NewVal =
      shrinkInsts(NewPtr, BR, Insts, AllClobberInToBlock, ResShift, Builder);
  DEBUG(dbgs() << "MemShrink: Replace" << IN << " with" << *NewVal << "\n");
  IN.replaceAllUsesWith(NewVal);
  markCandForDeletion(IN);
  NumLoadShrunk++;
  return true;
}

/// Check insts in CandidatesToErase. If they are dead, remove the dead
/// instructions recursively and clean up Memory SSA.
bool MemAccessShrinkingPass::removeDeadInsts() {
  bool Changed = false;

  for (Instruction *I : CandidatesToErase) {
    RecursivelyDeleteTriviallyDeadInstructions(I, TLInfo, MSSAUpdater.get());
    Changed = true;
  }
  CandidatesToErase.clear();
  return Changed;
}

/// Try memory access shrinking on Function \Fn.
bool MemAccessShrinkingPass::tryShrink(Function &Fn) {
  bool MadeChange = false;
  for (BasicBlock *BB : post_order(&Fn)) {
    for (auto InstI = BB->rbegin(), InstE = BB->rend(); InstI != InstE;) {
      Instruction &Inst = *InstI++;
      if (StoreInst *SI = dyn_cast<StoreInst>(&Inst)) {
        if (tryShrinkStoreBySplit(*SI)) {
          MadeChange = true;
          continue;
        }
        MadeChange |= tryShrinkStoreToLegalTy(*SI);
        continue;
      }
      if (isa<TruncInst>(&Inst) || isa<BinaryOperator>(&Inst))
        MadeChange |= tryShrinkLoadUse(Inst);
    }
  }
  return MadeChange;
}

bool MemAccessShrinkingPass::runOnFunction(Function &Fn) {
  if (skipFunction(Fn))
    return false;

  DL = &Fn.getParent()->getDataLayout();
  DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
  TLInfo = &getAnalysis<TargetLibraryInfoWrapperPass>().getTLI();
  MSSA = &getAnalysis<MemorySSAWrapperPass>().getMSSA();
  MSSAWalker = MSSA->getWalker();
  MSSAUpdater = make_unique<MemorySSAUpdater>(MSSA);
  CandidatesToErase.clear();

  Context = &Fn.getContext();

  if (TM)
    TLI = TM->getSubtargetImpl(Fn)->getTargetLowering();

  // Iterates the Fn until nothing is changed.
  MSSA->verifyMemorySSA();
  bool MadeChange = false;
  while (true) {
    if (!tryShrink(Fn) && !removeDeadInsts())
      break;
    MadeChange = true;
  }

  return MadeChange;
}
