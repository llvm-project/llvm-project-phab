//===--- AvoidPointerCastToMoreStrictAlignmentCheck.cpp - clang-tidy-------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "AvoidPointerCastToMoreStrictAlignmentCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace cert {

void AvoidPointerCastToMoreStrictAlignmentCheck::registerMatchers(
    MatchFinder *Finder) {
  Finder->addMatcher(
      castExpr(hasSourceExpression(expr().bind("source"))).bind("target"),
      this);
}

void AvoidPointerCastToMoreStrictAlignmentCheck::check(
    const MatchFinder::MatchResult &Result) {
  const auto *Source = Result.Nodes.getNodeAs<Expr>("source");
  const auto *Target = Result.Nodes.getNodeAs<CastExpr>("target");

  QualType SourceType = Source->getType();
  QualType TargetType = Target->getType();

  if (!SourceType->isPointerType() || !TargetType->isPointerType())
    return;

  QualType SourcePointedType = SourceType->getPointeeType();
  QualType TargetPointedType = TargetType->getPointeeType();

  if (SourcePointedType->isIncompleteType() ||
      TargetPointedType->isIncompleteType())
    return;

  if (Result.Context->getTypeAlign(SourcePointedType) >=
      Result.Context->getTypeAlign(TargetPointedType))
    return;

  if (Target->getCastKind() == CK_BitCast)
    diag(Target->getLocStart(),
         "do not cast pointers into more strictly aligned pointer types");
}

} // namespace cert
} // namespace tidy
} // namespace clang
