#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Transforms/Utils/LowerMemIntrinsics.h"

#include "PPC.h"

#define DEBUG_TYPE "ppc-lower-intrinsics"

STATISTIC(MemCpyLE, "Number of memcpy calls expanded into a loop.");

using namespace llvm;

// Options used to tune the size range where memcpy expansions occur.
static cl::opt<unsigned> MemcpyLoopFloor(
    "ppc-memcpy-loop-floor", cl::Hidden, cl::init(80),
    cl::desc(
        "The lower size bound of memcpy calls to get expanded into a loop"));

static cl::opt<unsigned> MemcpyLoopCeil(
    "ppc-memcpy-loop-ceil", cl::Hidden, cl::init(256),
    cl::desc("The upper size bound of memcpy calls to get expanded in a loop"));

// FIXME -- This option should be made to affect both the loop expansions
// as well as the non-loop target-independant expansions.
static cl::opt<bool> MemcpyLoopVectorOperand(
    "ppc-memcpy-loop-use-vector", cl::Hidden, cl::init(false),
    cl::desc("Allow the use of vector memory operands in the expansion of a "
             "memcpy into a loop"));

static cl::opt<bool> MemcpyUnKnownSizeExpansion(
    "ppc-memcpy-unknown-size-exp", cl::Hidden, cl::init(false),
    cl::desc("Expand memcpy calls whose size argument is not a compile-time "
             "constant."));

namespace {

class PPCLowerIntrinsics : public ModulePass {
public:
  static char ID;

  PPCLowerIntrinsics() : ModulePass(ID) {}
  bool runOnModule(Module &M) override;
  StringRef getPassName() const override {
    return "PPC Lower Memory Intrinsics";
  }
};
} // end anonymous namespace

char PPCLowerIntrinsics::ID = 0;

INITIALIZE_PASS(PPCLowerIntrinsics, DEBUG_TYPE,
                "Lower mem-intrinsics into loops", false, false)

// Returns the memory operand type to use for load/stores. If the alignment is
// suitable or the user has specified its 'safe' then we will use vector
// operations. Otherwise we must use double-word.
static Type *getLoopOperandType(unsigned Alignment, LLVMContext &Ctx) {
  if (MemcpyLoopVectorOperand || Alignment >= 16)
    return VectorType::get(Type::getInt32Ty(Ctx), 4U);

  // FIXME is it safe to always used double-word or must we match
  // the alignment restriction to a type with a matching alignment restriction?
  // Specifically in relation for non-cachable memory.
  return Type::getInt64Ty(Ctx);
}

static unsigned getLoopOpTypeSizeInBytes(Type *Ty) {
  if (VectorType *VTy = dyn_cast<VectorType>(Ty)) {
    return VTy->getBitWidth() / 8;
  }

  if (IntegerType *ITy = dyn_cast<IntegerType>(Ty)) {
    return ITy->getBitWidth() / 8;
  }

  assert(false && "Only vector or integer types expected!");
  return 0;
}

static bool shouldExpandMemCpy(MemCpyInst *MC) {
  // If compiling for min size we don't want to expand.
  Function *ParentFunc = MC->getParent()->getParent();
  if (ParentFunc->optForMinSize())
    return false;

  // Expand known sizes within the range [MemcpyLoopFloor, MemcpyLoopCeil].
  ConstantInt *CISize = dyn_cast<ConstantInt>(MC->getLength());
  if (CISize) {
    return CISize->getZExtValue() >= MemcpyLoopFloor &&
           CISize->getZExtValue() <= MemcpyLoopCeil;
  }

  // Otherwise expand unkown sizes ...
  // FIXME make this dependant on profiling information.
  return MemcpyUnKnownSizeExpansion;
}

