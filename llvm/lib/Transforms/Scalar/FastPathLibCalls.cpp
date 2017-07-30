//===-- FastPathLibCalls.cpp ----------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/Scalar/FastPathLibCalls.h"
#include "llvm/ADT/Sequence.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/Analysis/BlockFrequencyInfo.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/PatternMatch.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
using namespace llvm;

#define DEBUG_TYPE "fastpathlibcals"

STATISTIC(NumFastPaths, "Number of inserted fast paths");

static cl::opt<int> ForceMaxOpByteSize(
    "fast-path-force-max-op-byte-size", cl::init(0),
    cl::desc("Forces a specific max operation byte size for library function "
             "fast paths, overriding the target."),
    cl::Hidden);

static cl::opt<int> ForceMaxIterations(
    "fast-path-force-max-iterations", cl::init(0),
    cl::desc("Forces a specific max iterations for library "
             "function fast paths, overriding the target. Only has an effect "
             "if the max operation byte size is also forced."),
    cl::Hidden);

namespace {
class LibCallVisitor : private InstVisitor<LibCallVisitor, bool> {
  using BaseT = InstVisitor<LibCallVisitor, bool>;
  friend BaseT;

  const DataLayout &DL;
  AssumptionCache &AC;
  DominatorTree &DT;
  TargetLibraryInfo &TLI;
  TargetTransformInfo &TTI;

public:
  LibCallVisitor(const DataLayout &DL, AssumptionCache &AC, DominatorTree &DT,
                 TargetLibraryInfo &TLI, TargetTransformInfo &TTI)
      : DL(DL), AC(AC), DT(DT), TLI(TLI), TTI(TTI) {}

  /// Visit every instruction in the function and return if any changes were
  /// made.
  ///
  /// This hides the various entry points of the base class so that we can
  /// implement our desired visit and return semantics.
  bool visit(Function &F) {
    bool Changed = false;

    // Loop somewhat carefully over the instructions as we will be moving them
    // when making changes.
    // FIXME: This is the worst possible way to iterate over instructions, but
    // it doesn't crash when the instruction list mutates.
    SmallVector<Instruction *, 16> Insts;
    for (Instruction &I : instructions(F))
      Insts.push_back(&I);
    for (Instruction *I : Insts)
      Changed |= BaseT::visit(*I);

    return Changed;
  }

private:
  // Base case implementation.
  bool visitInstruction(Instruction &) { return false; }

  /// Checks whether a value is known non-zero at a particular location.
  bool isKnownNonZero(Value *V, Instruction &I) {
    // If value tracking knows enough, we're done.
    if (llvm::isKnownNonZero(V, DL, /*Depth*/ 0, &AC, &I, &DT))
      return true;

    // Otherwise we implement a really lame version of PredicateInfo.
    // FIXME: We should actually use PredicateInfo or some other more advanced
    // mechanism to analyze predicates.
    //
    // The lame version simply walks up the dominator tree looking for branches
    // on a test against zero where the non-zero edge dominates the location.
    int Depth = 0;
    for (DomTreeNode *N = DT.getNode(I.getParent()); N && Depth < 10;
         N = N->getIDom()) {
      auto *BB = N->getBlock();
      auto *BI = dyn_cast<BranchInst>(BB->getTerminator());
      if (!BI || !BI->isConditional())
        continue;

      auto *Cmp = dyn_cast<ICmpInst>(BI->getCondition());
      if (!Cmp || Cmp->getOperand(0) != V ||
          Cmp->getOperand(1) != ConstantInt::get(V->getType(), 0))
        continue;

      BasicBlock *NonZeroBB;
      switch (Cmp->getPredicate()) {
      default:
        llvm_unreachable("Invalid integer comparison predicate!");

      case ICmpInst::ICMP_NE:
      case ICmpInst::ICMP_UGT:
      case ICmpInst::ICMP_ULT:
      case ICmpInst::ICMP_SGT:
      case ICmpInst::ICMP_SLT:
        // Predicates where a match precludes equality with zero.
        NonZeroBB = BI->getSuccessor(0);
        break;

      case ICmpInst::ICMP_EQ:
      case ICmpInst::ICMP_UGE:
      case ICmpInst::ICMP_ULE:
      case ICmpInst::ICMP_SGE:
      case ICmpInst::ICMP_SLE:
        // Predicates where failing to match precludes equality with zero.
        NonZeroBB = BI->getSuccessor(1);
        break;
      }

      // If the non-zero edge dominates the instruction given, we have
      // a non-zero predicate.
      if (DT.dominates({BB, NonZeroBB}, I.getParent()))
        return true;
    }

    return false;
  }

