//===--- UselessIntermediateVarCheck.h - clang-tidy--------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_READABILITY_USELESS_INTERMEDIATE_VAR_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_READABILITY_USELESS_INTERMEDIATE_VAR_H

#include <unordered_set>
#include "../ClangTidy.h"

namespace clang {
namespace tidy {
namespace readability {

/// This checker detects useless intermediate variables used to store the
/// result of an expression just before using it in a return statement.
///
/// For the user-facing documentation see:
/// http://clang.llvm.org/extra/clang-tidy/checks/readability-useless-intermediate-var.html
class UselessIntermediateVarCheck : public ClangTidyCheck {
public:
  UselessIntermediateVarCheck(StringRef Name, ClangTidyContext *Context)
      : ClangTidyCheck(Name, Context),
        MaximumLineLength(Options.get("MaximumLineLength", 100)) {}
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;

  void storeOptions(ClangTidyOptions::OptionMap &Opts) override;

private:
  void emitMainWarning(const VarDecl* VarDecl1, const VarDecl* VarDecl2 = nullptr);
  void emitUsageInComparisonNote(const DeclRefExpr *VarRef, bool IsPlural);
  void emitVarDeclRemovalNote(const VarDecl* VarDecl1, const DeclStmt* DeclStmt1,
                              const VarDecl* VarDecl2 = nullptr,
                              const DeclStmt* DeclStmt2 = nullptr);
  void emitReturnReplacementNote(const Expr *LHS, StringRef LHSReplacement,
                                 const Expr *RHS = nullptr,
                                 StringRef RHSReplacement = StringRef(),
                                 const BinaryOperator *ReverseBinOp = nullptr);

  unsigned MaximumLineLength;
  std::unordered_set<const DeclStmt*> CheckedDeclStmt;
};

} // namespace readability
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_READABILITY_USELESS_INTERMEDIATE_VAR_H
