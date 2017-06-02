//===--- TerminatingContinueCheck.cpp - clang-tidy-------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "TerminatingContinueCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Lex/Lexer.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace misc {

void TerminatingContinueCheck::registerMatchers(MatchFinder *Finder) {
  const auto doWithFalse =
      doStmt(hasCondition(ignoringImpCasts(
                 anyOf(cxxBoolLiteral(equals(false)), integerLiteral(equals(0)),
                       cxxNullPtrLiteralExpr(), gnuNullExpr()))),
             equalsBoundNode("closestLoop"))
          .bind("doWithFalseCond");

  Finder->addMatcher(
      continueStmt(anyOf(hasAncestor(forStmt().bind("closestLoop")),
                         hasAncestor(cxxForRangeStmt().bind("closestLoop")),
                         hasAncestor(whileStmt().bind("closestLoop")),
                         hasAncestor(doStmt().bind("closestLoop"))),
                   hasAncestor(doWithFalse))
          .bind("continue"),
      this);
}

void TerminatingContinueCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *ContStmt = Result.Nodes.getNodeAs<ContinueStmt>("continue");

  auto Diag = diag(ContStmt->getLocStart(), "terminating 'continue'");
  Diag << FixItHint::CreateReplacement(ContStmt->getSourceRange(), "break");
}

} // namespace misc
} // namespace tidy
} // namespace clang
