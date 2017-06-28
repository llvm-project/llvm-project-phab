//===--------- X86InterleavedAccess.cpp ----------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===--------------------------------------------------------------------===//
///
/// \file
/// This file contains the X86 implementation of the interleaved accesses
/// optimization generating X86-specific instructions/intrinsics for
/// interleaved access groups.
///
//===--------------------------------------------------------------------===//

#include "X86ISelLowering.h"
#include "X86TargetMachine.h"
#include "llvm/Analysis/VectorUtils.h"

using namespace llvm;

namespace {
/// \brief This class holds necessary information to represent an interleaved
/// access group and supports utilities to lower the group into
/// X86-specific instructions/intrinsics.
///  E.g. A group of interleaving access loads (Factor = 2; accessing every
///       other element)
///        %wide.vec = load <8 x i32>, <8 x i32>* %ptr
///        %v0 = shuffle <8 x i32> %wide.vec, <8 x i32> undef, <0, 2, 4, 6>
///        %v1 = shuffle <8 x i32> %wide.vec, <8 x i32> undef, <1, 3, 5, 7>
class X86InterleavedAccessGroup {
  /// \brief Reference to the wide-load instruction of an interleaved access
  /// group.
  Instruction *const Inst;

  /// \brief Reference to the shuffle(s), consumer(s) of the (load) 'Inst'.
  ArrayRef<ShuffleVectorInst *> Shuffles;

  /// \brief Reference to the starting index of each user-shuffle.
  ArrayRef<unsigned> Indices;

  /// \brief Reference to the interleaving stride in terms of elements.
  const unsigned Factor;

  /// \brief Reference to the underlying target.
  const X86Subtarget &Subtarget;

  const DataLayout &DL;

  IRBuilder<> &Builder;

  /// \brief Breaks down a vector \p 'Inst' of N elements into \p NumSubVectors
  /// sub vectors of type \p T. Returns the sub-vectors in \p DecomposedVectors.
  void decompose(Instruction *Inst, unsigned NumSubVectors, VectorType *T,
                 SmallVectorImpl<Instruction *> &DecomposedVectors);

  /// \brief Performs matrix transposition on a 4x4 matrix \p InputVectors and
  /// returns the transposed-vectors in \p TransposedVectors.
  /// E.g.
  /// InputVectors:
  ///   In-V0 = p1, p2, p3, p4
  ///   In-V1 = q1, q2, q3, q4
  ///   In-V2 = r1, r2, r3, r4
  ///   In-V3 = s1, s2, s3, s4
  /// OutputVectors:
  ///   Out-V0 = p1, q1, r1, s1
  ///   Out-V1 = p2, q2, r2, s2
  ///   Out-V2 = p3, q3, r3, s3
  ///   Out-V3 = P4, q4, r4, s4
  void transpose_4x4(ArrayRef<Instruction *> InputVectors,
                     SmallVectorImpl<Value *> &TrasposedVectors);
  void transposeChar_32x4(ArrayRef<Instruction *> InputVectors,
                          SmallVectorImpl<Value *> &TrasposedVectors);
public:
  /// In order to form an interleaved access group X86InterleavedAccessGroup
  /// requires a wide-load instruction \p 'I', a group of interleaved-vectors
  /// \p Shuffs, reference to the first indices of each interleaved-vector
  /// \p 'Ind' and the interleaving stride factor \p F. In order to generate
  /// X86-specific instructions/intrinsics it also requires the underlying
  /// target information \p STarget.
  explicit X86InterleavedAccessGroup(Instruction *I,
                                     ArrayRef<ShuffleVectorInst *> Shuffs,
                                     ArrayRef<unsigned> Ind, const unsigned F,
                                     const X86Subtarget &STarget,
                                     IRBuilder<> &B)
      : Inst(I), Shuffles(Shuffs), Indices(Ind), Factor(F), Subtarget(STarget),
        DL(Inst->getModule()->getDataLayout()), Builder(B) {}

  /// \brief Returns true if this interleaved access group can be lowered into
  /// x86-specific instructions/intrinsics, false otherwise.
  bool isSupported() const;

  /// \brief Lowers this interleaved access group into X86-specific
  /// instructions/intrinsics.
  bool lowerIntoOptimizedSequence();
};
} // end anonymous namespace

