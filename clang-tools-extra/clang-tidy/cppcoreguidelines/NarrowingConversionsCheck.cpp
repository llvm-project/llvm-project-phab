//===--- NarrowingConversionsCheck.cpp - clang-tidy------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "NarrowingConversionsCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace cppcoreguidelines {

void NarrowingConversionsCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(
      binaryOperator(anyOf(hasOperatorName("+="), hasOperatorName("-=")),
                     hasLHS(hasType(isInteger())),
                     hasRHS(hasType(realFloatingPointType()))).bind("op"),
      this);
}

void NarrowingConversionsCheck::check(const MatchFinder::MatchResult &Result) {
  const auto* Op = Result.Nodes.getNodeAs<BinaryOperator>("op");
  diag(Op->getOperatorLoc(),
       "narrowing conversion from %0 to %1")
      << Op->getRHS()->getType()
      << Op->getLHS()->getType();
}

} // namespace cppcoreguidelines
} // namespace tidy
} // namespace clang
