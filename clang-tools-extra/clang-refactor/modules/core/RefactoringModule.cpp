//===--- RefactoringModule.cpp - clang-refactor -----------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "RefactoringModule.h"

namespace clang {
namespace refactor {

int RefactoringModule::run(int argc, const char **argv) {
  return 0;
}

void RefactoringModule::registerCommonOptions(
    llvm::cl::OptionCategory &Category) {}

std::vector<std::string>
RefactoringModule::getAffectedTUs(tooling::CompilationDatabase DB,
                                  std::string RefactoringInfo) {
  std::vector<std::string> TranslationUnits;
  // FIXME: Don't return all files; return one file per translation unit.
  TranslationUnits = DB.getAllFiles();
  return TranslationUnits;
}

StringRef RefactoringModule::getModuleName() { return ModuleName; }

StringRef RefactoringModule::getModuleMetaDescription() {
  return ModuleMetaDescription;
}

llvm::Error RefactoringModule::applyReplacementsAndOutputDiagnostics(
    tooling::Replacements Replacements,
    std::vector<Diagnostic> Diags) {
  llvm::Error Err;
  // TODO: Issue diagnostics.
  // TODO: Apply replacements.
  return Err;
}

} // end namespace refactor
} // end namespace clang
