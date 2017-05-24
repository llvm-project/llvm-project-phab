//===--- DefaultNumericsCheck.cpp - clang-tidy-----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "DefaultNumericsCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace misc {

void DefaultNumericsCheck::registerMatchers(MatchFinder *Finder) {
  if (!getLangOpts().CPlusPlus)
    return;
  // FIXME: We should also warn in template intantiations, but it would require
  // printing backtrace.
  Finder->addMatcher(
      callExpr(
          callee(cxxMethodDecl(
              hasAnyName("min", "max"),
              ofClass(cxxRecordDecl(unless(isExplicitTemplateSpecialization()),
                                    hasName("::std::numeric_limit"))))),
          unless(isInTemplateInstantiation()))
          .bind("call"),
      this);
}

void DefaultNumericsCheck::check(const MatchFinder::MatchResult &Result) {

  const auto *MatchedDecl = Result.Nodes.getNodeAs<CallExpr>("call");

  diag(MatchedDecl->getLocStart(),
       "called std::numeric_limit method on not specialized type");
}

} // namespace misc
} // namespace tidy
} // namespace clang
