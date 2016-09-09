//===--- Driver.cpp - clang-refactor ----------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
///  \file This file implements a clang-refactor tool.
///
////===----------------------------------------------------------------------===//

#include "clang/Tooling/CommonOptionsParser.h"
#include "llvm/Support/raw_ostream.h"

#include "ModuleManager.h"

namespace clang {
namespace refactor {

static int printHelpMessage(ModuleManager &Manager) {
  llvm::outs() << Manager.getTopLevelHelp();
  return 1;
}

int clangRefactorMain(int argc, const char **argv) {
  ModuleManager Manager;
  if (argc > 1) {
    llvm::StringRef FirstArgument(argv[1]);
    if (FirstArgument == "-h" || FirstArgument == "-help" ||
        FirstArgument == "--help")
      return printHelpMessage(Manager);
    const std::string Command = argv[1];
    argv[1] = (llvm::Twine(argv[0]) + " " + llvm::Twine(argv[1])).str().c_str();
    Manager.Dispatch(Command, argc - 1, argv + 1);
  } else {
    return printHelpMessage(Manager);
  }
  return 0;
}

} // end namespace refactor
} // end namespace clang

int main(int argc, const char **argv) {
  return clang::refactor::clangRefactorMain(argc, argv);
}
