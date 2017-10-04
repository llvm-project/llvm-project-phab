//===-- WasmEHPrepare - Prepare excepton handling for WebAssembly --------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This transformation is designed for use by code generators which use
// WebAssembly exception handling scheme.
//
// * WebAssembly EH instructions
// WebAssembly's try and catch instructions are structured as follows:
// try
//   instruction*
// catch (C++ tag)
//   instruction*
// ...
// catch_all
//   instruction*
// try_end
//
// A catch instruction in WebAssembly does not correspond to a C++ catch clause.
// In WebAssembly, there is a single catch instruction for all C++ exceptions.
// There can be more catch instructions for exceptions in other languages, but
// they are not generated for now. catch_all catches all exceptions including
// foreign ones, but this is not genereated for the current version of
// implementation yet as well. In this pass we generate a call to wasm.catch
// intrinsic, which is going to be lowered to a WebAssembly catch instruction at
// instruction selection phase.
//
// * libunwind
// In WebAssembly EH, the VM is responsible for unwinding stack once an
// exception is thrown. After stack is unwound, the control flow is transfered
// to WebAssembly 'catch' instruction, which returns a caught exception object.
//
// Unwinding stack is not done by libunwind but the VM, so the personality
// function in libcxxabi cannot be called from libunwind during the unwinding
// process. So after returned from a catch instruction, which means stack was
// unwound by the VM, we actively call a wrapper function in libunwind that in
// turn calls the real personality function.
//
// In Itanium EH, if the personality function decides there is no matching catch
// cluase in a call frame and there is no cleanup action to perform, the
// unwinder doesn't stop there and continues unwinding. But in Wasm EH, the
// unwinder stops at every call frame with a landing pad, after which the
// personality function is called actively from the code.
//
// In libunwind/include/Unwind-wasm.c, we have this struct that serves as a
// communincation channel between user code and the personality function in
// libcxxabi.
//
// struct _Unwind_LandingPadContext {
//   uintptr_t lpad_index;
//   __personality_routine personality;
//   uintptr_t lsda;
//   uintptr_t selector;
// };
// struct _Unwind_LandingPadContext __wasm_lpad_context = ...;
//
// And this wrapper in libunwind calls the personality function.
//
// _Unwind_Reason_Code _Unwind_CallPersonality(void *exception_ptr) {
//   struct _Unwind_Exception *exception_obj =
//       (struct _Unwind_Exception *)exception_ptr;
//   _Unwind_Reason_Code ret = (*__wasm_lpad_context->personality)(
//       1, _UA_CLEANUP_PHASE, exception_obj->exception_class, exception_obj,
//       (struct _Unwind_Context *)__wasm_lpad_context);
//   return ret;
// }
//
// * This pass does the following things:
//
// 1. Insert a call to wasm.eh.landingpad.index() before every invoke
//    instruction. Each landing pad has its own index starting from 0, and the
//    argument to wasm.eh.landingpad.index() is the index of the landing pad
//    each invoke instruction unwinds to.
//    e.g.
//    call void wasm.eh.landingpad.index(index);
//    invoke void @foo() ... // original invoke inst
//
//    wasm.eh.landingpad.index() function is lowered later to pass information
//    to LSDA generator.
//
// 2. Adds following code after every landingpad instruction:
//    (In C-style pseudocode)
//
//    {old_exn, old_selector} = landingpad .. // original landingpad inst
//    void *exn = wasm.catch(0);
//    __wasm_lpad_context.lpad_index = index;
//    __wasm_lpad_context.personality = __gxx_personality_v0;
//    __wasm_lpad_context.lsda = wasm.eh.lsda();
//    _Unwind_CallPersonality(exn);
//    int selector = __wasm.landingpad_context.selector;
//    // use {exn, selector} instead hereafter
//
//    wasm.catch() function returns a caught exception object. We pass a landing
//    pad index, personality function, and the address of LSDA for the current
//    function to the wrapper function _Unwind_CallPersonality in libunwind,
//    which calls the passed personality function. And we retrieve the selector
//    after it returns. The landingpad instruction will be removed at
//    instruction selection phase, and {exn, selector} will replace its return
//    value.
//
//===----------------------------------------------------------------------===//

#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/Triple.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/Pass.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
using namespace llvm;

#define DEBUG_TYPE "wasmehprepare"

namespace {
class WasmEHPrepare : public FunctionPass {
  Type *LPadContextTy = nullptr; // type of 'struct _Unwind_LandingPadContext'
  GlobalVariable *LPadContextGV = nullptr; // __wasm_lpad_context

