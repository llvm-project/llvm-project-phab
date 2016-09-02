//===--- tools/extra/clang-refactor/ClangRefactorOptions.h ----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// \brief Defines common options for clang-refactor subtools.
///
//===----------------------------------------------------------------------===//

#include "clang/Tooling/CommonOptionsParser.h"

#ifndef CLANG_REFACTOR_OPTIONS_H
#define CLANG_REFACTOR_OPTIONS_H

using namespace llvm;

namespace clang {
namespace clang_refactor {

// FIXME: Adjust libTooling so that these options would be displayed, too.
static cl::OptionCategory
    ClangRefactorCommonOptions("clang-refactor common options");

static cl::list<unsigned> SymbolOffsets(
    "offset",
    cl::desc("Locates the symbol by offset as opposed to <line>:<column>."),
    cl::ZeroOrMore, cl::cat(ClangRefactorCommonOptions));
static cl::opt<bool> Inplace("i", cl::desc("Overwrite edited <file>s."),
                      cl::cat(ClangRefactorCommonOptions));
static cl::list<std::string>
    QualifiedNames("qualified-name",
                   cl::desc("The fully qualified name of the symbol."),
                   cl::ZeroOrMore, cl::cat(ClangRefactorCommonOptions));

namespace rename_module {

static cl::OptionCategory
    ClangRefactorRenameCategory("clang-refactor rename options");

static cl::list<std::string>
    NewNames("new-name", cl::desc("The new name to change the symbol to."),
             cl::ZeroOrMore, cl::cat(ClangRefactorRenameCategory));
static cl::opt<bool> PrintName(
    "pn",
    cl::desc("Print the found symbol's name prior to renaming to stderr."),
    cl::cat(ClangRefactorRenameCategory));
static cl::opt<bool> PrintLocations(
    "pl", cl::desc("Print the locations affected by renaming to stderr."),
    cl::cat(ClangRefactorRenameCategory));
static cl::opt<std::string> ExportFixes(
    "export-fixes", cl::desc("YAML file to store suggested fixes in."),
    cl::value_desc("filename"), cl::cat(ClangRefactorRenameCategory));
static cl::opt<std::string>
    Input("input", cl::desc("YAML file to load oldname-newname pairs from."),
          cl::Optional, cl::cat(ClangRefactorRenameCategory));

} // namespace rename_module

} // namespace clang_refactor
} // namespace clang

#endif // CLANG_REFACTOR_OPTIONS_H
