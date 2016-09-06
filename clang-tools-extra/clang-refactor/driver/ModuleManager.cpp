//===--- ModuleManager.cpp - clang-refactor ---------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "ModuleManager.h"

namespace clang {
namespace refactor {

void ModuleManager::AddModule(StringRef Command,
                              std::unique_ptr<RefactoringModule> Module) {
  RegisteredModules.push_back(std::move(Module));
  CommandToModuleID[Command] = RegisteredModules.size() - 1;
}

int ModuleManager::Dispatch(StringRef Command, int argc, const char **argv) {
  if (CommandToModuleID.find(Command) != CommandToModuleID.end()) {
    return RegisteredModules[CommandToModuleID[Command]]->run(argc, argv);
  }
  return 1;
}

} // end namespace refactor
} // end namespace clang