  // Field addresses of struct _Unwind_LandingPadContext
  Value *LPadIndexField = nullptr;   // lpad_index field
  Value *PersonalityField = nullptr; // personality field
  Value *LSDAField = nullptr;        // lsda field
  Value *SelectorField = nullptr;    // selector

  Function *CatchF = nullptr;           // wasm.catch() intrinsic
  Function *LPadIndexF = nullptr;       // wasm.eh.landingpad.index() intrinsic
  Function *LSDAF = nullptr;            // wasm.eh.lsda() intrinsic
  Function *CallPersonalityF = nullptr; // _Unwind_CallPersonality() wrapper

  void prepareLandingPad(BasicBlock *BB, unsigned Index, bool IsTopLevelLPad);
  void substituteLPadValues(LandingPadInst *LPI, Value *Exn, Value *Selector);

public:
  static char ID; // Pass identification, replacement for typeid

  WasmEHPrepare() : FunctionPass(ID) {}

  bool doInitialization(Module &M) override;
  bool runOnFunction(Function &F) override;

  StringRef getPassName() const override {
    return "WebAssembly Exception handling preparation";
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<DominatorTreeWrapperPass>();
  }
};
} // end anonymous namespace

char WasmEHPrepare::ID = 0;
INITIALIZE_PASS(WasmEHPrepare, DEBUG_TYPE, "Prepare WebAssembly exceptions",
                false, false);

FunctionPass *llvm::createWasmEHPreparePass() { return new WasmEHPrepare(); }

bool WasmEHPrepare::doInitialization(Module &M) {
  IRBuilder<> IRB(M.getContext());
  LPadContextTy = StructType::get(IRB.getInt32Ty(),   // lpad_index
                                  IRB.getInt8PtrTy(), // personality
                                  IRB.getInt8PtrTy(), // lsda
                                  IRB.getInt32Ty()    // selector
  );
  return false;
}

bool WasmEHPrepare::runOnFunction(Function &F) {
  SmallVector<BasicBlock *, 16> LPads;
  for (BasicBlock &BB : F) {
    if (BB.isEHPad())
      LPads.push_back(&BB);
  }

  if (LPads.empty())
    return false;

  assert(F.hasPersonalityFn() && "Personality function not found");

  Module &M = *F.getParent();
  IRBuilder<> IRB(F.getContext());

  // wasm.catch() intinsic. Will be lowered to WebAssmebly 'catch' instruction.
  CatchF = Intrinsic::getDeclaration(&M, Intrinsic::wasm_catch);
  // wasm.eh.landingpad.index() intrinsic, which is to specify landingpad index
  LPadIndexF =
      Intrinsic::getDeclaration(&M, Intrinsic::wasm_eh_landingpad_index);
  // wasm.eh.lsda() intrinsic. Returns the address of LSDA table for the current
  // function.
  LSDAF = Intrinsic::getDeclaration(&M, Intrinsic::wasm_eh_lsda);
  // _Unwind_CallPersonality() wrapper function, which calls the personality
  CallPersonalityF = cast<Function>(M.getOrInsertFunction(
      "_Unwind_Wasm_CallPersonality", IRB.getInt32Ty(), IRB.getInt8PtrTy()));

  LPadContextGV = cast<GlobalVariable>(
      M.getOrInsertGlobal("__wasm_lpad_context", LPadContextTy));
  LPadIndexField = IRB.CreateConstGEP2_32(LPadContextTy, LPadContextGV, 0, 0,
                                          "lpad_index_gep");
  PersonalityField =
      IRB.CreateConstGEP2_32(LPadContextTy, LPadContextGV, 0, 1, "pers_fn_gep");
  LSDAField =
      IRB.CreateConstGEP2_32(LPadContextTy, LPadContextGV, 0, 2, "lsda_gep");
  SelectorField = IRB.CreateConstGEP2_32(LPadContextTy, LPadContextGV, 0, 3,
                                         "selector_gep");

  // Calculate top-level landing pads: landing pads that do not dominated by any
  // of other landing pads.
  auto &DT = getAnalysis<DominatorTreeWrapperPass>().getDomTree();
  SmallPtrSet<BasicBlock *, 16> TopLevelLPads;
  DomTreeNode *Root = DT.getRootNode();
  SmallVector<DomTreeNode *, 8> WL;
  WL.push_back(Root);
  while (!WL.empty()) {
    DomTreeNode *N = WL.pop_back_val();
    if (N->getBlock()->isEHPad())
      TopLevelLPads.insert(N->getBlock());
    else
      WL.append(N->begin(), N->end());
  }

  for (unsigned I = 0, E = LPads.size(); I != E; ++I)
    prepareLandingPad(LPads[I], I, TopLevelLPads.count(LPads[I]) > 0);

  return true;
}

