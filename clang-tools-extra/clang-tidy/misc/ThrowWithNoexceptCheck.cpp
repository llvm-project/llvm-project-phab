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
  if (!getLangOpts().CPlusPlus)
    return;
  Finder->addMatcher(
      cxxThrowExpr(stmt(forFunction(functionDecl(isNoThrow()).bind("func"))))
          .bind("throw"),
      this);
}

bool isSimpleConstant(const Expr *E) { return isa<CXXBoolLiteralExpr>(E); }

bool isCaughtWithType(const Type *Caught, const Type *Thrown) {
  // FIXME: The logic below is a very rough approximation of the real rules.
  // The real rules are much more complicated and probably
  // should be implemented elsewhere.

  if (Caught == nullptr) // the case of catch(...)
    return true;

  assert(Thrown != nullptr);
  if (Caught == Thrown)
    return true;

  const auto *CaughtAsRecordType = Caught->getPointeeCXXRecordDecl();
  const auto *ThrownAsRecordType = Thrown->getAsCXXRecordDecl();

  if (CaughtAsRecordType && ThrownAsRecordType) {
    if (CaughtAsRecordType == ThrownAsRecordType)
      return true;
    return ThrownAsRecordType->isDerivedFrom(CaughtAsRecordType);
  }

  return false;
}

bool isCaughtInTry(const CXXThrowExpr *Throw, const CXXTryStmt *Try) {
  for (unsigned int i = 0; i < Try->getNumHandlers(); ++i) {
    const auto *Catch = Try->getHandler(i);
    const auto *CaughtType = Catch->getCaughtType().getTypePtrOrNull();
    const auto *ThrownType = Throw->getSubExpr()->getType().getTypePtrOrNull();
    return isCaughtWithType(CaughtType, ThrownType);
  }
  return false;
}

bool isCaughtInFunction(ASTContext *Context, const CXXThrowExpr *Throw,
                        const FunctionDecl *Function,
                        const ast_type_traits::DynTypedNode node) {
  if (node.get<FunctionDecl>() == Function)
    return false;

  const auto *Try = node.get<CXXTryStmt>();
  if (Try != nullptr && isCaughtInTry(Throw, Try))
    return true;

  for (const auto Parent : Context->getParents(node))
    if (isCaughtInFunction(Context, Throw, Function, Parent))
      return true;

  return false;
}

void ThrowWithNoexceptCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *Function = Result.Nodes.getNodeAs<FunctionDecl>("func");
  const auto *Throw = Result.Nodes.getNodeAs<CXXThrowExpr>("throw");

  llvm::SmallVector<SourceRange, 5> NoExceptRanges;
  for (const auto *Redecl : Function->redecls()) {
    const auto *Proto = Redecl->getType()->getAs<FunctionProtoType>();

    const auto *NoExceptExpr = Proto->getNoexceptExpr();
    if (NoExceptExpr != nullptr && !isSimpleConstant(NoExceptExpr)) {
      // There is a complex noexcept expression, so we assume that
      // it can potentially be true or false, based on the compilation
      // flags etc.
      return;
    }

    SourceRange NoExceptRange = Redecl->getExceptionSpecSourceRange();

    if (NoExceptRange.isValid()) {
      NoExceptRanges.push_back(NoExceptRange);
    } else {
      // If a single one is not valid, we cannot apply the fix as we need to
      // remove noexcept in all declarations for the fix to be effective.
      NoExceptRanges.clear();
      break;
    }
  }

  const auto ThrowNode = ast_type_traits::DynTypedNode::create(*Throw);

  if (isCaughtInFunction(Result.Context, Throw, Function, ThrowNode))
    return;

  diag(Throw->getLocStart(), "'throw' expression in a function declared with a "
                             "non-throwing exception specification");

  const auto *Destructor = Result.Nodes.getNodeAs<CXXDestructorDecl>("func");
  if (Destructor != nullptr) {
    diag(Destructor->getSourceRange().getBegin(),
         "in a destructor defined here", DiagnosticIDs::Note);
    return;
  }

  for (const auto &NoExceptRange : NoExceptRanges)
    diag(NoExceptRange.getBegin(), "in a function declared nothrow here",
         DiagnosticIDs::Note)
        << FixItHint::CreateRemoval(NoExceptRange);
}

} // namespace misc
} // namespace tidy
} // namespace clang
