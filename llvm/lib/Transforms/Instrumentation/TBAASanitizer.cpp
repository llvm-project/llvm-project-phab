//===----- TBAASanitizer.cpp - type-based-aliasing-violation detector -----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file is a part of TBAASanitizer, a type-based-aliasing-violation
// detector.
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/Instrumentation.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/MDBuilder.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/ProfileData/InstrProf.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/MD5.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Regex.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/Local.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"
#include <cctype>

using namespace llvm;

#define DEBUG_TYPE "tbaasan"

static const char *const kTBAAsanModuleCtorName = "tbaasan.module_ctor";
static const char *const kTBAAsanInitName = "__tbaasan_init";
static const char *const kTBAAsanCheckName = "__tbaasan_check";
static const char *const kTBAAsanGVNamePrefix = "__tbaasan_v1_";

static const char *const kTBAAsanShadowMemoryAddress =
    "__tbaasan_shadow_memory_address";
static const char *const kTBAAsanAppMemMask =
    "__tbaasan_app_memory_mask";

static cl::opt<bool> ClWritesAlwaysSetType("tbaasan-writes-always-set-type",
         cl::desc("Writes always set the type"),
         cl::Hidden, cl::init(false));

STATISTIC(NumInstrumentedAccesses, "Number of instrumented accesses");

static Regex AnonNameRegex("^_ZTS.*N[1-9][0-9]*_GLOBAL__N");

namespace {

/// TBAASanitizer: instrument the code in module to find races.
struct TBAASanitizer : public FunctionPass {
  TBAASanitizer() : FunctionPass(ID) {}
  StringRef getPassName() const override;
  void getAnalysisUsage(AnalysisUsage &AU) const override;
  bool runOnFunction(Function &F) override;
  bool doInitialization(Module &M) override;
  static char ID;  // Pass identification, replacement for typeid.

 private:
  typedef SmallDenseMap<const MDNode *,
                        GlobalVariable *, 8> TypeDescriptorsMapTy;
  typedef SmallDenseMap<const MDNode *,
                        std::string, 8> TypeNameMapTy;

  void initializeCallbacks(Module &M);

  Value *getShadowBase(Function &F);
  Value *getAppMemMask(Function &F);
  bool instrumentMemoryAccess(Instruction *I, MemoryLocation &MLoc,
                             Value *&ShadowBase, Value *&AppMemMask,
                             bool SanitizeFunction,
                             TypeDescriptorsMapTy &TypeDescriptors,
                             const DataLayout &DL);
  bool instrumentMemInst(Value *I, Value *&ShadowBase, Value *&AppMemMask,
                         const DataLayout &DL);

  std::string getAnonymousStructIdentifier(const MDNode *MD,
                                           TypeNameMapTy &TypeNames);
  bool generateTypeDescriptor(const MDNode *MD,
                              TypeDescriptorsMapTy &TypeDescriptors,
                              TypeNameMapTy &TypeNames,
                              Module &M);
  bool generateBaseTypeDescriptor(const MDNode *MD,
                                  TypeDescriptorsMapTy &TypeDescriptors,
                                  TypeNameMapTy &TypeNames,
                                  Module &M);

  Type *IntptrTy;
  uint64_t PtrShift;
  IntegerType *OrdTy;

  // Callbacks to run-time library are computed in doInitialization.
  Function *TBAAsanCheck;
  Function *TBAAsanCtorFunction;
  Function *MemmoveFn, *MemcpyFn, *MemsetFn;
};
}  // namespace

char TBAASanitizer::ID = 0;
INITIALIZE_PASS_BEGIN(
    TBAASanitizer, "tbaasan",
    "TBAASanitizer: detects TBAA violations.",
    false, false)
INITIALIZE_PASS_DEPENDENCY(TargetLibraryInfoWrapperPass)
INITIALIZE_PASS_END(
    TBAASanitizer, "tbaasan",
    "TBAASanitizer: detects TBAA violations.",
    false, false)

StringRef TBAASanitizer::getPassName() const { return "TBAASanitizer"; }

void TBAASanitizer::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<TargetLibraryInfoWrapperPass>();
}

FunctionPass *llvm::createTBAASanitizerPass() {
  return new TBAASanitizer();
}

