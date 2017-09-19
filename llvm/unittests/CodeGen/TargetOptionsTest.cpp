#include "llvm/ADT/STLExtras.h"
#include "llvm/CodeGen/LiveIntervalAnalysis.h"
#include "llvm/CodeGen/MIRParser/MIRParser.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Target/TargetRegisterInfo.h"
#include "gtest/gtest.h"

using namespace llvm;

namespace llvm {
  void initializeTestPassPass(PassRegistry &);
}

namespace {

void initLLVM() {
  InitializeAllTargets();
  InitializeAllTargetMCs();
  InitializeAllAsmPrinters();
  InitializeAllAsmParsers();

  PassRegistry *Registry = PassRegistry::getPassRegistry();
  initializeCore(*Registry);
  initializeCodeGen(*Registry);
}

/// Create a TargetMachine. We need a target that doesn't have IPRA enabled by
/// default. That turns out to be all targets at the moment, so just use X86.
std::unique_ptr<TargetMachine> createTargetMachine(bool EnableIPRA) {
  Triple TargetTriple("x86_64--");
  std::string Error;
  const Target *T = TargetRegistry::lookupTarget("", TargetTriple, Error);
  if (!T)
    return nullptr;

  TargetOptions Options;
  Options.EnableIPRA = EnableIPRA;
  return std::unique_ptr<TargetMachine>(T->createTargetMachine(
      "X86", "", "", Options, None, None, CodeGenOpt::Aggressive));
}

typedef std::function<void(bool)> TargetOptionsTest;

static void targetOptionsTest(bool EnableIPRA) {
  LLVMContext Context;
  std::unique_ptr<TargetMachine> TM = createTargetMachine(EnableIPRA);
  // This test is designed for the X86 backend; stop if it is not available.
  if (!TM)
    return;
  legacy::PassManager PM;
  LLVMTargetMachine &LLVMTM = static_cast<LLVMTargetMachine &>(*TM);

  TargetPassConfig &TPC = *LLVMTM.createPassConfig(PM);

  ASSERT_TRUE(TM->Options.EnableIPRA == EnableIPRA);
}

} // End of anonymous namespace.

TEST(TargetOptionsTest, IPRASetToOff) {
  targetOptionsTest(false);
}

TEST(TargetOptionsTest, IPRASetToOn) {
  targetOptionsTest(true);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  initLLVM();
  return RUN_ALL_TESTS();
}
