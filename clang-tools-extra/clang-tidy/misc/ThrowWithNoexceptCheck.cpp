//===--- ThrowWithNoexceptCheck.cpp - clang-tidy---------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "ThrowWithNoexceptCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Lex/Lexer.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace misc {

void ThrowWithNoexceptCheck::registerMatchers(MatchFinder *Finder) {
  if (!getLangOpts().CPlusPlus11)
    return;
  Finder->addMatcher(
      cxxThrowExpr(stmt(forFunction(functionDecl(isNoThrow()).bind("func"))))
          .bind("throw"),
      this);
}

void ThrowWithNoexceptCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *Function = Result.Nodes.getNodeAs<FunctionDecl>("func");
  const auto *Throw = Result.Nodes.getNodeAs<Stmt>("throw");

  diag(Throw->getLocStart(), "'throw' expression in a function declared with a "
                             "non-throwing exception specification");

  llvm::SmallVector<SourceRange, 5> NoExceptRanges;
  for (const auto *Redecl : Function->redecls()) {
    SourceRange NoExceptRange =
        Redecl->getExceptionSpecSourceRange();

    if (NoExceptRange.isValid()) {
      NoExceptRanges.push_back(NoExceptRange);
    } else {
      // If a single one is not valid, we cannot apply the fix as we need to
      // remove noexcept in all declarations for the fix to be effective.
      NoExceptRanges.clear();
      break;
    }
  }

  for (const auto &NoExceptRange : NoExceptRanges) {
    // FIXME use DiagnosticIDs::Level::Note
    diag(NoExceptRange.getBegin(), "in a function declared no-throw here:", DiagnosticIDs::Note)
        << FixItHint::CreateRemoval(NoExceptRange);
  }
}

} // namespace misc
} // namespace tidy
} // namespace clang