void TBAASanitizer::initializeCallbacks(Module &M) {
  IRBuilder<> IRB(M.getContext());
  OrdTy = IRB.getInt32Ty();

  AttributeList Attr;
  Attr = Attr.addAttribute(M.getContext(), AttributeList::FunctionIndex,
                           Attribute::NoUnwind);
  // Initialize the callbacks.
  TBAAsanCheck = checkSanitizerInterfaceFunction(M.getOrInsertFunction(
      kTBAAsanCheckName, Attr, IRB.getVoidTy(),
        IRB.getInt8PtrTy(), // Pointer to data to be read.
        OrdTy, // Size of the data in bytes.
        IRB.getInt8PtrTy(), // Pointer to type descriptor.
        OrdTy, // Flags.
        nullptr));

  MemmoveFn = checkSanitizerInterfaceFunction(
      M.getOrInsertFunction("memmove", Attr, IRB.getInt8PtrTy(), IRB.getInt8PtrTy(),
                            IRB.getInt8PtrTy(), IntptrTy, nullptr));
  MemcpyFn = checkSanitizerInterfaceFunction(
      M.getOrInsertFunction("memcpy", Attr, IRB.getInt8PtrTy(), IRB.getInt8PtrTy(),
                            IRB.getInt8PtrTy(), IntptrTy, nullptr));
  MemsetFn = checkSanitizerInterfaceFunction(
      M.getOrInsertFunction("memset", Attr, IRB.getInt8PtrTy(), IRB.getInt8PtrTy(),
                            IRB.getInt32Ty(), IntptrTy, nullptr));
}

bool TBAASanitizer::doInitialization(Module &M) {
  const DataLayout &DL = M.getDataLayout();
  IntptrTy = DL.getIntPtrType(M.getContext());
  PtrShift = countTrailingZeros(IntptrTy->getPrimitiveSizeInBits()/8);
  std::tie(TBAAsanCtorFunction, std::ignore) =
    createSanitizerCtorAndInitFunctions(M, kTBAAsanModuleCtorName,
                                        kTBAAsanInitName, /*InitArgTypes=*/{},
                                        /*InitArgs=*/{});

  appendToGlobalCtors(M, TBAAsanCtorFunction, 0);

  return true;
}

static std::string encodeName(StringRef Name) {
  static const char *const LUT = "0123456789abcdef";
  size_t Length = Name.size();

  std::string Output = kTBAAsanGVNamePrefix;
  Output.reserve(Output.size() + 3 * Length);
  for (size_t i = 0; i < Length; ++i) {
    const unsigned char c = Name[i];

    if (isalnum((int)c)) {
      Output.push_back(c);
      continue;
    }

    if (c == '_') {
      Output.append("__");
      continue;
    }

    Output.push_back('_');
    Output.push_back(LUT[c >> 4]);
    Output.push_back(LUT[c & 15]);
  }

  return Output;
}

static bool isAnonymousNamespaceName(StringRef Name) {
  // Types that are in an anonymous namespace are local to this module.
  // FIXME: This should really be marked by the frontend in the metadata
  // instead of having us guess this from the mangled name. Moreover, the regex
  // here can pick up (unlikely) names in the non-reserved namespace (because
  // it needs to search into the type to pick up cases where the type in the
  // anonymous namespace is a template parameter, etc.).
  return AnonNameRegex.match(Name);
}

std::string TBAASanitizer::getAnonymousStructIdentifier(const MDNode *MD,
                                                        TypeNameMapTy &TypeNames) {
  MD5 Hash;

  for (int i = 1, e = MD->getNumOperands(); i < e; i += 2) {
    const MDNode *MemberNode = dyn_cast<MDNode>(MD->getOperand(i));
    if (!MemberNode)
      return "";

    auto TNI = TypeNames.find(MemberNode);
    std::string MemberName;
    if (TNI != TypeNames.end()) {
      MemberName = TNI->second;
    } else {
      if (MemberNode->getNumOperands() < 1)
        return "";
      MDString *MemberNameNode = dyn_cast<MDString>(MemberNode->getOperand(0));
      if (!MemberNameNode)
        return "";
      MemberName = MemberNameNode->getString();
      if (MemberName.empty())
        MemberName = getAnonymousStructIdentifier(MemberNode, TypeNames);
      if (MemberName.empty())
        return "";
      TypeNames[MemberNode] = MemberName;
    }

    Hash.update(MemberName);
    Hash.update("\0");

    uint64_t Offset =
      mdconst::extract<ConstantInt>(MD->getOperand(i+1))->getZExtValue();
    Hash.update(utostr(Offset));
    Hash.update("\0");
  }

  MD5::MD5Result HashResult;
  Hash.final(HashResult);
  return "__anonymous_" + std::string(HashResult.digest().str());
}

