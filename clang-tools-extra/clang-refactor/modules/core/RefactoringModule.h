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
#include "clang/Tooling/CompilationDatabase.h"
#include "clang/Tooling/Refactoring.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ErrorOr.h"
#include "llvm/Support/MemoryBuffer.h"
#include "clang/Basic/Diagnostic.h"
#include "llvm/Support/Signals.h"
#include <string>

namespace clang {
namespace refactor {

class RefactoringModule {
public:
  RefactoringModule(const std::string &ModuleName,
                    const std::string &ModuleMetaDescription)
      : ModuleName(ModuleName), ModuleMetaDescription(ModuleMetaDescription) {}

  //===--------------------------------------------------------------------===//
  // Refactoring cosists of 3 stages.
  //
  //===--------------------------------------------------------------------===//
  //
  // Stage I: Plan execution.
  //
  // In this stage the submodule parses arguments, determines whether the
  // refactoring invokation is correct and stores information neeeded to perform
  // that refactoring in each affected translation unit.
  //
  // Input: Command line (?) arguments.
  //
  // Ouput: List of affected translation units, in which the refactoring would
  // happen, and submodule-specific information needed for the execution.
  //
  // Has access to: AST <source0> belongs to, cross-reference index.
  //
  //===--------------------------------------------------------------------===//
  //
  // Stage II: Running on a single translation unit.
  //
  // Input: submodule-specific information needed for the execution from
  // Stage I.
  //
  // Output: list of replacements and diagnostics.
  //
  // Has access to: AST of given translation unit.
  //
  //===--------------------------------------------------------------------===//
  //
  // Stage III: Apply replacements and output diagnostics.
  //
  // This step is not submodule-specific. Therefore, it should be delegated to
  // the common interface.
  //
  // Input: Replacements and diagnostics from stage II.
  //
  // Output: Applied replacements and issued diagnostics.
  //
  //===--------------------------------------------------------------------===//

  int run(int argc, const char **argv);

  // Routine for registering common refactoring options.
  //
  // Common options reused by multiple tools like "-offset", "verbose" etc
  // should go here.
  void registerCommonOptions(llvm::cl::OptionCategory &Category);

  // A way for each tool to provide additional options. Overriding this one is
  // optional.
  virtual void registerCustomOptions(llvm::cl::OptionCategory &Category) = 0;

  //===--------------------------------------------------------------------===//
  // Stage I: Plan execution.

  // This processes given translation unit of <source0> and retrieves
  // information needed for the refactoring.
  //
  // Input: ASTContext of translation unit <source0> belongs to.
  //
  // Output: Serialized refactoring-specific information;
  virtual std::string getExecutionInformation() = 0;

  // Return translation units affected by certain refactoring. As a temporary
  // solution it can return all translation units, regardless of them being
  // affected by refactoring.
  //
  // Input: Serialized refactroing-specific information.
  //
  // Output: List of translation units.
  virtual std::vector<std::string> getAffectedTUs(tooling::CompilationDatabase,
                                                  std::string RefactoringInfo);
  //===--------------------------------------------------------------------===//

  //===--------------------------------------------------------------------===//
  // Stage II: Running on a single translation unit.
  virtual llvm::ErrorOr<
      std::pair<tooling::Replacements, std::vector<Diagnostic>>>
  handleTranslationUnit() = 0;
  //===--------------------------------------------------------------------===//

  //===--------------------------------------------------------------------===//
  // Stage III: Apply replacements and output diagnostics.
  llvm::Error
  applyReplacementsAndOutputDiagnostics(tooling::Replacements Replacements,
                                        std::vector<Diagnostic> Diags);
  //===--------------------------------------------------------------------===//

  StringRef getModuleName();
  StringRef getModuleMetaDescription();

  virtual ~RefactoringModule() = default;

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
