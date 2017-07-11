#include "llvm/Support/Plugin.h"
#include "llvm/Config/config.h"
#include "llvm/Passes/PassBuilder.h"

#include "TestPlugin.h"

using namespace llvm;

struct TestModulePass : public PassInfoMixin<TestModulePass> {
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &MAM) {
    return PreservedAnalyses::all();
  }
};

void registerCallbacks(PassBuilder &PB) {
  PB.registerPipelineParsingCallback(
      [](StringRef Name, ModulePassManager &PM,
         ArrayRef<PassBuilder::PipelineElement> InnerPipeline) {
        if (Name == "plugin-pass") {
          PM.addPass(TestModulePass());
          return true;
        }
        return false;
      });
}

LLVM_PLUGIN(TEST_PLUGIN_NAME, TEST_PLUGIN_VERSION, registerCallbacks)