bool X86InterleavedAccessGroup::isSupported() const {
  VectorType *ShuffleVecTy = Shuffles[0]->getType();
  uint64_t ShuffleVecSize = DL.getTypeSizeInBits(ShuffleVecTy);
  Type *ShuffleEltTy = ShuffleVecTy->getVectorElementType();

  // Currently, lowering is supported for the following vectors:
  // 1. 4-element vectors of 64 bits on AVX.
  // 2. 32-element vectors of 8 bits on AVX.
  uint64_t ExpectedShuffleVecSize;
  if (isa<LoadInst>(Inst))
    ExpectedShuffleVecSize = 256;
  else
    ExpectedShuffleVecSize = 1024;

  if (((DL.getTypeSizeInBits(ShuffleEltTy) != 64) && 
       (DL.getTypeSizeInBits(ShuffleEltTy) != 8)) || !Subtarget.hasAVX() ||
      ShuffleVecSize != ExpectedShuffleVecSize || Factor != 4)
    return false;

  return true;
}

void X86InterleavedAccessGroup::decompose(
    Instruction *VecInst, unsigned NumSubVectors, VectorType *SubVecTy,
    SmallVectorImpl<Instruction *> &DecomposedVectors) {

  assert((isa<LoadInst>(VecInst) || isa<ShuffleVectorInst>(VecInst)) &&
         "Expected Load or Shuffle");

  Type *VecTy = VecInst->getType();
  (void)VecTy;
  assert(VecTy->isVectorTy() &&
         DL.getTypeSizeInBits(VecTy) >=
             DL.getTypeSizeInBits(SubVecTy) * NumSubVectors &&
         "Invalid Inst-size!!!");

  if (auto *SVI = dyn_cast<ShuffleVectorInst>(VecInst)) {
    Value *Op0 = SVI->getOperand(0);
    Value *Op1 = SVI->getOperand(1);

    // Generate N(= NumSubVectors) shuffles of T(= SubVecTy) type.
    for (unsigned i = 0; i < NumSubVectors; ++i)
      DecomposedVectors.push_back(
          cast<ShuffleVectorInst>(Builder.CreateShuffleVector(
              Op0, Op1, createSequentialMask(Builder, Indices[i],
                                             SubVecTy->getVectorNumElements(), 0))));
    return;
  }

  // Decompose the load instruction.
  LoadInst *LI = cast<LoadInst>(VecInst);
  Type *VecBasePtrTy = SubVecTy->getPointerTo(LI->getPointerAddressSpace());
  Value *VecBasePtr =
      Builder.CreateBitCast(LI->getPointerOperand(), VecBasePtrTy);

  // Generate N loads of T type.
  for (unsigned i = 0; i < NumSubVectors; i++) {
    // TODO: Support inbounds GEP.
    Value *NewBasePtr = Builder.CreateGEP(VecBasePtr, Builder.getInt32(i));
    Instruction *NewLoad =
        Builder.CreateAlignedLoad(NewBasePtr, LI->getAlignment());
    DecomposedVectors.push_back(NewLoad);
  }
}

/// Generate unpacklo/unpackhi shuffle mask.
static void createUnpackShuffleMask(int NumElts, SmallVectorImpl<uint32_t> &Mask,
                                    bool Lo, bool Unary) {
  int NumEltsInLane = NumElts / 2;
  assert(Mask.empty() && "Expected an empty shuffle mask vector");
  for (int i = 0; i < NumElts; ++i) {
    unsigned LaneStart = (i / NumEltsInLane) * NumEltsInLane;
    int Pos = (i % NumEltsInLane) / 2 + LaneStart;
    Pos += (Unary ? 0 : NumElts * (i % 2));
    Pos += (Lo ? 0 : NumEltsInLane / 2);
    Mask.push_back(Pos);
  }
}

//  Create shuffle mask for concatenation of two half vectors.
//  Low = false:  mask generated for the shuffle
//  shufle(VEC1,VEC2,{NumElement/2, NumElement/2+1, NumElement/2+2...,
//                    NumElement-1, NumElement-NumElement/2,
//                    NumElement-NumElement/2+1..., 2*NumElement-1})
//  = concat(high_half(VEC1),high_half(VEC2))
//  Low = true:  mask generated for the shuffle
//  shufle(VEC1,VEC2,{0,1,2,...,NumElement/2-1,NumElement,
//                    NumElement+1...,NumElement+NumElement/2-1})
//  = concat(low_half(VEC1),low_half(VEC2))
static void createConcatShuffleMask(int NumElement,
                                    SmallVectorImpl<uint32_t> &Mask, bool Low) {
  int BeginIndex = Low ? 0 : NumElement / 2;
  int EndIndex = BeginIndex + NumElement / 2;
  for (int i = 0; i < NumElement; ++i) {
    if (BeginIndex == EndIndex)
      BeginIndex += NumElement / 2;
    Mask.push_back(BeginIndex);
    BeginIndex++;
  }
}

