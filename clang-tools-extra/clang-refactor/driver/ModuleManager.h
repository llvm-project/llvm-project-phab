//===--- ModuleManager.h - clang-refactor -----------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_REFACTOR_MODULE_MANAGER_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_REFACTOR_MODULE_MANAGER_H

#include "core/RefactoringModule.h"

#include <string>
#include <vector>
#include <unordered_map>

namespace clang {
namespace clang_refactor {

class ModuleManager {
public:
  void AddModule(std::string Command, std::unique_ptr<RefactoringModule> Module);

  int Dispatch(std::string &Command, int argc, const char **argv);

private:
  std::vector<std::unique_ptr<RefactoringModule>> RegisteredModules;
  std::unordered_map<std::string, unsigned> CommandToModuleID;
};

} // end namespace clang_refactor
} // end namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_REFACTOR_MODULE_MANAGER_H
