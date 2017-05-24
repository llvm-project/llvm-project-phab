//===--- ConstValueReturnCheck.cpp - clang-tidy----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "ConstValueReturnCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace readability {

void ConstValueReturnCheck::registerMatchers(MatchFinder *Finder) {
  // Returning const pointers (not pointers to const) does not make much sense,
  // returning const references even less so, but it is harmless and we don't
  // bother checking it.
  // Template parameter types may end up being pointers or references, so we
  // skip those too.
  Finder->addMatcher(functionDecl(returns(qualType(
        isConstQualified(),
        unless(anyOf(
          isAnyPointer(),
          references(type()),
          templateTypeParmType(),
          hasCanonicalType(templateTypeParmType())
      ))))).bind("func"), this);
}

void ConstValueReturnCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *Func = Result.Nodes.getNodeAs<FunctionDecl>("func");
  diag(Func->getLocation(), "function %0 has a const value return type; this "
                            "can cause unnecessary copies")
      << Func;
  // We do not have a FixItHint, because we cannot get the source range for the
  // return type (getReturnTypeSourceRange excludes the const qualifier).
}

} // namespace readability
} // namespace tidy
} // namespace clang
