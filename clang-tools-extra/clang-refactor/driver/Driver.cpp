//===--- Driver.cpp - clang-refactor ----------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
///  \file This file implements a clang-tidy tool.
///
///  This tool uses the Clang Tooling infrastructure, see
///    http://clang.llvm.org/docs/HowToSetupToolingForLLVM.html
///  for details on setting it up with LLVM source tree.
///
//===----------------------------------------------------------------------===//

#include "clang/Tooling/CommonOptionsParser.h"
#include "llvm/Support/raw_ostream.h"

#include "ModuleManager.h"
#include "template/TemplateModule.h"

namespace clang {
namespace clang_refactor {

const char HelpHead[] = "USAGE: clang-refactor [subcommand] [options] "
                        "<source0> [... <sourceN>]\n"
                        "\n"
                        "Subcommands:\n";

static int printHelpMessage() {
  llvm::outs() << HelpHead;
  return 1;
}

int clanRefactorMain(int argc, const char **argv) {
  ModuleManager Manager;
  Manager.AddModule("template",
                    std::unique_ptr<template_module::TemplateModule>(
                        new template_module::TemplateModule(
                            "Template module", "Example template module")));
  std::string Command = "";
  if (argc > 1) {
    llvm::StringRef FirstArgument(argv[1]);
    if (FirstArgument == "-h" || FirstArgument == "-help" ||
        FirstArgument == "--help")
      return printHelpMessage();
    Command = argv[1];
    std::string Invocation = std::string(argv[0]) + " " + argv[1];
    argv[1] = Invocation.c_str();
    Manager.Dispatch(Command, argc - 1, argv + 1);
  } else {
    return printHelpMessage();
  }
  return 0;
}

} // end namespace clang_refactor
} // end namespace clang

int main(int argc, const char **argv) {
  return clang::clang_refactor::clanRefactorMain(argc, argv);
}