// Calcultes the memory copy operations needed to copy the rest of the block
// not copied by the loop.
// FIXME take alignment into account?
static void getRemainingOps(SmallVectorImpl<Type *> &OpsOut,
                            unsigned RemainingBytes, LLVMContext &Ctx,
                            unsigned LoopOpSize) {
  Type *CopyTypes[] = {VectorType::get(Type::getInt32Ty(Ctx), 4U),
                       Type::getInt64Ty(Ctx), Type::getInt32Ty(Ctx),
                       Type::getInt16Ty(Ctx), Type::getInt8Ty(Ctx)};

  for (auto OpTy : CopyTypes) {
    unsigned OpSize = getLoopOpTypeSizeInBytes(OpTy);
    if (OpSize > LoopOpSize)
      continue;
    while (RemainingBytes >= OpSize) {
      RemainingBytes -= OpSize;
      OpsOut.push_back(OpTy);
    }
  }
  assert(RemainingBytes == 0);
}

static unsigned getUnrollFactor(ConstantInt *CLength, unsigned OperandSize) {
  // Don't unroll for unkown sizes.
  if (!CLength) {
    return 1;
  }

  uint64_t Length = CLength->getZExtValue();
  if (2 * OperandSize <= Length) {
    return 2;
  }

  return 1;
}