bool TBAASanitizer::generateBaseTypeDescriptor(
  const MDNode *MD, TypeDescriptorsMapTy &TypeDescriptors,
  TypeNameMapTy &TypeNames, Module &M) {
  if (MD->getNumOperands() < 1)
    return false;
  
  MDString *NameNode = dyn_cast<MDString>(MD->getOperand(0));
  if (!NameNode)
    return false;

  std::string Name = NameNode->getString();
  if (Name.empty())
    Name = getAnonymousStructIdentifier(MD, TypeNames);
  if (Name.empty())
    return false;
  TypeNames[MD] = Name;
  std::string EncodedName = encodeName(Name);

  GlobalVariable *GV =
    dyn_cast_or_null<GlobalVariable>(M.getNamedValue(EncodedName));
  if (GV) {
    TypeDescriptors[MD] = GV;
    return true;
  }

  SmallVector<std::pair<Constant *, uint64_t>, 8> Members;
  for (int i = 1, e = MD->getNumOperands(); i < e; i += 2) {
    const MDNode *MemberNode = dyn_cast<MDNode>(MD->getOperand(i));
    if (!MemberNode)
      return false;

    Constant *Member;
    auto TDI = TypeDescriptors.find(MemberNode);
    if (TDI != TypeDescriptors.end()) {
      Member = TDI->second;
    } else {
      if (!generateBaseTypeDescriptor(MemberNode, TypeDescriptors,
                                        TypeNames, M))
          return false;

      Member = TypeDescriptors[MemberNode];
    }

    uint64_t Offset =
      mdconst::extract<ConstantInt>(MD->getOperand(i+1))->getZExtValue();

    Members.push_back(std::make_pair(Member, Offset));
  }

  // The descriptor for a scalar is:
  //   [2, member count, [type pointer, offset]..., name]

  LLVMContext &C = MD->getContext();
  Constant *NameData = ConstantDataArray::getString(C, NameNode->getString());
  SmallVector<Type *, 8> TDSubTys;
  SmallVector<Constant *, 8> TDSubData;

  TDSubTys.push_back(IntptrTy);
  TDSubData.push_back(ConstantInt::get(IntptrTy, 2));

  TDSubTys.push_back(IntptrTy);
  TDSubData.push_back(ConstantInt::get(IntptrTy, Members.size()));

  bool ShouldBeComdat = !isAnonymousNamespaceName(NameNode->getString());
  for (auto &Member : Members) {
    TDSubTys.push_back(Member.first->getType());
    TDSubData.push_back(Member.first);

    if (!cast<GlobalVariable>(Member.first)->hasComdat())
      ShouldBeComdat = false;

    TDSubTys.push_back(IntptrTy);
    TDSubData.push_back(ConstantInt::get(IntptrTy, Member.second));
  }

  TDSubTys.push_back(NameData->getType());
  TDSubData.push_back(NameData);

  StructType *TDTy = StructType::get(C, TDSubTys);
  Constant *TD = ConstantStruct::get(TDTy, TDSubData);

  GlobalVariable *TDGV =
    new GlobalVariable(TDTy, true,
                       !ShouldBeComdat ? GlobalValue::InternalLinkage :
                                         GlobalValue::LinkOnceODRLinkage,
                       TD, EncodedName);
  M.getGlobalList().push_back(TDGV);

  if (ShouldBeComdat) {
    Comdat *TDComdat = M.getOrInsertComdat(EncodedName);
    TDGV->setComdat(TDComdat);
  }

  TypeDescriptors[MD] = TDGV;
  return true;
}

