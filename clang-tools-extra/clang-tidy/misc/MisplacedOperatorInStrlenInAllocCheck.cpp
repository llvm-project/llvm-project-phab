//===--- MisplacedOperatorInStrlenInAllocCheck.cpp - clang-tidy------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "MisplacedOperatorInStrlenInAllocCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace misc {

void MisplacedOperatorInStrlenInAllocCheck::registerMatchers(
    MatchFinder *Finder) {
  const auto BadUse =
      callExpr(
          callee(functionDecl(hasName("strlen"))),
          hasAnyArgument(ignoringParenImpCasts(
              binaryOperator(anyOf(hasOperatorName("+"), hasOperatorName("-")))
                  .bind("BinOp"))))
          .bind("StrLen");
  Finder->addMatcher(
      callExpr(callee(functionDecl(hasName("malloc"))),
               hasArgument(0, anyOf(hasDescendant(BadUse), BadUse)))
          .bind("Alloc"),
      this);
  Finder->addMatcher(
      callExpr(
          callee(functionDecl(anyOf(hasName("calloc"), hasName("realloc")))),
          hasArgument(1, anyOf(hasDescendant(BadUse), BadUse)))
          .bind("Alloc"),
      this);
}

void MisplacedOperatorInStrlenInAllocCheck::check(
    const MatchFinder::MatchResult &Result) {
  // FIXME: Add callback implementation.
  const auto *Alloc = Result.Nodes.getNodeAs<CallExpr>("Alloc");
  const auto *StrLen = Result.Nodes.getNodeAs<CallExpr>("StrLen");
  const auto *BinOp = Result.Nodes.getNodeAs<BinaryOperator>("BinOp");

  const std::string LHSText = Lexer::getSourceText(
      CharSourceRange::getTokenRange(BinOp->getLHS()->getSourceRange()),
      *Result.SourceManager, getLangOpts());
  const std::string RHSText = Lexer::getSourceText(
      CharSourceRange::getTokenRange(BinOp->getRHS()->getSourceRange()),
      *Result.SourceManager, getLangOpts());

  auto Hint = FixItHint::CreateReplacement(
      StrLen->getSourceRange(),
      "strlen(" + LHSText + ")" +
          ((BinOp->getOpcode() == BO_Add) ? " + " : " - ") + RHSText);

  diag(Alloc->getLocStart(), "Binary operator %0 %1 is inside strlen")
      << ((BinOp->getOpcode() == BO_Add) ? "+" : "-") << BinOp->getRHS()
      << Hint;
}

} // namespace misc
} // namespace tidy
} // namespace clang
