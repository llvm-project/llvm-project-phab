//===--- UnnecessaryIntermediateVarCheck.h - clang-tidy--------------*- C++
//-*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_READABILITY_UNNECESSARY_INTERMEDIATE_VAR_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_READABILITY_UNNECESSARY_INTERMEDIATE_VAR_H

#include "../ClangTidy.h"
#include "llvm/ADT/SmallSet.h"

namespace clang {
namespace tidy {
namespace readability {

/// This checker detects unnecessary intermediate variables used to store the
/// result of an expression just before using it in a return statement.
///
/// For the user-facing documentation see:
/// http://clang.llvm.org/extra/clang-tidy/checks/readability-unnecessary-intermediate-var.html
class UnnecessaryIntermediateVarCheck : public ClangTidyCheck {
public:
  UnnecessaryIntermediateVarCheck(StringRef Name, ClangTidyContext *Context)
      : ClangTidyCheck(Name, Context),
        MaximumLineLength(Options.get("MaximumLineLength", 100)) {}
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;

  void storeOptions(ClangTidyOptions::OptionMap &Opts) override;

private:
  void emitMainWarning(const VarDecl *VarDecl1,
                       const VarDecl *VarDecl2 = nullptr);
  void emitUsageInComparisonNote(const DeclRefExpr *VarRef,
                                 const bool IsPlural);
  void emitVarDeclRemovalNote(const VarDecl *VarDecl1,
                              const DeclStmt *DeclStmt1,
                              const VarDecl *VarDecl2 = nullptr,
                              const DeclStmt *DeclStmt2 = nullptr);
  void emitReturnReplacementNote(const Expr *LHS,
                                 const StringRef LHSReplacement,
                                 const Expr *RHS = nullptr,
                                 const StringRef RHSReplacement = StringRef(),
                                 const BinaryOperator *ReverseBinOp = nullptr);

  unsigned MaximumLineLength;
  llvm::SmallSet<const DeclStmt *, 10> CheckedDeclStmt;
};

} // namespace readability
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_READABILITY_UNNECESSARY_INTERMEDIATE_VAR_H