bool TBAASanitizer::generateTypeDescriptor(
  const MDNode *MD, TypeDescriptorsMapTy &TypeDescriptors,
  TypeNameMapTy &TypeNames, Module &M) {
  // Here we need to generate a type descriptor corresponding to this TBAA
  // metadata node. Under the current scheme there are three kinds of TBAA
  // metadata nodes: scalar nodes, struct nodes, and struct tag nodes.

  if (MD->getNumOperands() < 3)
    return false;

  const MDNode *BaseNode = dyn_cast<MDNode>(MD->getOperand(0));
  if (!BaseNode)
    return false;

  // This is a struct tag (element-access) node.

  const MDNode *AccessNode = dyn_cast<MDNode>(MD->getOperand(1));
  if (!AccessNode)
    return false;

  Constant *Base;
  auto TDI = TypeDescriptors.find(BaseNode);
  if (TDI != TypeDescriptors.end()) {
    Base = TDI->second;
  } else {
    if (!generateBaseTypeDescriptor(BaseNode, TypeDescriptors, TypeNames, M))
      return false;

    Base = TypeDescriptors[BaseNode];
  }

  Constant *Access;
  TDI = TypeDescriptors.find(AccessNode);
  if (TDI != TypeDescriptors.end()) {
    Access = TDI->second;
  } else {
    if (!generateBaseTypeDescriptor(AccessNode, TypeDescriptors, TypeNames, M))
      return false;

    Access = TypeDescriptors[AccessNode];
  }

  uint64_t Offset =
    mdconst::extract<ConstantInt>(MD->getOperand(2))->getZExtValue();
  std::string EncodedName = std::string(Base->getName()) + "_o_" +
                            utostr(Offset);

  GlobalVariable *GV =
    dyn_cast_or_null<GlobalVariable>(M.getNamedValue(EncodedName));
  if (GV) {
    TypeDescriptors[MD] = GV;
    return true;
  }

  // The descriptor for a scalar is:
  //   [1, base-type pointer, access-type pointer, offset]

  StructType *TDTy = StructType::get(IntptrTy, Base->getType(),
                                     Access->getType(), IntptrTy, nullptr);
  Constant *TD = ConstantStruct::get(TDTy, ConstantInt::get(IntptrTy, 1),
                                     Base, Access,
                                     ConstantInt::get(IntptrTy, Offset),
                                     nullptr);

  bool ShouldBeComdat = cast<GlobalVariable>(Base)->hasComdat();

  GlobalVariable *TDGV =
    new GlobalVariable(TDTy, true,
                       !ShouldBeComdat ? GlobalValue::InternalLinkage :
                                         GlobalValue::LinkOnceODRLinkage,
                       TD, EncodedName);
  M.getGlobalList().push_back(TDGV);

  if (ShouldBeComdat) {
    Comdat *TDComdat = M.getOrInsertComdat(EncodedName);
    TDGV->setComdat(TDComdat);
  }

  TypeDescriptors[MD] = TDGV;
  return true;
}

Value *TBAASanitizer::getShadowBase(Function &F) {
  IRBuilder<> IRB(&F.front().front());
  Value *GlobalShadowAddress = F.getParent()->getOrInsertGlobal(
      kTBAAsanShadowMemoryAddress, IntptrTy);
  return IRB.CreateLoad(GlobalShadowAddress);
}

Value *TBAASanitizer::getAppMemMask(Function &F) {
  IRBuilder<> IRB(&F.front().front());
  Value *GlobalAppMemMask = F.getParent()->getOrInsertGlobal(
      kTBAAsanAppMemMask, IntptrTy);
  return IRB.CreateLoad(GlobalAppMemMask);
}