// Expands a memcpy intrinsic call using a loop.
static void ppcExpandMemCpyAsLoop(MemCpyInst *MI) {
  // Original basic block before the call instruction becomes the 'pre-loop'
  // basic block. What is split out after the call instruction becomes the
  // 'post-loop' (or 'post-loops') basic-block.
  BasicBlock *PreLoopBB = MI->getParent();
  BasicBlock *PostLoopBB =
      PreLoopBB->splitBasicBlock(MI, "post-loop-memcpy-expansion");

  Function *ParentFunc = PreLoopBB->getParent();
  LLVMContext &Ctx = PreLoopBB->getContext();
  bool IsVolatile = MI->isVolatile();

  // need the alignment to determine what memory operand type to use.
  unsigned Alignment = MI->getAlignmentCst() ? MI->getAlignment() : 1;
  Type *LoopOpType = getLoopOperandType(Alignment, Ctx);
  unsigned LoopOpTypeSize = getLoopOpTypeSizeInBytes(LoopOpType);

  // Length of the copy and the type of the length argument.
  Value *Length = MI->getLength();
  Type *LengthType = Length->getType();
  ConstantInt *CILength = dyn_cast<ConstantInt>(Length);
  unsigned UnrollFactor = getUnrollFactor(CILength, LoopOpTypeSize);
  // How many bytes are copied each loop iteration.
  uint64_t LoopByteCount = UnrollFactor * LoopOpTypeSize;

  // Need the address-space of the arguments to be able to cast the args to
  // proper operand type for the loop body.
  Value *Src = MI->getSource();
  Value *Dst = MI->getDest();
  unsigned SrcAS = cast<PointerType>(Src->getType())->getAddressSpace();
  unsigned DstAS = cast<PointerType>(Dst->getType())->getAddressSpace();
  PointerType *SrcOperandType = PointerType::get(LoopOpType, SrcAS);
  PointerType *DstOperandType = PointerType::get(LoopOpType, DstAS);

  // the length type is need as an IntegerType in several places to create
  // ConstantInt values.
  IntegerType *ILengthType = dyn_cast<IntegerType>(LengthType);
  assert(ILengthType);

  // Fill in the instructions needed before the loop body.
  // This is the runtime loop count + residual calculations if the size is
  // not known at compile time, as well as casting the arguments to the loop
  // operand types.
  IRBuilder<> PLBuilder(PreLoopBB->getTerminator());
  Src = PLBuilder.CreateBitCast(Src, SrcOperandType);
  Dst = PLBuilder.CreateBitCast(Dst, DstOperandType);

  Value *RuntimeLoopCount = 0;
  Value *RuntimeResidual = 0;
  Value *RuntimeBytesCopied = 0;
  if (!CILength) {
    // FIXME can optimize to shift/mask when byteCount is a power of 2.
    ConstantInt *CIByteCount = ConstantInt::get(ILengthType, LoopByteCount);
    RuntimeLoopCount = PLBuilder.CreateUDiv(Length, CIByteCount);
    RuntimeResidual = PLBuilder.CreateURem(Length, CIByteCount);
    RuntimeBytesCopied = PLBuilder.CreateSub(Length, RuntimeResidual);
  }

  // Create the loop body. Since the successor will differ depending on whether
  // the size is known or not, set the successor later.
  BasicBlock *LoopBB =
      BasicBlock::Create(Ctx, "loop-memcpy-expansion", ParentFunc, nullptr);
  IRBuilder<> LoopBuilder(LoopBB);
  PHINode *LoopIndex = LoopBuilder.CreatePHI(LengthType, 0, "loop-index");
  LoopIndex->addIncoming(ConstantInt::get(LengthType, 0), PreLoopBB);

  // Create the loads.
  SmallVector<Value *, 4> Loads;
  for (unsigned i = 0; i != UnrollFactor; ++i) {
    Value *Index =
        LoopBuilder.CreateAdd(LoopIndex, ConstantInt::get(LengthType, i));
    Value *GEP = LoopBuilder.CreateGEP(LoopOpType, Src, Index);
    Loads.push_back(LoopBuilder.CreateLoad(GEP, IsVolatile));
  }

  // Create the Stores.
  for (unsigned i = 0; i != UnrollFactor; ++i) {
    Value *Index =
        LoopBuilder.CreateAdd(LoopIndex, ConstantInt::get(LengthType, i));
    Value *GEP = LoopBuilder.CreateGEP(LoopOpType, Dst, Index);
    LoopBuilder.CreateStore(Loads[i], GEP, IsVolatile);
  }

  // Update the loop counter.
  Value *NewIndex = LoopBuilder.CreateAdd(
      LoopIndex, ConstantInt::get(LengthType, UnrollFactor));
  LoopIndex->addIncoming(NewIndex, LoopBB);

  if (CILength) {
    // Finish up the loop for known-sizes.
    PreLoopBB->getTerminator()->setSuccessor(0, LoopBB);

    // Create the loops branch condition.
    uint64_t LoopEndCount = CILength->getZExtValue() / LoopByteCount;
    LoopEndCount *= UnrollFactor;
    Constant *LoopEndCI = ConstantInt::get(LengthType, LoopEndCount);
    LoopBuilder.CreateCondBr(LoopBuilder.CreateICmpULT(NewIndex, LoopEndCI),
                             LoopBB, PostLoopBB);

    uint64_t BytesCopied = LoopEndCount * LoopOpTypeSize;
    uint64_t RemainingBytes = CILength->getZExtValue() - BytesCopied;
    if (RemainingBytes) {
      IRBuilder<> RBuilder(PostLoopBB->getFirstNonPHI());
      SmallVector<Type *, 5> RemainingOps;
      getRemainingOps(RemainingOps, RemainingBytes, Ctx, LoopOpTypeSize);

      for (auto OpTy : RemainingOps) {
        // Calaculate the new index
        unsigned OperandSize = getLoopOpTypeSizeInBytes(OpTy);
        uint64_t GepIndex = BytesCopied / OperandSize;
        assert(GepIndex * OperandSize == BytesCopied &&
               "Division should have no Remainder!");

        // Create Load.
        PointerType *SrcPtrType = PointerType::get(OpTy, SrcAS);
        Value *CastedSrc = Src->getType() == SrcPtrType
                               ? Src
                               : RBuilder.CreateBitCast(Src, SrcPtrType);
        Value *SrcGep = RBuilder.CreateGEP(
            OpTy, CastedSrc, ConstantInt::get(LengthType, GepIndex));
        Value *Load = RBuilder.CreateLoad(SrcGep, IsVolatile);

        // Create Store.
        PointerType *DstPtrType = PointerType::get(OpTy, DstAS);
        Value *CastedDst = Dst->getType() == DstPtrType
                               ? Dst
                               : RBuilder.CreateBitCast(Dst, DstPtrType);
        Value *DstGep = RBuilder.CreateGEP(
            OpTy, CastedDst, ConstantInt::get(LengthType, GepIndex));
        RBuilder.CreateStore(Load, DstGep, IsVolatile);

        // Increment Bytes Copied.
        BytesCopied += OperandSize;
      }
      assert(BytesCopied == CILength->getZExtValue() &&
             "Bytes copied should match size in the call!");
    }
  } else {
    // Finish up the loop for unknown sizes.

    // Basic block for the loop to copy the residual
    BasicBlock *ResLoopBB = BasicBlock::Create(Ctx, "loop-memcpy-residual",
                                               PreLoopBB->getParent(), nullptr);
    // Basic block to jump to when the copy size is less then the size copied in
    // BasicBlock to decide whether to execute the residual loop or not.
    BasicBlock *ResHeaderBB = BasicBlock::Create(
        Ctx, "loop-memcpy-residual-header", PreLoopBB->getParent(), nullptr);

    // Need to update the pre-loop basic block to branch to the correct place.
    // branch to the main loop if the count is non-zero, branch to the residual
    // loop if the copy size is smaller then 1 iteration of the main loop but
    // non-zero and finally branch to after the residual loop if the memcpy
    //  size is zero.
    ConstantInt *Zero = ConstantInt::get(ILengthType, 0U);
    // The split has set the successor to the post-loop bb, however that is no
    // longer a successor, so unlink it.`
    PLBuilder.CreateCondBr(PLBuilder.CreateICmpNE(RuntimeLoopCount, Zero),
                           LoopBB, ResHeaderBB);
    PreLoopBB->getTerminator()->eraseFromParent();

    LoopBuilder.CreateCondBr(
        LoopBuilder.CreateICmpULT(NewIndex, RuntimeLoopCount), LoopBB,
        ResHeaderBB);

    // Determine if we need to branch to the residual loop or bypass it.
    IRBuilder<> RHBuilder(ResHeaderBB);
    RHBuilder.CreateCondBr(RHBuilder.CreateICmpNE(RuntimeResidual, Zero),
                           ResLoopBB, PostLoopBB);

    // Copy the residual with single byte load/store loop.
    IRBuilder<> ResBuilder(ResLoopBB);
    PHINode *ResidualIndex =
        ResBuilder.CreatePHI(LengthType, 0, "residual-loop-index");
    ResidualIndex->addIncoming(Zero, ResHeaderBB);

    Type *Int8Type = Type::getInt8Ty(Ctx);
    Value *SrcAsInt8 =
        ResBuilder.CreateBitCast(Src, PointerType::get(Int8Type, SrcAS));
    Value *DstAsInt8 =
        ResBuilder.CreateBitCast(Dst, PointerType::get(Int8Type, DstAS));
    Value *FullOffset = ResBuilder.CreateAdd(RuntimeBytesCopied, ResidualIndex);
    Value *SrcGep = ResBuilder.CreateGEP(Int8Type, SrcAsInt8, FullOffset);
    Value *Load = ResBuilder.CreateLoad(SrcGep, IsVolatile);
    Value *DstGep = ResBuilder.CreateGEP(Int8Type, DstAsInt8, FullOffset);
    ResBuilder.CreateStore(Load, DstGep, IsVolatile);

    Value *NewIndex =
        ResBuilder.CreateAdd(ResidualIndex, ConstantInt::get(LengthType, 1));
    ResidualIndex->addIncoming(NewIndex, ResLoopBB);

    // Create the loop branch condition.
    ResBuilder.CreateCondBr(ResBuilder.CreateICmpULT(NewIndex, RuntimeResidual),
                            ResLoopBB, PostLoopBB);
  }
}

static bool expandMemcopies(Function &F) {
  bool AnyExpanded = false;

  // loop over all memcpy calls
  for (auto I = F.user_begin(), E = F.user_end(); I != E; ++I) {
    MemCpyInst *MC = dyn_cast<MemCpyInst>(*I);
    assert(MC && "Must be a MemcpyInst!");
    if (shouldExpandMemCpy(MC)) {
      ppcExpandMemCpyAsLoop(MC);
      MC->eraseFromParent();
      AnyExpanded = true;
      MemCpyLE += 1;
    }
  }

  return AnyExpanded;
}

bool PPCLowerIntrinsics::runOnModule(Module &M) {
  bool Modified = false;

  for (Function &F : M) {
    if (!F.isDeclaration())
      continue;

    switch (F.getIntrinsicID()) {
    default:
      break;
    case Intrinsic::memcpy:
      Modified = expandMemcopies(F);
    }
  }

  return Modified;
}

ModulePass *llvm::createPPCLowerIntrinsicsPass() {
  return new PPCLowerIntrinsics();
}
