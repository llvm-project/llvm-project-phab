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
namespace bugprone {

void MisplacedOperatorInStrlenInAllocCheck::registerMatchers(
    MatchFinder *Finder) {
  const auto BadUse =
      callExpr(callee(functionDecl(hasName("strlen"))),
               hasAnyArgument(ignoringParenImpCasts(
                   binaryOperator(allOf(hasOperatorName("+"),
                                        hasRHS(ignoringParenImpCasts(
                                            integerLiteral(equals(1))))))
                       .bind("BinOp"))))
          .bind("StrLen");
  const auto BadArg = anyOf(
      allOf(hasDescendant(BadUse),
            unless(binaryOperator(allOf(
                hasOperatorName("+"), hasLHS(BadUse),
                hasRHS(ignoringParenImpCasts(integerLiteral(equals(1)))))))),
      BadUse);
  Finder->addMatcher(callExpr(callee(functionDecl(
                                  anyOf(hasName("malloc"), hasName("alloca")))),
                              hasArgument(0, BadArg))
                         .bind("Alloc"),
                     this);
  Finder->addMatcher(callExpr(callee(functionDecl(anyOf(hasName("calloc"),
                                                        hasName("realloc")))),
                              hasArgument(1, BadArg))
                         .bind("Alloc"),
                     this);
}

void MisplacedOperatorInStrlenInAllocCheck::check(
    const MatchFinder::MatchResult &Result) {
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

} // namespace bugprone
} // namespace tidy
} // namespace clang