  struct FastPathMemOpFramework {
    BasicBlock *HeadBB;
    BasicBlock *IfBB;
    BasicBlock *ThenBB;
    BasicBlock *ElseBB;
    BasicBlock *TailBB;

    // The memory operation byte size to use.
    int OpByteSize;

    // The `count`, scaled by the op byte size used in the loop, and available
    // within the `if` block.
    Value *Count;

    // The `then` basic block contains a loop, and we make the index of that
    // loop available here for use when populating a particular fast path.
    PHINode *Index;
  };

  template <typename CallableT>
  Optional<FastPathMemOpFramework>
  buildFastPathMemOpFramework(MemIntrinsic &I, CallableT GetSizeInfo) {
    Optional<FastPathMemOpFramework> FastPath = None;

    // First we analyze the IR looking for a good fastpath.

    // Try to match a scaling operation so we can use a coarser fast path.
    Value *Count = I.getLength();
    auto *CountTy = Count->getType();
    int ShiftScale = 0;
    ConstantInt *ShiftScaleC;
    using namespace PatternMatch;
    if (match(I.getLength(),
              m_NUWShl(m_Value(Count), m_ConstantInt(ShiftScaleC)))) {
      // Don't bother with shifts wider than the number of bits in count.
      if (ShiftScaleC->getValue().uge(CountTy->getIntegerBitWidth()))
        return None;
      assert(I.getLength()->getType() == Count->getType() &&
             "Cannot change type with a shift!");
      ShiftScale = (int)ShiftScaleC->getValue().getZExtValue();
    }

    // Compute the alignment, mapping zero to the actual resulting alignment.
    int Alignment = std::max<int>(1, I.getAlignment());
    int MaxOpByteSize = std::min<int>(1 << ShiftScale, Alignment);

    auto SizeInfo = GetSizeInfo(MaxOpByteSize);

    // For testing, we may have overrides for the TTI selected parameters.
    if (ForceMaxOpByteSize.getNumOccurrences() > 0) {
      SizeInfo.OpByteSize = std::min<int>(MaxOpByteSize, ForceMaxOpByteSize);
      SizeInfo.MaxIterations = ForceMaxIterations;
    }

    // If we won't fast-path any iterations, bail.
    if (SizeInfo.MaxIterations == 0)
      return FastPath;

    assert(SizeInfo.OpByteSize <= Alignment && "Stores would be underaligned!");

    // Otherwise build an actual fast path.
    ++NumFastPaths;
    FastPath = {};
    FastPath->OpByteSize = SizeInfo.OpByteSize;
    FastPath->HeadBB = I.getParent();
    IRBuilder<> IRB(&I);

    // If necessary, check for zero and bypass everything.
    if (!isKnownNonZero(Count, I)) {
      auto *ZeroCond = cast<Instruction>(
          IRB.CreateICmpNE(Count, ConstantInt::get(CountTy, 0), "zero_cond"));
      TerminatorInst *IfTerm = SplitBlockAndInsertIfThen(
          ZeroCond, &I, /*Unreachable*/ false, /*BranchWeights*/ nullptr, &DT);
      FastPath->IfBB = IfTerm->getParent();
      FastPath->IfBB->setName(Twine(FastPath->HeadBB->getName()) + ".if");
      FastPath->TailBB = I.getParent();
      // Lift the operation into its basic block.
      I.moveBefore(IfTerm);
    } else {
      FastPath->IfBB = FastPath->HeadBB;
      FastPath->TailBB = SplitBlock(FastPath->HeadBB,
                                    &*std::next(BasicBlock::iterator(I)), &DT);
    }
    FastPath->TailBB->setName(Twine(FastPath->HeadBB->getName()) + ".tail");
    IRB.SetInsertPoint(&I);

    // Adjust the count based on the op size we want for the loop.
    auto AdjustCountAndShiftScaleForOpSize =
        [&](Value *Count, Value *ByteSize, int ShiftScale,
            int OpByteSize) -> std::pair<Value *, int> {
      assert(OpByteSize > 0 && isPowerOf2_32(OpByteSize) &&
             "Invalid operation byte size!");

      // For one byte stores simply reset to the original byte size.
      if (OpByteSize == 1)
        return {ByteSize, 0};

      // When the op shift scale matches, we don't need to adjust anything.
      int OpShiftScale = Log2_32(OpByteSize);
      if (ShiftScale == OpShiftScale)
        return {Count, ShiftScale};

      assert(ShiftScale > OpShiftScale && "Cannot have a wider op than shift!");

      return {IRB.CreateShl(
                  Count, ShiftScale - OpShiftScale, "loop_count",
                  /*HasNUW*/ true,
                  /*HasNSW*/ cast<Instruction>(ByteSize)->hasNoSignedWrap()),
              OpShiftScale};
    };
    std::tie(Count, ShiftScale) = AdjustCountAndShiftScaleForOpSize(
        Count, I.getLength(), ShiftScale, SizeInfo.OpByteSize);
    FastPath->Count = Count;

    // Now create the condition for using the fast path.
    auto *Cond = cast<Instruction>(IRB.CreateICmpULE(
        Count, ConstantInt::get(CountTy, SizeInfo.MaxIterations),
        "count_cond"));

    // Split into an if-then-else FastPath based on the condition.
    // FIXME: We should use profile information about the count (if available)
    // to guide the metadata on this branch.
    auto *ThenTerm = cast<BranchInst>(SplitBlockAndInsertIfThen(
        Cond, &I, /*Unreachable*/ false, /*BranchWeights*/ nullptr, &DT));

    FastPath->ThenBB = ThenTerm->getParent();
    FastPath->ThenBB->setName(Twine(FastPath->HeadBB->getName()) +
                              ".fast_path_then");
    FastPath->ElseBB = I.getParent();
    FastPath->ElseBB->setName(Twine(FastPath->HeadBB->getName()) +
                              ".fast_path_else");

    // Build the fast-path loop in the then block and save the index.
    ThenTerm->eraseFromParent();
    IRB.SetInsertPoint(FastPath->ThenBB);
    FastPath->Index = IRB.CreatePHI(CountTy, /*NumReservedValues*/ 2, "index");
    FastPath->Index->addIncoming(ConstantInt::get(CountTy, 0), FastPath->IfBB);
    auto *NextIndex = IRB.CreateAdd(FastPath->Index,
                                    ConstantInt::get(CountTy, 1), "next_index");
    auto *LoopCond = IRB.CreateICmpEQ(NextIndex, Count, "loop_cond");
    IRB.CreateCondBr(LoopCond, FastPath->TailBB, FastPath->ThenBB);
    FastPath->Index->addIncoming(NextIndex, FastPath->ThenBB);

    // If the tail's current IDom is the else, we need to update it now that
    // the then block directly connects to it.
    DomTreeNode *TailN = DT.getNode(FastPath->TailBB);
    if (TailN->getIDom()->getBlock() == FastPath->ElseBB)
      DT.changeImmediateDominator(TailN, DT.getNode(FastPath->HeadBB));

    return FastPath;
  }

