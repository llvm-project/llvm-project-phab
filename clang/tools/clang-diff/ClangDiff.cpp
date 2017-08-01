//===- ClangDiff.cpp - compare source files by AST nodes ------*- C++ -*- -===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements a tool for syntax tree based comparison using
// Tooling/ASTDiff.
//
//===----------------------------------------------------------------------===//

#include "clang/Tooling/ASTDiff/ASTDiff.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/CommandLine.h"

using namespace llvm;
using namespace clang;
using namespace clang::tooling;

static cl::OptionCategory ClangDiffCategory("clang-diff options");

static cl::opt<bool>
    ASTDump("ast-dump",
            cl::desc("Print the internal representation of the AST as JSON."),
            cl::init(false), cl::cat(ClangDiffCategory));

static cl::opt<bool> NoCompilationDatabase(
    "no-compilation-database",
    cl::desc(
        "Do not attempt to load build settings from a compilation database"),
    cl::init(false), cl::cat(ClangDiffCategory));

static cl::opt<std::string> SourcePath(cl::Positional, cl::desc("<source>"),
                                       cl::Required,
                                       cl::cat(ClangDiffCategory));

static cl::opt<std::string> DestinationPath(cl::Positional,
                                            cl::desc("<destination>"),
                                            cl::Optional,
                                            cl::cat(ClangDiffCategory));

static cl::opt<std::string> StopAfter("stop-after",
                                      cl::desc("<topdown|bottomup>"),
                                      cl::Optional, cl::init(""),
                                      cl::cat(ClangDiffCategory));

static cl::opt<int> MaxSize("s", cl::desc("<maxsize>"), cl::Optional,
                            cl::init(-1), cl::cat(ClangDiffCategory));

static cl::opt<std::string> BuildPath("p", cl::desc("Build path"), cl::init(""),
                                      cl::Optional, cl::cat(ClangDiffCategory));

static cl::list<std::string> ArgsAfter(
    "extra-arg",
    cl::desc("Additional argument to append to the compiler command line"),
    cl::cat(ClangDiffCategory));

static cl::list<std::string> ArgsBefore(
    "extra-arg-before",
    cl::desc("Additional argument to prepend to the compiler command line"),
    cl::cat(ClangDiffCategory));

static std::unique_ptr<ASTUnit> getAST(const StringRef Filename) {
  std::string ErrorMessage;
  std::unique_ptr<CompilationDatabase> Compilations;
  if (!NoCompilationDatabase)
    Compilations = CompilationDatabase::autoDetectFromSource(
        BuildPath.empty() ? Filename : BuildPath, ErrorMessage);
  if (!Compilations) {
    if (!NoCompilationDatabase)
      llvm::errs()
          << "Error while trying to load a compilation database, running "
             "without flags.\n"
          << ErrorMessage;
    Compilations = llvm::make_unique<clang::tooling::FixedCompilationDatabase>(
        ".", std::vector<std::string>());
  }
  auto AdjustingCompilations =
      llvm::make_unique<ArgumentsAdjustingCompilations>(
          std::move(Compilations));
  AdjustingCompilations->appendArgumentsAdjuster(
      getInsertArgumentAdjuster(ArgsBefore, ArgumentInsertPosition::BEGIN));
  AdjustingCompilations->appendArgumentsAdjuster(
      getInsertArgumentAdjuster(ArgsAfter, ArgumentInsertPosition::END));
  Compilations = std::move(AdjustingCompilations);
  std::array<std::string, 1> Files = {{Filename}};
  ClangTool Tool(*Compilations, Files);
  std::vector<std::unique_ptr<ASTUnit>> ASTs;
  Tool.buildASTs(ASTs);
  if (ASTs.size() != Files.size())
    return nullptr;
  return std::move(ASTs[0]);
}

int main(int argc, const char **argv) {
  cl::HideUnrelatedOptions(ClangDiffCategory);
  if (!cl::ParseCommandLineOptions(argc, argv)) {
    cl::PrintOptionValues();
    return 1;
  }

  if (ASTDump) {
    if (!DestinationPath.empty()) {
      llvm::errs() << "Error: Please specify exactly one filename.\n";
      return 1;
    }
    std::unique_ptr<ASTUnit> AST = getAST(SourcePath);
    if (!AST)
      return 1;
    diff::SyntaxTree Tree(AST->getASTContext());
    Tree.printAsJson(llvm::outs());
    return 0;
  }

  if (DestinationPath.empty()) {
    llvm::errs() << "Error: Exactly two paths are required.\n";
    return 1;
  }

  std::unique_ptr<ASTUnit> Src = getAST(SourcePath);
  std::unique_ptr<ASTUnit> Dst = getAST(DestinationPath);
  if (!Src || !Dst)
    return 1;

  diff::ComparisonOptions Options;
  if (MaxSize != -1)
    Options.MaxSize = MaxSize;
  if (!StopAfter.empty()) {
    if (StopAfter == "topdown")
      Options.StopAfterTopDown = true;
    else if (StopAfter != "bottomup") {
      llvm::errs() << "Error: Invalid argument for -stop-after";
      return 1;
    }
  }
  diff::SyntaxTree SrcTree(Src->getASTContext());
  diff::SyntaxTree DstTree(Dst->getASTContext());
  diff::ASTDiff DiffTool(SrcTree, DstTree, Options);
  for (const auto &Match : DiffTool.getMatches())
    DiffTool.printMatch(llvm::outs(), Match);
  for (const auto &Change : DiffTool.getChanges())
    DiffTool.printChange(llvm::outs(), Change);

  return 0;
}