bool TBAASanitizer::runOnFunction(Function &F) {
  // This is required to prevent instrumenting call to __tbaasan_init from within
  // the module constructor.
  if (&F == TBAAsanCtorFunction)
    return false;
  initializeCallbacks(*F.getParent());

  SmallVector<std::pair<Instruction *, MemoryLocation>, 8> MemoryAccesses;
  SmallSetVector<const MDNode *, 8> TBAAMetadata;
  SmallVector<Value*, 8> MemTypeResetInsts;

  bool Res = false;
  bool SanitizeFunction = F.hasFnAttribute(Attribute::SanitizeTBAA);
  const DataLayout &DL = F.getParent()->getDataLayout();
  const TargetLibraryInfo *TLI =
      &getAnalysis<TargetLibraryInfoWrapperPass>().getTLI();

  // Traverse all instructions, collect loads/stores/returns, check for calls.
  for (auto &BB : F) {
    for (auto &Inst : BB) {
      if (isa<LoadInst>(Inst) || isa<StoreInst>(Inst) ||
          isa<AtomicCmpXchgInst>(Inst) || isa<AtomicRMWInst>(Inst)) {
        MemoryLocation MLoc = MemoryLocation::get(&Inst);

        // Swift errors are special (we can't introduce extra uses on them).
        if (MLoc.Ptr->isSwiftError())
          continue;

        // Skip non-address-space-0 pointers; we don't know how to handle them.
        Type *PtrTy = cast<PointerType>(MLoc.Ptr->getType());
        if (PtrTy->getPointerAddressSpace() != 0)
          continue;

        if (MLoc.AATags.TBAA)
          TBAAMetadata.insert(MLoc.AATags.TBAA);
        MemoryAccesses.push_back(std::make_pair(&Inst, MLoc));
      } else if (isa<CallInst>(Inst) || isa<InvokeInst>(Inst)) {
        if (CallInst *CI = dyn_cast<CallInst>(&Inst))
          maybeMarkSanitizerLibraryCallNoBuiltin(CI, TLI);

        if (isa<MemIntrinsic>(Inst)) {
          MemTypeResetInsts.push_back(&Inst);
        } else if (auto *II = dyn_cast<IntrinsicInst>(&Inst)) {
          if (II->getIntrinsicID() == Intrinsic::lifetime_start ||
              II->getIntrinsicID() == Intrinsic::lifetime_end)
            MemTypeResetInsts.push_back(&Inst);
        }
      } else if (isa<AllocaInst>(Inst)) {
        MemTypeResetInsts.push_back(&Inst);
      }
    }
  }

  // byval arguments also need their types reset (they're new stack memory,
  // just like allocas).
  for (auto &A : F.args())
    if (A.hasByValAttr())
      MemTypeResetInsts.push_back(&A);

  // We have collected all loads and stores, and know for what TBAA nodes we
  // need to generate type descriptors.

  Module &M = *F.getParent();
  TypeDescriptorsMapTy TypeDescriptors;
  TypeNameMapTy TypeNames;
  for (const MDNode *MD : TBAAMetadata) {
    if (TypeDescriptors.count(MD))
      continue;

    if (!generateTypeDescriptor(MD, TypeDescriptors, TypeNames, M))
      return Res; // Giving up.

    Res = true;
  }

  Value *ShadowBase = nullptr, *AppMemMask = nullptr;
  for (auto &MA : MemoryAccesses)
    Res |= instrumentMemoryAccess(MA.first, MA.second, ShadowBase, AppMemMask,
                                 SanitizeFunction, TypeDescriptors, DL);

  for (auto Inst : MemTypeResetInsts)
    Res |= instrumentMemInst(Inst, ShadowBase, AppMemMask, DL);

  return Res;
}