void WasmEHPrepare::prepareLandingPad(BasicBlock *BB, unsigned Index,
                                      bool IsTopLevelLPad) {
  assert(BB->isEHPad() && "BB is not a landing pad!");
  Function *F = BB->getParent();
  LandingPadInst *LPI = BB->getLandingPadInst();
  IRBuilder<> IRB(F->getContext());

  // Create a call to wasm.eh.landingpad.index intrinsic before every invoke
  // instruction that has this BB as its unwind destination. This is to create a
  // map of <landingpad BB, landingpad index> in SelectionDAG, which is to be
  // used to create a map of <landingpad EH label, landingpad index> in
  // SelectionDAGISel, again which is to be used in EHStreamer to emit LSDA
  // tables.
  for (BasicBlock *Pred : predecessors(BB)) {
    auto *II = dyn_cast<InvokeInst>(Pred->getTerminator());
    assert(II && "Landingpad's predecessor does not end with invoke");
    IRB.SetInsertPoint(II);
    IRB.CreateCall(LPadIndexF, IRB.getInt32(Index));
  }

  IRB.SetInsertPoint(&*BB->getFirstInsertionPt());
  // The argument to wasm.catch() is the tag for C++ exceptions, which we set to
  // 0 for this module.
  ConstantInt *CppTag = IRB.getInt32(0);
  // void *exn = wasm.catch(0);
  Instruction *Exn = IRB.CreateCall(CatchF, CppTag, "exn");

  // __wasm_lpad_context.lpad_index = index;
  IRB.CreateStore(IRB.getInt32(Index), LPadIndexField, true);

  // If this is not a top level landing pad, i.e., there is another landing pad
  // that dominates this landing pad, we don't need to store personality
  // function address and LSDA address again, because they are the same
  // throughout the function and have been already stored before.
  if (IsTopLevelLPad) {
    // __wasm_lpad_context.personality = __gxx_personality_v0;
    IRB.CreateStore(
        IRB.CreateBitCast(F->getPersonalityFn(), IRB.getInt8PtrTy()),
        PersonalityField, true);
    // __wasm_lpad_context.lsda = wasm.eh.lsda();
    IRB.CreateStore(IRB.CreateCall(LSDAF), LSDAField, true);
  }
  // _Unwind_CallPersonality(exn);
  IRB.CreateCall(CallPersonalityF, Exn);
  // int selector = __wasm.landingpad_context.selector;
  Instruction *Selector = IRB.CreateLoad(SelectorField, "selector");
  substituteLPadValues(LPI, Exn, Selector);
}

void WasmEHPrepare::substituteLPadValues(LandingPadInst *LPI, Value *Exn,
                                         Value *Selector) {
  SmallVector<Value *, 8> UseWorkList(LPI->user_begin(), LPI->user_end());
  while (!UseWorkList.empty()) {
    Value *V = UseWorkList.pop_back_val();
    auto *EVI = dyn_cast<ExtractValueInst>(V);
    if (!EVI)
      continue;
    if (EVI->getNumIndices() != 1)
      continue;
    if (*EVI->idx_begin() == 0)
      EVI->replaceAllUsesWith(Exn);
    else if (*EVI->idx_begin() == 1)
      EVI->replaceAllUsesWith(Selector);
    if (EVI->use_empty())
      EVI->eraseFromParent();
  }

  if (LPI->use_empty())
    return;

  // There are still some uses of LPI. Construct an aggregate with the exception
  // values and replace the LPI with that aggregate.
  Type *LPadType = LPI->getType();
  Value *LPadVal = UndefValue::get(LPadType);
  auto *SelI = cast<Instruction>(Selector);
  IRBuilder<> Builder(SelI->getParent(), std::next(SelI->getIterator()));
  LPadVal = Builder.CreateInsertValue(LPadVal, Exn, 0, "lpad.val");
  LPadVal = Builder.CreateInsertValue(LPadVal, Selector, 1, "lpad.val");
  LPI->replaceAllUsesWith(LPadVal);
}