  bool visitMemCpyInst(MemCpyInst &I) {
    if (I.isVolatile())
      return false;
    Value *ByteSize = I.getLength();
    // Constant sizes don't need a fast path, we can code generate an optimal
    // lowering.
    if (isa<Constant>(ByteSize))
      return false;

    auto FastPath = buildFastPathMemOpFramework(I, [&](int MaxOpByteSize) {
      return TTI.getMemcpyInlineFastPathSizeInfo(MaxOpByteSize);
    });
    if (!FastPath)
      return false;

    // Now build the inner part of the fastpath for this rotine.
    IRBuilder<> IRB(FastPath->ThenBB->getFirstNonPHI());

    // Cast the pointer to the desired type.
    IntegerType *ValTy = IRB.getIntNTy(FastPath->OpByteSize * 8);
    PointerType *PtrTy = ValTy->getPointerTo();
    Value *Dst = PtrTy == I.getRawDest()->getType()
                     ? I.getRawDest()
                     : IRB.CreatePointerCast(I.getDest(), PtrTy, "dst.cast");
    Value *Src = PtrTy == I.getRawSource()->getType()
                     ? I.getRawSource()
                     : IRB.CreatePointerCast(I.getSource(), PtrTy, "src.cast");

    // Build a store loop in the then block.
    Value *Indices[] = {FastPath->Index};
    auto *IndexedDst =
        IRB.CreateInBoundsGEP(ValTy, Dst, Indices, "indexed_dst");
    auto *IndexedSrc =
        IRB.CreateInBoundsGEP(ValTy, Src, Indices, "indexed_src");
    IRB.CreateAlignedStore(IRB.CreateAlignedLoad(IndexedSrc, I.getAlignment()),
                           IndexedDst, I.getAlignment());

    // Compute the byte size within that block to avoid computing it when
    // possible.
    if (FastPath->Count != ByteSize)
      if (auto *ByteSizeI = dyn_cast<Instruction>(ByteSize))
        if (ByteSizeI->hasOneUse())
          ByteSizeI->moveBefore(&I);

    // Return that we changed the function.
    return true;
  }

