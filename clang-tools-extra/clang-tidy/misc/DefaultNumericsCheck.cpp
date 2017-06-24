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
              ofClass(classTemplateSpecializationDecl(
                  hasName("::std::numeric_limits"),
                  unless(isExplicitTemplateSpecialization()),
                  hasTemplateArgument(0, templateArgument().bind("type")))))),
          unless(isInTemplateInstantiation()))
          .bind("call"),
      this);
}

void DefaultNumericsCheck::check(const MatchFinder::MatchResult &Result) {
  const auto &MatchedCall = *Result.Nodes.getNodeAs<CallExpr>("call");
  const auto &NumericType = *Result.Nodes.getNodeAs<TemplateArgument>("type");
  bool IsMax = MatchedCall.getCalleeDecl()->getAsFunction()->getName() == "max";

  diag(MatchedCall.getLocStart(),
       "'std::numeric_limits::%select{min|max}0' called with type %1; no such "
       "specialization exists, so the default value for that type is returned")
      << IsMax << NumericType;
}

} // namespace misc
} // namespace tidy
} // namespace clang
