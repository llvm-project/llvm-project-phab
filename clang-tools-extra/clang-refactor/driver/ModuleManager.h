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

#include "clang/Basic/LLVM.h"
#include "llvm/ADT/StringRef.h"

#include <string>
#include <unordered_map>
#include <vector>

#include "core/RefactoringModule.h"

using namespace llvm;

namespace clang {
namespace refactor {

class ModuleManager {
public:
  void AddModule(StringRef Command, std::unique_ptr<RefactoringModule> Module);

  int Dispatch(StringRef Command, int argc, const char **argv);

private:
  std::vector<std::unique_ptr<RefactoringModule>> RegisteredModules;
  std::unordered_map<std::string, unsigned> CommandToModuleID;
};

} // end namespace refactor
} // end namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_REFACTOR_MODULE_MANAGER_H