  bool visitMemSetInst(MemSetInst &I) {
    if (I.isVolatile())
      return false;
    Value *ByteSize = I.getLength();
    // Constant sizes don't need a fast path, we can code generate an optimal
    // lowering.
    if (isa<Constant>(ByteSize))
      return false;

    Value *V = I.getValue();
    assert(V->getType()->isIntegerTy(8) && "Non-i8 value in memset!");
    auto *CV = dyn_cast<ConstantInt>(V);

    auto FastPath = buildFastPathMemOpFramework(I, [&](int MaxOpByteSize) {
      // If we don't have a constant value, forcibly cap the size to one so we
      // don't need to scale it.
      if (!CV)
        MaxOpByteSize = 1;

      return TTI.getMemsetInlineFastPathSizeInfo(MaxOpByteSize);
    });
    if (!FastPath)
      return false;

    // Now build the inner part of the fastpath for this rotine.
    IRBuilder<> IRB(FastPath->ThenBB->getFirstNonPHI());

    // Scale up our value if necesasry.
    if (FastPath->OpByteSize > 1) {
      assert(CV && "Cannot scale up non-constant value!");

      IntegerType *ScaledValTy = IRB.getIntNTy(FastPath->OpByteSize * 8);
      APInt RawV = CV->getValue();
      if (RawV.getBitWidth() > 8)
        RawV = RawV.trunc(8);
      V = ConstantInt::get(ScaledValTy,
                           APInt::getSplat(FastPath->OpByteSize * 8, RawV));
    }

    // Cast the pointer to the desired type.
    PointerType *DstTy = V->getType()->getPointerTo();
    Value *Dst = DstTy == I.getRawDest()->getType()
                     ? I.getRawDest()
                     : IRB.CreatePointerCast(I.getDest(), DstTy, "dst.cast");

    // Add the store to the loop in the then block.
    Value *Indices[] = {FastPath->Index};
    auto *IndexedDst =
        IRB.CreateInBoundsGEP(V->getType(), Dst, Indices, "indexed_dst");
    IRB.CreateAlignedStore(V, IndexedDst, I.getAlignment());

    // Compute the byte size within that block to avoid computing it when
    // possible.
    if (FastPath->Count != ByteSize)
      if (auto *ByteSizeI = dyn_cast<Instruction>(ByteSize))
        if (ByteSizeI->hasOneUse())
          ByteSizeI->moveBefore(&I);

    // Return that we changed the function.
    return true;
  }

