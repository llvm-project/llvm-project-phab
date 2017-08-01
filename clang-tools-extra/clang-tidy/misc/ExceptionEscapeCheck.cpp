//===--- ExceptionEscapeCheck.cpp - clang-tidy-----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "ExceptionEscapeCheck.h"

#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

#include "llvm/ADT/SmallSet.h"

using namespace clang;
using namespace clang::ast_matchers;

namespace {
typedef llvm::SmallVector<const Type *, 8> TypeVec;
typedef llvm::SmallSet<StringRef, 8> StringSet;

const TypeVec _throws(const FunctionDecl *Func);
const TypeVec _throws(const Stmt *St, const TypeVec &Caught);
} // namespace

namespace clang {
namespace ast_matchers {
AST_MATCHER_P(FunctionDecl, throws, internal::Matcher<Type>, InnerMatcher) {
  auto ExceptionList = _throws(&Node);
  auto *NewEnd = std::remove_if(ExceptionList.begin(), ExceptionList.end(),
                                [this, Finder, Builder](const Type *Exception) {
                                  return !InnerMatcher.matches(*Exception,
                                                               Finder, Builder);
                                });
  ExceptionList.erase(NewEnd, ExceptionList.end());
  return ExceptionList.size();
}

AST_MATCHER_P(Type, isIgnored, StringSet, IgnoredExceptions) {
  if (const auto *TD = Node.getAsTagDecl()) {
    return IgnoredExceptions.count(TD->getNameAsString()) > 0;
  }
  return false;
}

AST_MATCHER_P(FunctionDecl, isEnabled, StringSet, EnabledFunctions) {
  return EnabledFunctions.count(Node.getNameAsString()) > 0;
  return false;
}
} // namespace ast_matchers

namespace tidy {
namespace misc {

ExceptionEscapeCheck::ExceptionEscapeCheck(StringRef Name,
                                           ClangTidyContext *Context)
    : ClangTidyCheck(Name, Context),
      RawEnabledFunctions(Options.get("EnabledFunctions", "")),
      RawIgnoredExceptions(Options.get("IgnoredExceptions", "")) {
  llvm::SmallVector<StringRef, 8> EnabledFunctionsVec, IgnoredExceptionsVec;
  StringRef(RawEnabledFunctions).split(EnabledFunctionsVec, ",", -1, false);
  EnabledFunctions.insert(EnabledFunctionsVec.begin(),
                          EnabledFunctionsVec.end());
  StringRef(RawIgnoredExceptions).split(IgnoredExceptionsVec, ",", -1, false);
  IgnoredExceptions.insert(IgnoredExceptionsVec.begin(),
                           IgnoredExceptionsVec.end());
}

void ExceptionEscapeCheck::storeOptions(ClangTidyOptions::OptionMap &Opts) {
  Options.store(Opts, "EnabledFunctions", RawEnabledFunctions);
  Options.store(Opts, "IgnoredExceptions", RawIgnoredExceptions);
}

void ExceptionEscapeCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(
      functionDecl(allOf(throws(unless(isIgnored(IgnoredExceptions))),
                         anyOf(isNoThrow(), cxxDestructorDecl(),
                               cxxConstructorDecl(isMoveConstructor()),
                               cxxMethodDecl(isMoveAssignmentOperator()),
                               hasName("main"), hasName("swap"),
                               isEnabled(EnabledFunctions))))
          .bind("thrower"),
      this);
}

void ExceptionEscapeCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *MatchedDecl = Result.Nodes.getNodeAs<FunctionDecl>("thrower");
  if (!MatchedDecl)
    return;

  diag(MatchedDecl->getLocation(), "function %0 throws") << MatchedDecl;
}

} // namespace misc
} // namespace tidy
} // namespace clang