void X86InterleavedAccessGroup::transposeChar_32x4(
    ArrayRef<Instruction *> Matrix,
    SmallVectorImpl<Value *> &TransposedMatrix) {

  // Example: Assuming we start from the following vectors:
  // Matrix[0]= c0 c1 c2 c3 c4 ... c31
  // Matrix[1]= m0 m1 m2 m3 m4 ... m31
  // Matrix[2]= y0 y1 y2 y3 y4 ... y31
  // Matrix[3]= k0 k1 k2 k3 k4 ... k31

  TransposedMatrix.resize(4);

  SmallVector<uint32_t, 32> MaskHighTemp;
  SmallVector<uint32_t, 32> MaskLowTemp;
  SmallVector<uint32_t, 32> MaskHighTemp1;
  SmallVector<uint32_t, 32> MaskLowTemp1;
  SmallVector<uint32_t, 32> ConcatLow;
  SmallVector<uint32_t, 32> ConcatHigh;

  // MaskHighTemp and MaskLowTemp built in the vpunpckhbw and vpunpcklbw X86
  // shuffle pattern.
  createUnpackShuffleMask(32, MaskHighTemp, false, false);
  createUnpackShuffleMask(32, MaskLowTemp, true, false);

  // MaskHighTemp1 and MaskLowTemp1 built in the vpunpckhdw and vpunpckldw X86
  // shuffle pattern.
  createUnpackShuffleMask(16, MaskLowTemp1, true, false);
  createUnpackShuffleMask(16, MaskHighTemp1, false, false);

  // ConcatHigh and ConcatLow built in the vperm2i128 and vinserti128 X86
  // shuffle pattern.
  createConcatShuffleMask(16, ConcatLow, true);
  createConcatShuffleMask(16, ConcatHigh, false);

  ArrayRef<uint32_t> MaskHigh = makeArrayRef(MaskHighTemp);
  ArrayRef<uint32_t> MaskLow = makeArrayRef(MaskLowTemp);
  ArrayRef<uint32_t> MaskConcatLow = makeArrayRef(ConcatLow);
  ArrayRef<uint32_t> MaskConcatHigh = makeArrayRef(ConcatHigh);
  ArrayRef<uint32_t> MaskHighWord = makeArrayRef(MaskHighTemp1);
  ArrayRef<uint32_t> MaskLowWord = makeArrayRef(MaskLowTemp1);

  // IntrVec1Low  = c0  m0  c1  m1 ... c7  m7  | c16 m16 c17 m17 ... c23 m23
  // IntrVec1High = c8  m8  c9  m9 ... c15 m15 | c24 m24 c25 m25 ... c31 m31
  // IntrVec2Low  = y0  k0  y1  k1 ... y7  k7  | y16 k16 y17 k17 ... y23 k23
  // IntrVec2High = y8  k8  y9  k9 ... y15 k15 | y24 k24 y25 k25 ... y31 k31

  Value *IntrVec1Low =
      Builder.CreateShuffleVector(Matrix[0], Matrix[1], MaskLow);
  Value *IntrVec1High =
      Builder.CreateShuffleVector(Matrix[0], Matrix[1], MaskHigh);
  Value *IntrVec2Low =
      Builder.CreateShuffleVector(Matrix[2], Matrix[3], MaskLow);
  Value *IntrVec2High =
      Builder.CreateShuffleVector(Matrix[2], Matrix[3], MaskHigh);

  IntrVec1Low = Builder.CreateBitCast(
      IntrVec1Low,
      VectorType::get(Type::getInt16Ty(Shuffles[0]->getContext()), 16));
  IntrVec1High = Builder.CreateBitCast(
      IntrVec1High,
      VectorType::get(Type::getInt16Ty(Shuffles[0]->getContext()), 16));
  IntrVec2Low = Builder.CreateBitCast(
      IntrVec2Low,
      VectorType::get(Type::getInt16Ty(Shuffles[0]->getContext()), 16));
  IntrVec2High = Builder.CreateBitCast(
      IntrVec2High,
      VectorType::get(Type::getInt16Ty(Shuffles[0]->getContext()), 16));

  // cmyk4  cmyk5  cmyk6   cmyk7  | cmyk20 cmyk21 cmyk22 cmyk23
  // cmyk12 cmyk13 cmyk14  cmyk15 | cmyk28 cmyk29 cmyk30 cmyk31
  // cmyk0  cmyk1  cmyk2   cmyk3  | cmyk16 cmyk17 cmyk18 cmyk19
  // cmyk8  cmyk9  cmyk10  cmyk11 | cmyk24 cmyk25 cmyk26 cmyk27

  Value *High = Builder.CreateShuffleVector(IntrVec1Low, IntrVec2Low,
                                            MaskHighWord);
  Value *High1 = Builder.CreateShuffleVector(IntrVec1High, IntrVec2High,
                                             MaskHighWord);
  Value *Low = Builder.CreateShuffleVector(IntrVec1Low, IntrVec2Low,
                                           MaskLowWord);
  Value *Low1 = Builder.CreateShuffleVector(IntrVec1High, IntrVec2High,
                                            MaskLowWord);

  // cmyk0  cmyk1  cmyk2   cmyk3  | cmyk4  cmyk5  cmyk6   cmyk7
  // cmyk8  cmyk9  cmyk10  cmyk11 | cmyk12 cmyk13 cmyk14  cmyk15
  // cmyk16 cmyk17 cmyk18 cmyk19  | cmyk20 cmyk21 cmyk22 cmyk23
  // cmyk24 cmyk25 cmyk26 cmyk27  | cmyk28 cmyk29 cmyk30 cmyk31

  TransposedMatrix[0] =
      Builder.CreateShuffleVector(Low, High, MaskConcatLow);
  TransposedMatrix[1] =
      Builder.CreateShuffleVector(Low1, High1, MaskConcatLow);
  TransposedMatrix[2] =
      Builder.CreateShuffleVector(Low, High, MaskConcatHigh);
  TransposedMatrix[3] =
      Builder.CreateShuffleVector(Low1, High1, MaskConcatHigh);
}