  bool visitCallSite(CallSite CS) {
    LibFunc F;
    if (!TLI.getLibFunc(CS, F))
      return false;

    switch (F) {
    default:
      // No fast-path logic.
      return false;
    }
  }
};
} // namespace

static bool injectLibCallFastPaths(Function &F, AssumptionCache &AC,
                                   DominatorTree &DT, TargetLibraryInfo &TLI,
                                   TargetTransformInfo &TTI) {
  if (F.optForSize())
    return false;

  return LibCallVisitor(F.getParent()->getDataLayout(), AC, DT, TLI, TTI)
      .visit(F);
}

PreservedAnalyses FastPathLibCallsPass::run(Function &F,
                                            FunctionAnalysisManager &AM) {
  auto &AC = AM.getResult<AssumptionAnalysis>(F);
  auto &DT = AM.getResult<DominatorTreeAnalysis>(F);
  auto &TLI = AM.getResult<TargetLibraryAnalysis>(F);
  auto &TTI = AM.getResult<TargetIRAnalysis>(F);

  if (!injectLibCallFastPaths(F, AC, DT, TLI, TTI))
    return PreservedAnalyses::all();

  DT.verifyDomTree();

  PreservedAnalyses PA;
  PA.preserve<DominatorTreeAnalysis>();
  return PA;
}

namespace {
struct FastPathLibCallsLegacyPass : public FunctionPass {
  static char ID;
  FastPathLibCallsLegacyPass() : FunctionPass(ID) {
    initializeFastPathLibCallsLegacyPassPass(*PassRegistry::getPassRegistry());
  }

  bool runOnFunction(Function &F) override {
    if (skipFunction(F))
      return false;

    auto &AC = getAnalysis<AssumptionCacheTracker>().getAssumptionCache(F);
    auto &DT = getAnalysis<DominatorTreeWrapperPass>().getDomTree();
    auto &TLI = getAnalysis<TargetLibraryInfoWrapperPass>().getTLI();
    auto &TTI = getAnalysis<TargetTransformInfoWrapperPass>().getTTI(F);

    return injectLibCallFastPaths(F, AC, DT, TLI, TTI);
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<AssumptionCacheTracker>();
    AU.addRequired<DominatorTreeWrapperPass>();
    AU.addRequired<TargetLibraryInfoWrapperPass>();
    AU.addRequired<TargetTransformInfoWrapperPass>();

    AU.addPreserved<DominatorTreeWrapperPass>();
  }
};
} // namespace

char FastPathLibCallsLegacyPass::ID = 0;
INITIALIZE_PASS_BEGIN(FastPathLibCallsLegacyPass, "fast-path-lib-calls",
                      "Fast Path Lib Calls", false, false)
INITIALIZE_PASS_DEPENDENCY(AssumptionCacheTracker)
INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
INITIALIZE_PASS_DEPENDENCY(TargetLibraryInfoWrapperPass)
INITIALIZE_PASS_DEPENDENCY(TargetTransformInfoWrapperPass)
INITIALIZE_PASS_END(FastPathLibCallsLegacyPass, "fast-path-lib-calls",
                    "Fast Path Lib Calls", false, false)

Pass *llvm::createFastPathLibCallsLegacyPass() {
  return new FastPathLibCallsLegacyPass();
}
