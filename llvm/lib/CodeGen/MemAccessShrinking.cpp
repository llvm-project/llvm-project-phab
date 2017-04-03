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
/// Consecutive bitfields are now wrapped as a group and represented as a
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
/// List some typical patterns and the way to transform them, in order to
/// give an idea how the optimization works. More general patterns to recognize
/// can be found in function head comments.
///
///    P1: store(or(and(load P, Cst), MaskedVal), P)
/// If the bit operations show the range (width+shift, shift] of MaskedVal
/// is extracted and filled into the target memory. It will be transformed
/// to:
///    narrow_store(trunc(lshr(MaskedVal, shift) to width), P+shift)
/// Note: It is often the case that the pattern "trunc(lshr(MaskedVal, ...)"
/// can be further simplified during DAG Combiner, which will increase the
/// benefit.
///
///    P2: and(lshr(load P, Cst1), Cst2)
/// If the bit operations show the range (width+shift, shift] of load P is
/// the actual valid bits to be used. It will be transformed to:
///    narrow_load(P+shift) with its type shrinked to 'width' bits integer.
///
/// Some notes about algorithm:
/// * The algorithm scans the program and tries to recognize and transform the
///   patterns above. It runs iteratively until no more change has happened.
/// * To prevent the optimization from blocking load/store coalescing, it is
///   invoked late in the pipeline, just before CodeGenPrepare. In this late
///   stage, both the pattern matching and related safety check become more
///   difficult because of previous optimizations introducing mergd load/store
///   and more complex control flow. That is why MemorySSA is used here. It is
///   scalable and precise for the safety check especially when we tries to
///   insert a shrinked load in the block which is many blocks away from its
///   original load.
//===----------------------------------------------------------------------===//

#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/ADT/Statistic.h"
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
#include "llvm/Transforms/Utils/MemorySSA.h"
#include "llvm/Transforms/Utils/MemorySSAUpdater.h"

#define DEBUG_TYPE "memaccessshrink"

using namespace llvm;
using namespace llvm::PatternMatch;

STATISTIC(NumStoreShrinked, "Number of stores shrinked");
STATISTIC(NumLoadShrinked, "Number of Loads shrinked");

static cl::opt<bool> EnableLoadShrink("enable-load-shrink", cl::init(true));
static cl::opt<bool> EnableStoreShrink("enable-store-shrink", cl::init(true));

