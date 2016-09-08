//===--- RefactoringModule.h - clang-refactor -------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_REFACTOR_REFACTORING_MODULE_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_REFACTOR_REFACTORING_MODULE_H

#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Refactoring.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Signals.h"

#include <string>

namespace clang {
namespace refactor {

class RefactoringModule {
public:
  RefactoringModule(const std::string &ModuleName,
                    const std::string &ModuleMetaDescription)
      : ModuleName(ModuleName), ModuleMetaDescription(ModuleMetaDescription) {}

  // Refactoring consists of 3.5 stages:
  //
  // 0. Check arguments for validity.
  //
  // 1. Extract information needed for the refactoring upon tool invocation.
  //
  // 2. Handle translation unit and identify whether the refactoring can be
  // applied. If it can, store the replacements. Panic otherwise.
  //
  // 3. Merge duplicate replacements, panic if there are conflicting ones.
  // Process translation unit and apply refactoring.
  //
  // Few examples of how each of these stages would look like for future
  // modules "rename" and "inline".
  //
  // Rename
  // ======
  //
  // 0. Check arguments for validity.
  //
  // 1. Rename operates with USRs, so the first stage would be finding the
  // symbol, which will be renamed and storing its USR.
  //
  // 2. Check whether renaming will introduce any name conflicts. If it won't
  // find each occurrence of the symbol in translation unit using USR and store
  // replacements.
  //
  // 3. Apply replacements or output diagnostics. Handle duplicates and panic if
  // there are conflicts.
  //
  // Inline
  // ======
  //
  // 0. Check arguments for validity.
  //
  // 1. Find function, identify what the possible issues might be.
  //
  // 2. Check whether inlining will introduce any issues, e.g. there is a
  // pointer passed somewhere to the inlined function and after inlining it the
  // code will no longer compile. If it won't find function calls, add needed
  // temporary variables and replace the call with function body.
  //
  // 3. Apply replacements. Handle duplicates and panic if there are conflicts.
  //
  // Summary
  // =======
  //
  // As one can easily see, step 1 should be performed upon module invocation
  // and it needs to process translation unit, from which the passed <source0>
  // comes from.
  //
  // With appropriate facilities step 2 can be parallelized to process multiple
  // translation units of the project independently. If none of them have any
  // issues with applying this refactoring replacements are stored and queued
  // for later.
  //
  // Step 3 can be parallelized even more easily. It basically consists of text
  // replacements.
  //
  int run(int argc, const char **argv) {
    // Register options.
    llvm::cl::OptionCategory RefactoringModuleOptions(ModuleName.c_str(),
                                                      ModuleMetaDescription.c_str());
    registerCustomOptions(RefactoringModuleOptions);
    registerCustomOptions(RefactoringModuleOptions);
    // Parse options and get compilations.
    llvm::sys::PrintStackTraceOnErrorSignal(argv[0]);
    tooling::CommonOptionsParser OptionsParser(argc, argv,
                                               RefactoringModuleOptions);
    tooling::RefactoringTool Tool(OptionsParser.getCompilations(),
                                  OptionsParser.getSourcePathList());

    // Actual routine.
    checkArguments();
    extractRefactoringInformation();
    handleTranslationUnit();
    applyReplacementsOrOutputDiagnostics();
    return 0;
  }

  // Routine for registering common modules options.
  void registerCommonOptions(llvm::cl::OptionCategory &Category) {
  }

  // A way for each tool to provide additional options. Overriding this one is
  // optional.
  virtual void registerCustomOptions(llvm::cl::OptionCategory &Category) {}

  // Step 0.
  // Just checking arguments.
  //
  // Panic: if they are not valid.
  virtual int checkArguments() = 0;

  // Step 1.
  //
  // Process translation <source0> and figure out what should be done.
  //
  // Panic: if refactoring can not be applied. E.g. unsupported cases like
  // renaming macros etc.
  virtual int extractRefactoringInformation() = 0;

  // Step 2.
  //
  // Find places where refactoring should be applied and store replacements for
  // the future.
  //
  // Panic: if there are any issues with applying refactorings to the
  // translation unit.
  virtual int handleTranslationUnit() = 0;

  // Step 3.
  //
  // Handle duplicates.
  //
  // Panic: if there are conflicting replacements.
  virtual int applyReplacementsOrOutputDiagnostics() = 0;

  virtual ~RefactoringModule() = default;

  StringRef getModuleName() { return ModuleName; }

  StringRef getModuleMetaDescription() { return ModuleMetaDescription; }

private:
  // ModuleName is the lowercase submodule name, which will be passed to
  // clang-refactor to invoke the submodule tool.
  const std::string ModuleName;

  // ModuleMetaDescription will appear as the module description upon calling
  // $ clang-refactor --help
  const std::string ModuleMetaDescription;
};

} // end namespace refactor
} // end namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_REFACTOR_REFACTORING_MODULE_H