void X86InterleavedAccessGroup::transpose_4x4(
    ArrayRef<Instruction *> Matrix,
    SmallVectorImpl<Value *> &TransposedMatrix) {
  assert(Matrix.size() == 4 && "Invalid matrix size");
  TransposedMatrix.resize(4);

  // dst = src1[0,1],src2[0,1]
  uint32_t IntMask1[] = {0, 1, 4, 5};
  ArrayRef<uint32_t> Mask = makeArrayRef(IntMask1, 4);
  Value *IntrVec1 = Builder.CreateShuffleVector(Matrix[0], Matrix[2], Mask);
  Value *IntrVec2 = Builder.CreateShuffleVector(Matrix[1], Matrix[3], Mask);

  // dst = src1[2,3],src2[2,3]
  uint32_t IntMask2[] = {2, 3, 6, 7};
  Mask = makeArrayRef(IntMask2, 4);
  Value *IntrVec3 = Builder.CreateShuffleVector(Matrix[0], Matrix[2], Mask);
  Value *IntrVec4 = Builder.CreateShuffleVector(Matrix[1], Matrix[3], Mask);

  // dst = src1[0],src2[0],src1[2],src2[2]
  uint32_t IntMask3[] = {0, 4, 2, 6};
  Mask = makeArrayRef(IntMask3, 4);
  TransposedMatrix[0] = Builder.CreateShuffleVector(IntrVec1, IntrVec2, Mask);
  TransposedMatrix[2] = Builder.CreateShuffleVector(IntrVec3, IntrVec4, Mask);

  // dst = src1[1],src2[1],src1[3],src2[3]
  uint32_t IntMask4[] = {1, 5, 3, 7};
  Mask = makeArrayRef(IntMask4, 4);
  TransposedMatrix[1] = Builder.CreateShuffleVector(IntrVec1, IntrVec2, Mask);
  TransposedMatrix[3] = Builder.CreateShuffleVector(IntrVec3, IntrVec4, Mask);
}

