//===---- tools/extra/clang-refactor/Driver.cpp - clang-refactor driver ---===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  \file This file implements clang-refactor driver.
//
//===----------------------------------------------------------------------===//

#include "Rename.h"

#include "llvm/ADT/StringSwitch.h"

#include <functional>
#include <string>

using namespace clang;
using namespace llvm;

const char HelpMessage[] =
    "USAGE: clang-refactor [subcommand] [options] <source0> [... <sourceN>]\n"
    "\n"
    "Subcommands:\n"
    "  rename: rename the symbol found at <offset>s or by <qualified-name>s in "
    "<source0>.\n";

static int printHelpMessage(int argc, const char *argv[]) {
  outs() << HelpMessage;
  return 0;
}

int main(int argc, const char **argv) {
  if (argc > 1) {
    using MainFunction = std::function<int(int, const char *[])>;
    MainFunction Func =
        StringSwitch<MainFunction>(argv[1])
            .Cases("-help", "--help", printHelpMessage)
            .Case("rename", clang_refactor::rename_module::RenameModuleMain)
            .Default(nullptr);
    if (Func) {
      std::string Invocation = std::string(argv[0]) + " " + argv[1];
      argv[1] = Invocation.c_str();
      return Func(argc - 1, argv + 1);
    }
  }

  printHelpMessage(argc, argv);
  return 0;
}