namespace {
/// Describe the value range used for mem access.
struct ModRange {
  unsigned Shift;
  unsigned Width;
  bool ShrinkWithMaskedVal;
};

/// \brief The memory access shrinking pass.
class MemAccessShrinkingPass : public FunctionPass {
public:
  static char ID; // Pass identification, replacement for typeid
  MemAccessShrinkingPass(const TargetMachine *TM = nullptr) : FunctionPass(ID) {
    this->TM = TM;
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

  // MultiUsesSeen shows a multiuse node is found on the du-chain.
  bool MultiUsesSeen;
  // SavedInsts shows how many instructions saved for a specific shrink so far.
  // It is used to evaluate whether some instructions will be saved for the
  // shrinking especially when MultiUsesSeen is true.
  unsigned SavedInsts;

  void analyzeOrAndPattern(Value &MaskedVal, ConstantInt &Cst, ModRange &MR,
                           unsigned TBits);
  void analyzeBOpPattern(Value &Val, ConstantInt &Cst, ModRange &MR,
                         unsigned TBits);
  bool updateModRange(ModRange &MR, unsigned TBits, unsigned Align);
  bool hasClobberBetween(Instruction &From, Instruction &To);
  bool needNewStore(Value &OrigVal, StoreInst &SI);
  Value *createNewPtr(Value *Ptr, unsigned StOffset, Type *NewTy,
                      IRBuilder<> &Builder);

  bool reduceLoadOpsStoreWidth(StoreInst &SI);
  bool tryShrinkOnInst(Instruction &Inst);
  bool reduceLoadOpsWidth(Instruction &IN);
  Value *shrinkInsts(Value *NewPtr, ModRange &MR,
                     SmallVectorImpl<Instruction *> &Insts,
                     Instruction *NewInsertPt, unsigned ResShift,
                     IRBuilder<> &Builder);
  bool matchInstsToShrink(Value *Val, ModRange &MR,
                          SmallVectorImpl<Instruction *> &Insts);
  bool hasClobberBetween(Instruction &From, Instruction &To,
                         const MemoryLocation &Mem, Instruction *&NewInsertPt);
  void addSavedInst(Instruction &Inst, bool ReplaceAllUses = false);
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

/// Analyze or ((and (load P), \p Cst), \p MaskedVal). Update \p MR.Width
/// with the number of bits of the original load to be modified, and update
/// \p MR.Shift with the pos of the first bit to be modified. If the
/// analysis result shows we can use bits extracted from MaskedVal as store
/// value, set \p MR.ShrinkWithMaskedVal to be true.
void MemAccessShrinkingPass::analyzeOrAndPattern(Value &MaskedVal,
                                                 ConstantInt &Cst, ModRange &MR,
                                                 unsigned TBits) {
  // Cst is the mask. Analyze the pattern of mask after sext it to uint64_t. We
  // will handle patterns like either 0..01..1 or 1..10..01..1
  APInt Mask = Cst.getValue().sextOrTrunc(TBits);
  assert(Mask.getBitWidth() == TBits && "Unexpected bitwidth of Mask");
  unsigned MaskLeadOnes = Mask.countLeadingOnes();
  if (MaskLeadOnes == TBits)
    return;
  unsigned MaskTrailOnes = Mask.countTrailingOnes();
  unsigned MaskMidZeros = !MaskLeadOnes
                              ? Mask.countLeadingZeros()
                              : Mask.ashr(MaskTrailOnes).countTrailingZeros();

  MR.ShrinkWithMaskedVal = true;
  // See if we have a continuous run of zeros.
  if (MaskLeadOnes + MaskMidZeros + MaskTrailOnes != TBits) {
    MaskMidZeros = TBits - MaskLeadOnes - MaskTrailOnes;
    MR.ShrinkWithMaskedVal = false;
  }

  // Check MaskedVal only provides nonzero bits within range from lowbits
  // (MaskTrailOnes) to highbits (MaskTrailOnes + MaskMidZeros).
  APInt BitMask =
      ~APInt::getBitsSet(TBits, MaskTrailOnes, MaskTrailOnes + MaskMidZeros);

  // Find out the range in which 1 appears in MaskedVal.
  APInt KnownOne(TBits, 0), KnownZero(TBits, 0);
  computeKnownBits(&MaskedVal, KnownZero, KnownOne, *DL, 0);

  MR.Shift = MaskTrailOnes;
  MR.Width = MaskMidZeros;
  // Expect the bits being 1 in BitMask are all KnownZero bits in MaskedVal,
  // otherwise we need to set ShrinkWithMaskedVal to false and expand MR.
  if ((KnownZero & BitMask) != BitMask) {
    MR.ShrinkWithMaskedVal = false;
    // Lower is the first bit for which we are not sure about the fact of
    // its being zero.
    unsigned Lower = KnownZero.countTrailingOnes();
    // Higher is the last bit for which we are not sure about the fact of
    // its being zero.
    unsigned Higher = TBits - KnownZero.countLeadingOnes();
    MR.Shift = std::min(Lower, MaskTrailOnes);
    MR.Width = std::max(Higher, MaskTrailOnes + MaskMidZeros) - MR.Shift;
  }
}

/// Analyze \p Val = or/xor/and ((load P), \p Cst). Update \p MR.Width
/// with the number of bits of the original load to be modified, and update
/// \p MR.Shift with the pos of the first bit to be modified.
void MemAccessShrinkingPass::analyzeBOpPattern(Value &Val, ConstantInt &Cst,
                                               ModRange &MR, unsigned TBits) {
  APInt Mask = Cst.getValue().sextOrTrunc(TBits);
  BinaryOperator *BOP = cast<BinaryOperator>(&Val);
  if (BOP->getOpcode() == Instruction::And)
    Mask = ~Mask;

  MR.Shift = Mask.countTrailingZeros();
  MR.Width = Mask.getBitWidth() - MR.Shift;
  if (MR.Width)
    MR.Width = MR.Width - Mask.countLeadingZeros();
  MR.ShrinkWithMaskedVal = false;
}

/// Update \p MR.Width and \p MR.Shift so the updated \p MR.Width
/// bits can form a legal type and also cover all the modified bits.
bool MemAccessShrinkingPass::updateModRange(ModRange &MR, unsigned TBits,
                                            unsigned Align) {
  ModRange NewMR;
  NewMR.Width = PowerOf2Ceil(MR.Width);
  Type *NewTy = Type::getIntNTy(*Context, NewMR.Width);

  // Check if we can find a new Shift for the Width of NewMR, so that
  // NewMR forms a new range covering the old modified range without
  // worsening alignment.
  auto coverOldRange = [&](ModRange &NewMR, ModRange &MR) -> bool {
    unsigned MAlign = MinAlign(Align, DL->getABITypeAlignment(NewTy));
    int Shift = MR.Shift - MR.Shift % (MAlign * 8);
    while (Shift >= 0) {
      if (NewMR.Width + Shift <= TBits &&
          NewMR.Width + Shift >= MR.Width + MR.Shift) {
        NewMR.Shift = Shift;
        return true;
      }
      Shift -= MAlign * 8;
    }
    return false;
  };
  // See whether we can store NewTy legally.
  auto isStoreLegalType = [&](Type *NewTy) -> bool {
    EVT OldEVT =
        TLI->getValueType(*DL, Type::getIntNTy(*Context, PowerOf2Ceil(TBits)));
    EVT NewEVT = TLI->getValueType(*DL, NewTy);
    return TLI->isOperationLegalOrCustom(ISD::STORE, NewEVT) ||
           TLI->isTruncStoreLegalOrCustom(OldEVT, NewEVT);
  };
  // Try to find the minimal NewMR.Width which can form a legal type and cover
  // all the old modified bits.
  while (NewMR.Width < TBits &&
         (!isStoreLegalType(NewTy) || !coverOldRange(NewMR, MR))) {
    NewMR.Width = NextPowerOf2(NewMR.Width);
    NewTy = Type::getIntNTy(*Context, NewMR.Width);
  }
  MR.Width = NewMR.Width;
  MR.Shift = NewMR.Shift;

  if (MR.Width >= TBits)
    return false;
  return true;
}

/// Compute the offset used to compute the new ptr address. It will be
/// mainly based on MR and the endian of the target.
static unsigned computeStOffset(ModRange &MR, unsigned TBits,
                                const DataLayout &DL) {
  unsigned StOffset;
  if (DL.isBigEndian())
    StOffset = TBits - MR.Shift - MR.Width;
  else
    StOffset = MR.Shift;

  if (StOffset % 8 != 0)
    MR.ShrinkWithMaskedVal = false;

  return StOffset / 8;
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

/// It is possible that we already have a store of OrigVal previously and
/// it is not clobbered, then we can use it and don't have to generate a
/// a new store.
bool MemAccessShrinkingPass::needNewStore(Value &OrigVal, StoreInst &SI) {
  for (User *U : OrigVal.users()) {
    StoreInst *PrevSI = dyn_cast<StoreInst>(U);
    if (!PrevSI ||
        !hasSamePtr(PrevSI->getPointerOperand(), SI.getPointerOperand()))
      continue;
    if (!hasClobberBetween(*PrevSI, SI))
      return false;
  }
  return true;
}

/// Create new address to be used by shrinked load/store based on original
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

/// Try to shrink the store when the input \p SI has one of the patterns:
/// Pattern1: or(and(Val, Cst), MaskedVal).
/// Pattern2: or/and/xor(load, Cst).
/// For the first pattern, when the Cst and MaskedVal satisfies some
/// requirements, the or+and pair has the property that only a portion of
/// Val is modified and the rest of it are not changed. We want to shrink
/// the store to be aligned to the modified range of the Val.
/// Pattern1 after the shrinking looks like:
///
///   store(Val)        // The store can be omitted if the Val is a load
///                     // with the same address as the original store.
///   narrow_store(shrinked MaskedVal)
///
/// However, if some condition doesn't satisfy, which will be indicated by
/// MR::ShrinkWithMaskedVal being false, We may try another type of shrinking
/// -- shrink the store to legal type if it is of illegal type. So for
/// Pattern2 and Pattern1 when Val is load and MR::ShrinkWithMaskedVal is
/// false, if the store is of illegal type, we may shrink them to:
///
///   store(or(and(shrinked load, shrinked Cst), shrinked MaskedVal)), or
///   store(or/and/xor(shrinked load, shrinked Cst))
///
/// After the shrinking, all the operations will be of legal type.
bool MemAccessShrinkingPass::reduceLoadOpsStoreWidth(StoreInst &SI) {
  Value *Val = SI.getOperand(0);
  Value *Ptr = SI.getOperand(1);
  Type *StoreTy = Val->getType();
  if (StoreTy->isVectorTy() || !StoreTy->isIntegerTy())
    return false;

  if (SI.isVolatile() || !SI.isUnordered())
    return false;

  unsigned TBits = DL->getTypeSizeInBits(StoreTy);
  if (TBits != DL->getTypeStoreSizeInBits(StoreTy))
    return false;

  LoadInst *LI = nullptr;
  Value *OrigVal = nullptr;
  Value *MaskedVal;
  ConstantInt *Cst;
  // Match or(and(Val, Cst), MaskedVal) pattern or their correspondents
  // after commutation.
  // However the pattern matching below is more complex than that because
  // of the commutation and some matching preference we expect.
  // for case: or(and(or(and(LI, Cst_1), MaskedVal_1), Cst_2), MaskedVal_2)
  // and if MaskedVal_2 happens to be another and operator, we hope we can
  // match or(and(LI, Cst_1), MaskedVal_1) to Val instead of MaskedVal.
  bool OrAndPattern =
      match(Val, m_c_Or(m_And(m_Load(LI), m_ConstantInt(Cst)),
                        m_Value(MaskedVal))) ||
      (match(Val, m_Or(m_And(m_Value(OrigVal), m_ConstantInt(Cst)),
                       m_Value(MaskedVal))) &&
       match(OrigVal, m_c_Or(m_And(m_Value(), m_Value()), m_Value()))) ||
      (match(Val, m_Or(m_Value(MaskedVal),
                       m_And(m_Value(OrigVal), m_ConstantInt(Cst)))) &&
       match(OrigVal, m_c_Or(m_And(m_Value(), m_Value()), m_Value())));
  // Match "or/and/xor(load, cst)" pattern or their correspondents after
  // commutation.
  bool BopPattern = match(Val, m_c_Or(m_Load(LI), m_ConstantInt(Cst))) ||
                    match(Val, m_c_And(m_Load(LI), m_ConstantInt(Cst))) ||
                    match(Val, m_c_Xor(m_Load(LI), m_ConstantInt(Cst)));
  if (!OrAndPattern && !BopPattern)
    return false;

  // LI should have the same address as SI.
  if (LI && !hasSamePtr(LI->getPointerOperand(), Ptr))
    return false;

  if (LI && (LI->isVolatile() || !LI->isUnordered()))
    return false;

  // Make sure the memory location of LI/SI is not clobbered between them.
  if (LI && hasClobberBetween(*LI, SI))
    return false;

  // Analyze MR which indicates the range of the input that will actually
  // be modified and stored.
  ModRange MR = {0, 0, true};
  if (OrAndPattern)
    analyzeOrAndPattern(*MaskedVal, *Cst, MR, TBits);
  if (BopPattern)
    analyzeBOpPattern(*Val, *Cst, MR, TBits);

  assert(MR.Shift + MR.Width <= TBits && "Unexpected ModRange!");
  if (!MR.Width)
    return false;

  unsigned StOffset;
  if (MR.ShrinkWithMaskedVal) {
    // Get the offset from Ptr for the shrinked store.
    StOffset = computeStOffset(MR, TBits, *DL);

    // If MR.Width is not the length of legal type, we cannot
    // store MaskedVal directly.
    if (MR.Width != 8 && MR.Width != 16 && MR.Width != 32)
      MR.ShrinkWithMaskedVal = false;
  }

  unsigned Align = SI.getAlignment();
  // If we are shrink illegal type of store with original val, update MR.Width
  // and MR.Shift to ensure the shrinked store is of legal type.
  if (!MR.ShrinkWithMaskedVal) {
    // Check whether the store is of illegal type. If it is not, don't bother.
    if (!TLI || TLI->isOperationLegalOrCustom(ISD::STORE,
                                              TLI->getValueType(*DL, StoreTy)))
      return false;
    // Try to find out a MR of legal type.
    if (!updateModRange(MR, TBits, Align))
      return false;
    // It is meaningless to shrink illegal type store for and(or(Val, ...)
    // pattern if Val is not a load, because we still have to insert another
    // illegal type store for Val.
    if (OrAndPattern && !LI)
      return false;

    StOffset = computeStOffset(MR, TBits, *DL);
  }
  IntegerType *NewTy = Type::getIntNTy(*Context, MR.Width);
  if (TLI->isNarrowingExpensive(EVT::getEVT(StoreTy), EVT::getEVT(NewTy)))
    return false;

  // Start shrinking the size of the store.
  IRBuilder<> Builder(*Context);
  Builder.SetInsertPoint(&SI);
  Value *NewPtr = createNewPtr(Ptr, StOffset, NewTy, Builder);

  if (StOffset)
    Align = MinAlign(StOffset, Align);

  LoadInst *NewLI;
  if (MR.ShrinkWithMaskedVal) {
    // Check if we need to split the original store and generate a new
    // store for the OrigVal.
    if (OrigVal && needNewStore(*OrigVal, SI)) {
      StoreInst *NewSI = cast<StoreInst>(SI.clone());
      NewSI->setOperand(0, OrigVal);
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
  } else {
    // MemorySSA update for the shrinked load.
    NewLI = cast<LoadInst>(LI->clone());
    NewLI->mutateType(NewTy);
    NewLI->setOperand(0, NewPtr);
    NewLI->setAlignment(Align);
    Builder.Insert(NewLI, "load.trunc");
    MemoryDef *OldMemAcc = cast<MemoryDef>(MSSA->getMemoryAccess(&SI));
    MemoryAccess *Def = OldMemAcc->getDefiningAccess();
    MSSAUpdater->createMemoryAccessBefore(NewLI, Def, OldMemAcc);
  }

  // Create the New Value to store.
  Value *NewVal = nullptr;
  APInt ModifiedCst = Cst->getValue().lshr(MR.Shift).trunc(MR.Width);
  ConstantInt *NewCst = ConstantInt::get(*Context, ModifiedCst);
  if (OrAndPattern) {
    // Shift and truncate MaskedVal.
    Value *Trunc;
    if (auto MVCst = dyn_cast<ConstantInt>(MaskedVal)) {
      ModifiedCst = MVCst->getValue().lshr(MR.Shift).trunc(MR.Width);
      Trunc = ConstantInt::get(*Context, ModifiedCst);
    } else {
      Value *ShiftedVal = MR.Shift
                              ? Builder.CreateLShr(MaskedVal, MR.Shift, "lshr")
                              : MaskedVal;
      Trunc = Builder.CreateTruncOrBitCast(ShiftedVal, NewTy, "trunc");
    }
    if (MR.ShrinkWithMaskedVal) {
      NewVal = Trunc;
    } else {
      Value *NewAnd = Builder.CreateAnd(NewLI, NewCst, "and.trunc");
      NewVal = Builder.CreateOr(NewAnd, Trunc, "or.trunc");
    }
  } else {
    BinaryOperator *BOP = cast<BinaryOperator>(Val);
    switch (BOP->getOpcode()) {
    default:
      llvm_unreachable("BOP can only be And/Or/Xor");
    case Instruction::And:
      NewVal = Builder.CreateAnd(NewLI, NewCst, "and.trunc");
      break;
    case Instruction::Or:
      NewVal = Builder.CreateOr(NewLI, NewCst, "or.trunc");
      break;
    case Instruction::Xor:
      NewVal = Builder.CreateXor(NewLI, NewCst, "xor.trunc");
      break;
    }
  }

  // Create the new store and remove the old one.
  StoreInst *NewSI = cast<StoreInst>(SI.clone());
  NewSI->setOperand(0, NewVal);
  NewSI->setOperand(1, NewPtr);
  NewSI->setAlignment(Align);
  Builder.Insert(NewSI);

  DEBUG(dbgs() << "MemShrink: Replace" << SI << " with" << *NewSI << "\n");
  // MemorySSA update for the shrinked store.
  MemoryDef *OldMemAcc = cast<MemoryDef>(MSSA->getMemoryAccess(&SI));
  MemoryAccess *Def = OldMemAcc->getDefiningAccess();
  MemoryAccess *NewMemAcc =
      MSSAUpdater->createMemoryAccessBefore(NewSI, Def, OldMemAcc);
  OldMemAcc->replaceAllUsesWith(NewMemAcc);
  MSSAUpdater->removeMemoryAccess(OldMemAcc);

  markCandForDeletion(*cast<Instruction>(Val));
  SI.eraseFromParent();
  NumStoreShrinked++;
  return true;
}

/// The driver of shrinking and final dead instructions cleanup.
bool MemAccessShrinkingPass::tryShrinkOnInst(Instruction &Inst) {
  StoreInst *SI = dyn_cast<StoreInst>(&Inst);
  if (EnableStoreShrink && SI)
    return reduceLoadOpsStoreWidth(*SI);
  if (EnableLoadShrink && (isa<TruncInst>(&Inst) || isa<BinaryOperator>(&Inst)))
    return reduceLoadOpsWidth(Inst);
  return false;
}

/// Check the range of Cst containing non-zero bit is within \p MR when
/// \p AInB is true. If \p AInB is false, check MR is within the range
/// of Cst.
static bool CompareAPIntWithModRange(APInt &AI, ModRange &MR, bool AInB) {
  unsigned LZ = AI.countLeadingZeros();
  unsigned TZ = AI.countTrailingZeros();
  unsigned TBits = AI.getBitWidth();
  if (AInB && (TZ < MR.Shift || (TBits - LZ) > MR.Width + MR.Shift))
    return false;

  if (!AInB && (MR.Shift < TZ || MR.Width + MR.Shift > (TBits - LZ)))
    return false;
  return true;
}

/// Check if there is overlap between range \MR and \OtherMR.
static bool nonoverlap(ModRange &MR, ModRange &OtherMR) {
  return (MR.Shift > OtherMR.Shift + OtherMR.Width - 1) ||
         (OtherMR.Shift > MR.Shift + MR.Width - 1);
}

/// Check there is no instruction between *LI and IN which may clobber
/// the MemoryLocation specified by MR. We relax it a little bit to allow
/// clobber instruction in the same BB as IN, if that happens we need to
/// set the NewInsertPt to be the new location to insert the shrinked load.
bool MemAccessShrinkingPass::hasClobberBetween(Instruction &From,
                                               Instruction &To,
                                               const MemoryLocation &Mem,
                                               Instruction *&NewInsertPt) {
  assert(DT->dominates(&From, &To) && "From doesn't dominate To");
  const BasicBlock *FBB = From.getParent();
  const BasicBlock *TBB = To.getParent();
  // Collect BasicBlocks to scan between FBB and TBB into BBSet.
  // Limit the maximum number of BasicBlocks to 3 to protect compile time.
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
        if (MSSAWalker->getClobberingMemoryAccess(const_cast<MemoryDef *>(MD),
                                                  Mem) == MD) {
          if (TBB == Inst->getParent()) {
            if (!NewInsertPt || DT->dominates(Inst, NewInsertPt))
              NewInsertPt = Inst;
            continue;
          }
          return true;
        }
      }
    }
  }
  return false;
}

/// Match \p Val to the pattern:
/// Bop(...Bop(V, Cst_1), Cst_2, ..., Cst_N) and pattern:
/// or(and(...or(and(LI, Cst_1), MaskedVal_1), ..., Cse_N), MaskedVal_N),
/// and find the final load to be shrinked.
/// All the Bop instructions in the first pattern and the final load will
/// be added into \p Insts and will be shrinked afterwards.
bool MemAccessShrinkingPass::matchInstsToShrink(
    Value *Val, ModRange &MR, SmallVectorImpl<Instruction *> &Insts) {
  Value *Opd;
  ConstantInt *Cst;
  unsigned TBits = DL->getTypeSizeInBits(Val->getType());

  // Match the pattern:
  // Bop(...Bop(V, Cst_1), Cst_2, ..., Cst_N), Bop can be Add/Sub/And/Or/Xor.
  // All these Csts cannot have one bits outside of range MR, so that we can
  // shrink these Bops to be MR.Width bits integer type.
  while (match(Val, m_Add(m_Value(Opd), m_ConstantInt(Cst))) ||
         match(Val, m_Sub(m_Value(Opd), m_ConstantInt(Cst))) ||
         match(Val, m_And(m_Value(Opd), m_ConstantInt(Cst))) ||
         match(Val, m_Or(m_Value(Opd), m_ConstantInt(Cst))) ||
         match(Val, m_Xor(m_Value(Opd), m_ConstantInt(Cst)))) {
    addSavedInst(*cast<Instruction>(Val));
    APInt AI = Cst->getValue().zextOrTrunc(TBits);
    if (!CompareAPIntWithModRange(AI, MR, true))
      return false;
    Insts.push_back(cast<Instruction>(Val));
    Val = Opd;
  }

  LoadInst *LI;
  if ((LI = dyn_cast<LoadInst>(Val))) {
    addSavedInst(*LI);
    Insts.push_back(LI);
    return true;
  }

  // Match the pattern:
  // or(and(...or(and(LI, Cst_1), MaskedVal_1), ..., Cse_N), MaskedVal_N)
  // Analyze the ModRange of each or(and(...)) pair and see if it has
  // any overlap with MR. If any overlap is found, we cannot do shrinking
  // for the final Load LI.
  Value *MaskedVal;
  Value *OrigVal;
  ModRange OtherMR;
  while (match(Val, m_c_Or(m_And(m_Load(LI), m_ConstantInt(Cst)),
                           m_Value(MaskedVal))) ||
         (match(Val, m_Or(m_And(m_Value(OrigVal), m_ConstantInt(Cst)),
                          m_Value(MaskedVal))) &&
          match(OrigVal, m_c_Or(m_And(m_Value(), m_Value()), m_Value()))) ||
         (match(Val, m_Or(m_Value(MaskedVal),
                          m_And(m_Value(OrigVal), m_ConstantInt(Cst)))) &&
          match(OrigVal, m_c_Or(m_And(m_Value(), m_Value()), m_Value())))) {
    addSavedInst(*cast<Instruction>(Val));
    OtherMR.Shift = 0;
    OtherMR.Width = TBits;
    // Analyze ModRange OtherMR of A from or(and(A, Cst), MaskedVal) pattern.
    analyzeOrAndPattern(*MaskedVal, *Cst, OtherMR, TBits);
    // If OtherMR has overlap with MR, we cannot shrink load to its MR portion.
    if (!nonoverlap(MR, OtherMR))
      return false;
    if (LI) {
      Insts.push_back(LI);
      return true;
    }
    Val = OrigVal;
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

/// Shrink \p Insts according to the range MR.
Value *MemAccessShrinkingPass::shrinkInsts(
    Value *NewPtr, ModRange &MR, SmallVectorImpl<Instruction *> &Insts,
    Instruction *NewInsertPt, unsigned ResShift, IRBuilder<> &Builder) {
  Value *NewVal;
  IntegerType *NewTy = Type::getIntNTy(*Context, MR.Width);
  Instruction &InsertPt = *Builder.GetInsertPoint();
  for (Instruction *I : make_range(Insts.rbegin(), Insts.rend())) {
    if (LoadInst *LI = dyn_cast<LoadInst>(I)) {
      unsigned TBits = DL->getTypeSizeInBits(LI->getType());
      unsigned StOffset = computeStOffset(MR, TBits, *DL);
      unsigned Align = MinAlign(StOffset, LI->getAlignment());
      // If we use a new insertion point, we have to recreate the NewPtr in
      // the new place.
      if (NewInsertPt) {
        Builder.SetInsertPoint(NewInsertPt);
        RecursivelyDeleteTriviallyDeadInstructions(NewPtr);
        NewPtr =
            createNewPtr(LI->getPointerOperand(), StOffset, NewTy, Builder);
      }
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
      APInt NewAPInt = Cst->getValue().extractBits(MR.Width, MR.Shift);
      ConstantInt *NewCst =
          ConstantInt::get(*Context, NewAPInt.zextOrTrunc(MR.Width));
      NewVal = Builder.CreateBinOp(BOP->getOpcode(), NewVal, NewCst);
    }
  }
  // Adjust the type to be consistent with the use of input trunc/and
  // instructions.
  IntegerType *UseType = cast<IntegerType>(InsertPt.getType());
  if (DL->getTypeSizeInBits(UseType) != MR.Width)
    NewVal = Builder.CreateZExt(NewVal, UseType, "trunc.zext");
  // Adjust the NewVal with ResShift.
  if (ResShift) {
    ConstantInt *NewCst = ConstantInt::get(UseType, ResShift);
    NewVal = Builder.CreateShl(NewVal, NewCst, "shl");
  }
  return NewVal;
}

/// Determine whether \p Inst will be saved in the final shrinking and update
/// MultiUsesSeen and SavedInsts accordingly. If \p ReplaceAllUses is
/// true, it means we are going to replace all the uses of \p Inst with the
/// shrinked result, so it doesn't matter even if it has multiple uses.
void MemAccessShrinkingPass::addSavedInst(Instruction &Inst,
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
/// ud chain, try to find out a final load and find out the range of the load
/// really being used. That is the range we want to shrink the load to.
/// The general pattern we are looking for is:
///   and/trunc(sequence_of_shl_or_lshr(sequence_of_bops(
///   sequence_of_or_and_pair(load P), ... )))
/// An example:
///   and(lshr(add(or(and(load P, -65536), MaskedVal), 0x1000000000000), 48),
///   255)
/// The actual modified range of load is [48, 56). The value in the range is
/// incremented by 1. The sequence above will be transformed to:
///   zext(add(load_i8(P + 48), 1), 64)
bool MemAccessShrinkingPass::reduceLoadOpsWidth(Instruction &IN) {
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
  addSavedInst(IN, true);

  // Get initial MR.
  ModRange MR;
  MR.Shift = 0;
  unsigned ResShift = 0;
  unsigned TBits = DL->getTypeSizeInBits(Val->getType());
  if (AndCst) {
    APInt AI = AndCst->getValue();
    // Check AI has consecutive one bits. The consecutive bits are the
    // range to be used. If the num of trailing zeros are not zero,
    // remember the num in ResShift and the val after shrinking needs
    // to be shifted accordingly.
    MR.Shift = AI.countTrailingZeros();
    ResShift = MR.Shift;
    if (!(AI.lshr(MR.Shift) + 1).isPowerOf2() || MR.Shift == TBits)
      return false;
    MR.Width = AI.getActiveBits() - MR.Shift;
  } else {
    MR.Width = DL->getTypeSizeInBits(Ty);
  }

  // Match a series of LShr or Shl. Adjust MR.Shift accordingly.
  // Note we have to be careful that valid bits may be cleared during
  // back-and-force shifts. We use a all-one Mask APInt to simulate
  // the shifts and track the valid bits after shifts.
  Value *Opd;
  bool isLShr;
  unsigned NewShift = MR.Shift;
  APInt Mask(TBits, -1);
  ConstantInt *Cst;
  // Record the series of LShr or Shl in ShiftRecs.
  SmallVector<std::pair<bool, unsigned>, 4> ShiftRecs;
  while ((isLShr = match(Val, m_LShr(m_Value(Opd), m_ConstantInt(Cst)))) ||
         match(Val, m_Shl(m_Value(Opd), m_ConstantInt(Cst)))) {
    addSavedInst(*cast<Instruction>(Val));
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
  // If all the bits in MR are one in Mask, MR is valid after the shifts.
  if (!CompareAPIntWithModRange(Mask, MR, false))
    return false;
  MR.Shift = NewShift;

  // Specify the proper MR we want to handle.
  if (MR.Shift + MR.Width > TBits)
    return false;
  if (MR.Width != 8 && MR.Width != 16 && MR.Width != 32)
    return false;
  if (MR.Shift % 8 != 0)
    return false;

  // Match other Binary operators like Add/Sub/And/Xor/Or or pattern like
  // And(Or(...And(Or(Val, Cst)))) and find the final load.
  SmallVector<Instruction *, 8> Insts;
  if (!matchInstsToShrink(Val, MR, Insts))
    return false;

  // Expect the final load has been found here.
  assert(!Insts.empty() && "Expect Insts to be not empty");
  LoadInst *LI = dyn_cast<LoadInst>(Insts.back());
  assert(LI && "Last elem in Insts should be a LoadInst");
  if (LI->isVolatile() || !LI->isUnordered())
    return false;

  IntegerType *NewTy = Type::getIntNTy(*Context, MR.Width);
  if (TLI->isNarrowingExpensive(EVT::getEVT(Ty), EVT::getEVT(NewTy)))
    return false;

  // Do legality check to ensure the range of shrinked load is not clobbered
  // from *LI to IN.
  IRBuilder<> Builder(*Context);
  Builder.SetInsertPoint(&IN);
  unsigned StOffset = computeStOffset(MR, TBits, *DL);
  Value *NewPtr =
      createNewPtr(LI->getPointerOperand(), StOffset, NewTy, Builder);
  Instruction *NewInsertPt = nullptr;
  if (hasClobberBetween(*LI, IN, MemoryLocation(NewPtr, MR.Width / 8),
                        NewInsertPt)) {
    RecursivelyDeleteTriviallyDeadInstructions(NewPtr);
    return false;
  }
  if (!isBeneficial(ResShift, Insts)) {
    RecursivelyDeleteTriviallyDeadInstructions(NewPtr);
    return false;
  }

  // Start to shrink instructions in Ops.
  Value *NewVal =
      shrinkInsts(NewPtr, MR, Insts, NewInsertPt, ResShift, Builder);
  DEBUG(dbgs() << "MemShrink: Replace" << IN << " with" << *NewVal << "\n");
  IN.replaceAllUsesWith(NewVal);
  markCandForDeletion(IN);
  NumLoadShrinked++;
  return true;
}

/// Check insts in CandidatesToErase. If they are dead, remove the dead
/// instructions recursively and clean up Memory SSA.
bool MemAccessShrinkingPass::removeDeadInsts() {
  SmallVector<Instruction *, 16> DeadInsts;
  bool Changed = false;

  for (Instruction *I : CandidatesToErase) {
    if (!I || !I->use_empty() || !isInstructionTriviallyDead(I, TLInfo))
      continue;
    assert(DeadInsts.empty() && "DeadInsts is empty at the beginning");
    DeadInsts.push_back(I);

    do {
      I = DeadInsts.pop_back_val();

      // Null out all of the instruction's operands to see if any operand
      // becomes dead as we go.
      for (unsigned i = 0, e = I->getNumOperands(); i != e; ++i) {
        Value *OpV = I->getOperand(i);
        I->setOperand(i, nullptr);

        if (!OpV->use_empty())
          continue;

        // If the operand is an instruction that became dead as we nulled out
        // the operand, and if it is 'trivially' dead, delete it in a future
        // loop iteration.
        if (Instruction *OpI = dyn_cast<Instruction>(OpV)) {
          if (isInstructionTriviallyDead(OpI, TLInfo))
            DeadInsts.push_back(OpI);
          else
            CandidatesToErase.insert(OpI);
        }
      }

      MemoryAccess *MemAcc = MSSA->getMemoryAccess(I);
      if (MemAcc)
        MSSAUpdater->removeMemoryAccess(MemAcc);

      CandidatesToErase.erase(I);
      I->eraseFromParent();

      Changed = true;
    } while (!DeadInsts.empty());
  }
  return Changed;
}

/// Perform the memaccess shrinking optimization for the given function.
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
  bool MadeChange = true;
  bool EverMadeChange = false;
  while (MadeChange) {
    MadeChange = false;
    for (BasicBlock *BB : post_order(&Fn)) {
      auto CurInstIterator = BB->rbegin();
      while (CurInstIterator != BB->rend())
        MadeChange |= tryShrinkOnInst(*CurInstIterator++);
    }
    MadeChange |= removeDeadInsts();
    EverMadeChange |= MadeChange;
  }

  return EverMadeChange;
}