bool TBAASanitizer::instrumentMemoryAccess(Instruction *I,
                                          MemoryLocation &MLoc,
                                          Value *&ShadowBase,
                                          Value *&AppMemMask,
                                          bool SanitizeFunction,
                                          TypeDescriptorsMapTy &TypeDescriptors,
                                          const DataLayout &DL) {
  if (!ShadowBase)
    ShadowBase = getShadowBase(*I->getParent()->getParent());
  if (!AppMemMask)
    AppMemMask = getAppMemMask(*I->getParent()->getParent());

  IRBuilder<> IRB(I);

  Constant *TDGV;
  if (MLoc.AATags.TBAA)
    TDGV = TypeDescriptors[MLoc.AATags.TBAA];
  else
    TDGV = Constant::getNullValue(IRB.getInt8PtrTy());

  Value *TD = IRB.CreateBitCast(TDGV, IRB.getInt8PtrTy());

  Value *ShadowDataInt =
    IRB.CreateAdd(IRB.CreateShl(IRB.CreateAnd(IRB.CreatePtrToInt(
                                                 const_cast<Value *>(MLoc.Ptr),
                                                 IntptrTy),
                                               AppMemMask),
                                 PtrShift),
                  ShadowBase);

  Type *Int8PtrPtrTy = IRB.getInt8PtrTy()->getPointerTo();
  Value *ShadowData = IRB.CreateIntToPtr(ShadowDataInt, Int8PtrPtrTy);

  Type *AccessTy = cast<PointerType>(MLoc.Ptr->getType())->getElementType();
  assert(AccessTy->isSized());
  uint64_t AccessSize = DL.getTypeStoreSize(AccessTy);

  // This is the TD value, -1, which is used to indicate that the byte is not
  // the first byte of the type.
  Value *BadTD = IRB.CreateIntToPtr(ConstantInt::getSigned(IntptrTy, -1),
                                    IRB.getInt8PtrTy());

  auto SetType = [&]() {
    IRB.CreateStore(TD, ShadowData);

    // Now fill the remainder of the shadow memory corresponding to the
    // remainder of the the bytes of the type with a bad type descriptor.
    for (uint64_t i = 1; i < AccessSize; ++i) {
      Value *BadShadowData =
        IRB.CreateIntToPtr(IRB.CreateAdd(ShadowDataInt,
                                         ConstantInt::get(IntptrTy,
                                                          i << PtrShift)),
                           Int8PtrPtrTy);
      IRB.CreateStore(BadTD, BadShadowData);
    }
  };

  Constant *Flags =
    ConstantInt::get(OrdTy, (int) I->mayReadFromMemory() |
                            (((int) I->mayWriteToMemory()) << 1));

  if (!ClWritesAlwaysSetType || I->mayReadFromMemory()) {
    // We need to check the type here. If the type is unknown, then the read
    // sets the type. If the type is known, then it is checked. If the type
    // doesn't match, then we call the runtime (which may yet determine that
    // the mismatch is okay).
    LLVMContext &C = I->getContext();
    MDNode *UnlikelyBW = MDBuilder(C).createBranchWeights(1, 100000);

    Value *LoadedTD = IRB.CreateLoad(ShadowData);
    if (SanitizeFunction) {
      Value *BadTDCmp = IRB.CreateICmpNE(LoadedTD, TD);
      TerminatorInst *BadTDTerm, *GoodTDTerm;
      SplitBlockAndInsertIfThenElse(BadTDCmp, &*IRB.GetInsertPoint(),
                                    &BadTDTerm, &GoodTDTerm, UnlikelyBW);
      IRB.SetInsertPoint(BadTDTerm);

      // We now know that the types did not match (we're on the slow path). If
      // the type is unknown, then set it.
      Value *NullTDCmp = IRB.CreateIsNull(LoadedTD);
      TerminatorInst *NullTDTerm, *MismatchTerm;
      SplitBlockAndInsertIfThenElse(NullTDCmp, &*IRB.GetInsertPoint(),
                                    &NullTDTerm, &MismatchTerm);


      // If the type is unknown, then set the type.
      IRB.SetInsertPoint(NullTDTerm);

      // We're about to set the type. Make sure that all bytes in the value are
      // also of unknown type.
      Value *Size = ConstantInt::get(OrdTy, AccessSize);
      Value *NotAllUnkTD = IRB.getFalse();
      for (uint64_t i = 1; i < AccessSize; ++i) {
        Value *UnkShadowData =
          IRB.CreateIntToPtr(IRB.CreateAdd(ShadowDataInt,
                                           ConstantInt::get(IntptrTy,
                                                            i << PtrShift)),
                             Int8PtrPtrTy);
        Value *ILdTD = IRB.CreateLoad(UnkShadowData);
        NotAllUnkTD = IRB.CreateOr(NotAllUnkTD, IRB.CreateIsNotNull(ILdTD));
      }

      Instruction *BeforeSetType = &*IRB.GetInsertPoint();
      TerminatorInst *BadUTDTerm =
        SplitBlockAndInsertIfThen(NotAllUnkTD, BeforeSetType, false,
                                  UnlikelyBW);
      IRB.SetInsertPoint(BadUTDTerm);
      IRB.CreateCall(TBAAsanCheck,
        {IRB.CreateBitCast(const_cast<Value *>(MLoc.Ptr), IRB.getInt8PtrTy()),
         Size, (Value *) TD, (Value *) Flags});

      IRB.SetInsertPoint(BeforeSetType);
      SetType();

      // We have a non-trivial mismatch. Call the runtime.
      IRB.SetInsertPoint(MismatchTerm);
      IRB.CreateCall(TBAAsanCheck,
        {IRB.CreateBitCast(const_cast<Value *>(MLoc.Ptr), IRB.getInt8PtrTy()),
         Size, (Value *) TD, (Value *) Flags});

      // We appear to have the right type. Make sure that all other bytes in
      // the type are still marked as interior bytes. If not, call the runtime.
      IRB.SetInsertPoint(GoodTDTerm);
      Value *NotAllBadTD = IRB.getFalse();
      for (uint64_t i = 1; i < AccessSize; ++i) {
        Value *BadShadowData =
          IRB.CreateIntToPtr(IRB.CreateAdd(ShadowDataInt,
                                           ConstantInt::get(IntptrTy,
                                                            i << PtrShift)),
                             Int8PtrPtrTy);
        Value *ILdTD = IRB.CreateLoad(BadShadowData);
        NotAllBadTD = IRB.CreateOr(NotAllBadTD, IRB.CreateICmpNE(ILdTD, BadTD));
      }

      TerminatorInst *BadITDTerm =
        SplitBlockAndInsertIfThen(NotAllBadTD, &*IRB.GetInsertPoint(),
                                  false, UnlikelyBW);
      IRB.SetInsertPoint(BadITDTerm);
      IRB.CreateCall(TBAAsanCheck,
        {IRB.CreateBitCast(const_cast<Value *>(MLoc.Ptr), IRB.getInt8PtrTy()),
         Size, (Value *) TD, (Value *) Flags});
    } else {
      // If we're not sanitizing this function, then we only care whether we
      // need to *set* the type.
      Value *NullTDCmp = IRB.CreateIsNull(LoadedTD);
      TerminatorInst *NullTDTerm =
        SplitBlockAndInsertIfThen(NullTDCmp, &*IRB.GetInsertPoint(), false,
                                  UnlikelyBW);
      IRB.SetInsertPoint(NullTDTerm);
      SetType();
    }
  } else if (I->mayWriteToMemory()) {
    // In the mode where writes always set the type, for a write (which does
    // not also read), we just set the type.
    SetType();
  }

  ++NumInstrumentedAccesses;
  return true;
}

