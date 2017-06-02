//===--- RedundantKeywordCheck.cpp - clang-tidy----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "RedundantKeywordCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Lex/Lexer.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace readability {

template <class T>
static bool startsWith(const T &Node, StringRef Value) {
  Token Result;
  Lexer::getRawToken(Node.getLocStart(), Result,
                     Node.getASTContext().getSourceManager(),
                     Node.getASTContext().getLangOpts());
  return Result.isAnyIdentifier() && Result.getRawIdentifier() == Value;
}

AST_MATCHER(FunctionDecl, hasExternKeyword) {
  return startsWith(Node, "extern");
}

AST_MATCHER(CXXMethodDecl, hasInlineKeyword) {
  return startsWith(Node, "inline");
}

void RedundantKeywordCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(
      cxxMethodDecl(hasParent(cxxRecordDecl()), isDefinition(), hasInlineKeyword())
          .bind("redundant_inline"),
      this);
  Finder->addMatcher(
      functionDecl(hasExternKeyword(), unless(anyOf(isDefinition(), isExternC()))).bind("redundant_extern"),
      this);
}

void RedundantKeywordCheck::check(const MatchFinder::MatchResult &Result) {
  auto *RedKeyword =
      Result.Nodes.getNodeAs<FunctionDecl>("redundant_inline");
  auto IsInline = true;

  if (!RedKeyword) {
    RedKeyword =
        Result.Nodes.getNodeAs<FunctionDecl>("redundant_extern");
    IsInline = false;
  }

  if (RedKeyword->getLocStart().isMacroID())
    return;

  auto Diag = diag(RedKeyword->getLocStart(), "redundant '%select{inline|extern}0' keyword") << (IsInline ? 0 : 1);
  Token TokenToRemove;

  Lexer::getRawToken(RedKeyword->getLocStart(), TokenToRemove,
                     *Result.SourceManager,
                     Result.Context->getLangOpts());
  Diag << FixItHint::CreateRemoval(CharSourceRange::getTokenRange(
      RedKeyword->getLocStart(),
      TokenToRemove.getEndLoc()));
}

} // namespace readability
} // namespace tidy
} // namespace clang