namespace {

bool isBaseOf(const Type *DerivedType, const Type *BaseType);

const TypeVec _throws(const FunctionDecl *Func) {
  static thread_local llvm::SmallSet<const FunctionDecl *, 32> CallStack;

  if (CallStack.count(Func))
    return TypeVec();

  if (const auto *Body = Func->getBody()) {
    CallStack.insert(Func);
    const auto Result = _throws(Body, TypeVec());
    CallStack.erase(Func);
    return Result;
  } else {
    TypeVec Result;
    if (const auto *FPT = Func->getType()->getAs<FunctionProtoType>()) {
      for (const auto Ex : FPT->exceptions()) {
        Result.push_back(&*Ex);
      }
    }
    return Result;
  }
}

const TypeVec _throws(const Stmt *St, const TypeVec &Caught) {
  TypeVec Results;

  if (!St)
    return Results;

  if (const auto *Throw = dyn_cast<CXXThrowExpr>(St)) {
    if (const auto *ThrownExpr = Throw->getSubExpr()) {
      const auto *ThrownType =
          ThrownExpr->getType()->getUnqualifiedDesugaredType();
      if (ThrownType->isReferenceType()) {
        ThrownType = ThrownType->castAs<ReferenceType>()
                         ->getPointeeType()
                         ->getUnqualifiedDesugaredType();
      }
      if (const auto *TD = ThrownType->getAsTagDecl()) {
        if (TD->getNameAsString() == "bad_alloc")
          return Results;
      }
      Results.push_back(ThrownExpr->getType()->getUnqualifiedDesugaredType());
    } else {
      Results.append(Caught.begin(), Caught.end());
    }
  } else if (const auto *Try = dyn_cast<CXXTryStmt>(St)) {
    auto Uncaught = _throws(Try->getTryBlock(), Caught);
    for (unsigned i = 0; i < Try->getNumHandlers(); ++i) {
      const auto *Catch = Try->getHandler(i);
      if (!Catch->getExceptionDecl()) {
        const auto Rethrown = _throws(Catch->getHandlerBlock(), Uncaught);
        Results.append(Rethrown.begin(), Rethrown.end());
        Uncaught.clear();
      } else {
        const auto *CaughtType =
            Catch->getCaughtType()->getUnqualifiedDesugaredType();
        if (CaughtType->isReferenceType()) {
          CaughtType = CaughtType->castAs<ReferenceType>()
                           ->getPointeeType()
                           ->getUnqualifiedDesugaredType();
        }
        auto *NewEnd = std::remove_if(Uncaught.begin(), Uncaught.end(),
                                      [&CaughtType](const Type *ThrownType) {
                                        return ThrownType == CaughtType ||
                                               isBaseOf(ThrownType, CaughtType);
                                      });
        if (NewEnd != Uncaught.end()) {
          const auto Rethrown =
              _throws(Catch->getHandlerBlock(), TypeVec(1, CaughtType));
          Results.append(Rethrown.begin(), Rethrown.end());
          Uncaught.erase(NewEnd, Uncaught.end());
        }
      }
    }
    Results.append(Uncaught.begin(), Uncaught.end());
  } else if (const auto *Call = dyn_cast<CallExpr>(St)) {
    if (const auto *Func = Call->getDirectCallee()) {
      auto Excs = _throws(Func);
      Results.append(Excs.begin(), Excs.end());
    }
  } else {
    for (const auto *Child : St->children()) {
      auto Excs = _throws(Child, Caught);
      Results.append(Excs.begin(), Excs.end());
    }
  }
  return Results;
}

bool isBaseOf(const Type *DerivedType, const Type *BaseType) {
  const auto *DerivedClass = DerivedType->getAsCXXRecordDecl();
  const auto *BaseClass = BaseType->getAsCXXRecordDecl();
  if (!DerivedClass || !BaseClass)
    return false;

  return !DerivedClass->forallBases(
      [BaseClass](const CXXRecordDecl *Cur) { return Cur != BaseClass; });
}

} // namespace