// Memory-related intrinsics/instructions reset the type of the destination
// memory (including allocas and byval arguments).
bool TBAASanitizer::instrumentMemInst(Value *V, Value *&ShadowBase,
                                      Value *&AppMemMask, const DataLayout &DL) {
  BasicBlock::iterator IP;
  BasicBlock *BB;
  Function *F;

  if (auto *I = dyn_cast<Instruction>(V)) {
    IP = BasicBlock::iterator(I);
    BB = I->getParent();
    F = BB->getParent();
  } else {
    auto *A = cast<Argument>(V);
    F = A->getParent();
    BB = &F->getEntryBlock();
    IP = BB->getFirstInsertionPt();
  }

  Value *Dest, *Size;
  IRBuilder<> IRB(BB, IP);

  if (auto *A = dyn_cast<Argument>(V)) {
    assert(A->hasByValAttr() && "Type reset for non-byval argument?");

    Dest = A;
    Size =
      ConstantInt::get(IntptrTy,
                       DL.getTypeAllocSize(cast<PointerType>(A->getType())->
                                             getElementType()));
  } else {
    auto *I = cast<Instruction>(V);
    if (MemIntrinsic *MI = dyn_cast<MemIntrinsic>(I)) {
      if (MI->getDestAddressSpace() != 0)
        return false;

      Dest = MI->getDest();
      Size = MI->getLength();
    } else if (IntrinsicInst *II = dyn_cast<IntrinsicInst>(I)) {
      if (II->getIntrinsicID() != Intrinsic::lifetime_start &&
          II->getIntrinsicID() != Intrinsic::lifetime_end)
        return false;

      Size = II->getArgOperand(0);
      Dest = II->getArgOperand(1);
    } else if (auto *AI = dyn_cast<AllocaInst>(I)) {
      // We need to clear the types for new stack allocations (or else we might
      // read stale type information from a previous function execution).

      IRB.SetInsertPoint(&*std::next(BasicBlock::iterator(I)));
      IRB.SetInstDebugLocation(I);

      Size = IRB.CreateMul(IRB.CreateZExtOrTrunc(AI->getArraySize(), IntptrTy),
                           ConstantInt::get(IntptrTy,
                                            DL.getTypeAllocSize(
                                              AI->getAllocatedType())));
      Dest = I;
    } else {
      return false;
    }
  }

  if (!ShadowBase)
    ShadowBase = getShadowBase(*F);
  if (!AppMemMask)
    AppMemMask = getAppMemMask(*F);

  Value *ShadowDataInt =
    IRB.CreateAdd(IRB.CreateShl(IRB.CreateAnd(IRB.CreatePtrToInt(Dest,
                                                                  IntptrTy),
                                               AppMemMask),
                                 PtrShift),
                  ShadowBase);
  Value *ShadowData = IRB.CreateIntToPtr(ShadowDataInt, IRB.getInt8PtrTy());

  IRB.CreateMemSet(ShadowData, IRB.getInt8(0),
                   IRB.CreateShl(Size, PtrShift), 1u << PtrShift);

  return true;
}