// Lowers this interleaved access group into X86-specific
// instructions/intrinsics.
bool X86InterleavedAccessGroup::lowerIntoOptimizedSequence() {
  SmallVector<Instruction *, 4> DecomposedVectors;
  SmallVector<Value *, 4> TransposedVectors;
  VectorType *ShuffleTy = Shuffles[0]->getType();

  if (isa<LoadInst>(Inst)) {
    // Try to generate target-sized register(/instruction).
    decompose(Inst, Factor, ShuffleTy, DecomposedVectors);

    // Perform matrix-transposition in order to compute interleaved
    // results by generating some sort of (optimized) target-specific
    // instructions.
    transpose_4x4(DecomposedVectors, TransposedVectors);

    // Now replace the unoptimized-interleaved-vectors with the
    // transposed-interleaved vectors.
    for (unsigned i = 0, e = Shuffles.size(); i < e; ++i)
      Shuffles[i]->replaceAllUsesWith(TransposedVectors[Indices[i]]);

    return true;
  }

  Type *ShuffleEltTy = ShuffleTy->getVectorElementType();
  unsigned NumSubVecElems = ShuffleTy->getVectorNumElements() / Factor;

  // Lower the interleaved stores:
  //   1. Decompose the interleaved wide shuffle into individual shuffle
  //   vectors.
  decompose(Shuffles[0], Factor,
            VectorType::get(ShuffleEltTy, NumSubVecElems), DecomposedVectors);

  //   2. Transpose the interleaved-vectors into vectors of contiguous
  //      elements.
  StoreInst *SI = cast<StoreInst>(Inst);
  switch (NumSubVecElems) {
  case 4: {
    transpose_4x4(DecomposedVectors, TransposedVectors);

    //   3. Concatenate the contiguous-vectors back into a wide vector.
    Value *WideVec = concatenateVectors(Builder, TransposedVectors);

    //   4. Generate a store instruction for wide-vec.
    Builder.CreateAlignedStore(WideVec, SI->getPointerOperand(),
                               SI->getAlignment());
    break;
  }
  case 32: {
    transposeChar_32x4(DecomposedVectors, TransposedVectors);
    // VecInst contains the Ptr argument.
    Value *VecInst = Inst->getOperand(1);
    Type *IntOf16 = Type::getInt16Ty(Shuffles[0]->getContext());
    // From <128xi8>* to <16xi16>*
    Type *VecTran = VectorType::get(IntOf16, 16)->getPointerTo();
    Value *VecBasePtr = Builder.CreateBitCast(VecInst, VecTran);
    for (unsigned i = 0; i < 4; i++) {
      Value *NewBasePtr = Builder.CreateGEP(VecBasePtr, Builder.getInt32(i));
      Builder.CreateAlignedStore(TransposedVectors[i], NewBasePtr,
                                 SI->getAlignment());
    }
    break;
  }
  default:
    return false;
  }

  return true;
}

// Lower interleaved load(s) into target specific instructions/
// intrinsics. Lowering sequence varies depending on the vector-types, factor,
// number of shuffles and ISA.
// Currently, lowering is supported for 4x64 bits with Factor = 4 on AVX.
bool X86TargetLowering::lowerInterleavedLoad(
    LoadInst *LI, ArrayRef<ShuffleVectorInst *> Shuffles,
    ArrayRef<unsigned> Indices, unsigned Factor) const {
  assert(Factor >= 2 && Factor <= getMaxSupportedInterleaveFactor() &&
         "Invalid interleave factor");
  assert(!Shuffles.empty() && "Empty shufflevector input");
  assert(Shuffles.size() == Indices.size() &&
         "Unmatched number of shufflevectors and indices");

  // Create an interleaved access group.
  IRBuilder<> Builder(LI);
  X86InterleavedAccessGroup Grp(LI, Shuffles, Indices, Factor, Subtarget,
                                Builder);

  return Grp.isSupported() && Grp.lowerIntoOptimizedSequence();
}

// Currently lowering is supported for the following interleaves:
// 1. stride4 x 64bit elements with vector factor 4 on AVX
// 2. stride4 x 8bit elements with vector factor 32 on AVX2
bool X86TargetLowering::lowerInterleavedStore(StoreInst *SI,
                                              ShuffleVectorInst *SVI,
                                              unsigned Factor) const {
  assert(Factor >= 2 && Factor <= getMaxSupportedInterleaveFactor() &&
         "Invalid interleave factor");

  assert(SVI->getType()->getVectorNumElements() % Factor == 0 &&
         "Invalid interleaved store");

  // Holds the indices of SVI that correspond to the starting index of each
  // interleaved shuffle.
  SmallVector<unsigned, 4> Indices;
  auto Mask = SVI->getShuffleMask();
  for (unsigned i = 0; i < Factor; i++)
    Indices.push_back(Mask[i]);

  ArrayRef<ShuffleVectorInst *> Shuffles = makeArrayRef(SVI);

  // Create an interleaved access group.
  IRBuilder<> Builder(SI);
  X86InterleavedAccessGroup Grp(SI, Shuffles, Indices, Factor, Subtarget,
                                Builder);
  return Grp.isSupported() && Grp.lowerIntoOptimizedSequence();
}