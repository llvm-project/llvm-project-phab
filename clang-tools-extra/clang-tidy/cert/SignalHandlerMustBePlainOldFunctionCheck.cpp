//===--- SignalHandlerMustBePlainOldFunctionCheck.cpp - clang-tidy---------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "SignalHandlerMustBePlainOldFunctionCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include <queue>

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace cert {

namespace {
internal::Matcher<Decl> notExternCSignalHandler() {
  return functionDecl(unless(isExternC())).bind("signal_handler");
}

internal::Matcher<Decl> hasCxxStmt() {
  return hasDescendant(
      stmt(anyOf(cxxBindTemporaryExpr(), cxxBoolLiteral(), cxxCatchStmt(),
                 cxxConstCastExpr(), cxxConstructExpr(), cxxDefaultArgExpr(),
                 cxxDeleteExpr(), cxxDynamicCastExpr(), cxxForRangeStmt(),
                 cxxFunctionalCastExpr(), cxxMemberCallExpr(), cxxNewExpr(),
                 cxxNullPtrLiteralExpr(), cxxOperatorCallExpr(),
                 cxxReinterpretCastExpr(), cxxStaticCastExpr(),
                 cxxTemporaryObjectExpr(), cxxThisExpr(), cxxThrowExpr(),
                 cxxTryStmt(), cxxUnresolvedConstructExpr()))
          .bind("cxx_stmt"));
}

internal::Matcher<Decl> hasCxxDecl() {
  return hasDescendant(
      decl(anyOf(cxxConstructorDecl(), cxxConversionDecl(), cxxDestructorDecl(),
                 cxxMethodDecl(),
                 cxxRecordDecl(unless(anyOf(isStruct(), isUnion())))))
          .bind("cxx_decl"));
}

internal::Matcher<Decl> signalHandlerWithCallExpr() {
  return functionDecl(hasDescendant(callExpr()))
      .bind("signal_handler_with_call_expr");
}

internal::Matcher<Decl> allCallExpr() {
  return decl(forEachDescendant(callExpr().bind("call")));
}

class CallGraphCheck {
  std::queue<const FunctionDecl *> UncheckedCalls;
  llvm::DenseSet<const FunctionDecl *> CheckedCalls;
  SourceLocation CxxRepresentation;
  SourceLocation FunctionCall;
  ASTContext *Context;

  bool hasCxxRepr(const FunctionDecl *Func) {
    auto MatchesCxxStmt = match(hasCxxStmt(), *Func, *Context);
    if (!MatchesCxxStmt.empty()) {
      CxxRepresentation =
          MatchesCxxStmt[0].getNodeAs<Stmt>("cxx_stmt")->getLocStart();
      return true;
    }

    auto MatchesCxxDecl = match(hasCxxDecl(), *Func, *Context);
    if (!MatchesCxxDecl.empty()) {
      CxxRepresentation =
          MatchesCxxDecl[0].getNodeAs<Decl>("cxx_decl")->getLocStart();
      return true;
    }
    return false;
  }

  bool callExprContainsCxxRepr(SmallVectorImpl<BoundNodes> &Calls) {
    for (const auto &Match : Calls) {
      const auto *Call = Match.getNodeAs<CallExpr>("call");
      const auto *Func = Call->getDirectCallee();
      if (!Func || !Func->getDefinition())
        continue;
      auto MatchesCxxMethod = match(decl(cxxMethodDecl()), *Func, *Context);
      if (hasCxxRepr(Func->getDefinition()) || !MatchesCxxMethod.empty()) {
        FunctionCall = Call->getLocStart();
        return true;
      }
      if (!CheckedCalls.count(Func))
        UncheckedCalls.push(Func);
    }
    return false;
  }

public:
  CallGraphCheck(const FunctionDecl *SignalHandler, ASTContext *ResultContext)
      : Context(ResultContext) {
    UncheckedCalls.push(SignalHandler);
  }
  SourceLocation callLoc() { return FunctionCall; }
  SourceLocation cxxRepLoc() { return CxxRepresentation; }

  bool checkAllCall() {
    while (!UncheckedCalls.empty()) {
      CheckedCalls.insert(UncheckedCalls.front());
      auto Matches = match(allCallExpr(),
                           *UncheckedCalls.front()->getDefinition(), *Context);
      UncheckedCalls.pop();
      if (callExprContainsCxxRepr(Matches))
        return true;
    }
    return false;
  }
};
} // namespace

void SignalHandlerMustBePlainOldFunctionCheck::registerMatchers(
    MatchFinder *Finder) {
  auto SignalHas = [](const internal::Matcher<Decl> &SignalHandler) {
    return callExpr(hasDeclaration(functionDecl(hasName("signal"))),
                    hasArgument(1, declRefExpr(hasDeclaration(SignalHandler))
                                       .bind("signal_argument")));
  };
  auto SignalHandler = [](const internal::Matcher<Decl> &HasCxxRepr) {
    return functionDecl(isExternC(), HasCxxRepr)
        .bind("signal_handler_with_cxx_repr");
  };
  Finder->addMatcher(SignalHas(notExternCSignalHandler()), this);
  Finder->addMatcher(SignalHas(SignalHandler(hasCxxStmt())), this);
  Finder->addMatcher(SignalHas(SignalHandler(hasCxxDecl())), this);
  Finder->addMatcher(SignalHas(signalHandlerWithCallExpr()), this);
}

void SignalHandlerMustBePlainOldFunctionCheck::check(
    const MatchFinder::MatchResult &Result) {
  if (const auto *SignalHandler =
          Result.Nodes.getNodeAs<FunctionDecl>("signal_handler")) {
    diag(SignalHandler->getLocation(),
         "use 'external C' prefix for signal handlers");
    diag(Result.Nodes.getNodeAs<DeclRefExpr>("signal_argument")->getLocation(),
         "given as a signal parameter here", DiagnosticIDs::Note);
  }

  if (const auto *SingalHandler = Result.Nodes.getNodeAs<FunctionDecl>(
          "signal_handler_with_cxx_repr")) {
    diag(SingalHandler->getLocation(),
         "do not use C++ representations in signal handlers");
    if (const auto *CxxStmt = Result.Nodes.getNodeAs<Stmt>("cxx_stmt"))
      diag(CxxStmt->getLocStart(), "C++ representation used here",
           DiagnosticIDs::Note);
    else
      diag(Result.Nodes.getNodeAs<Decl>("cxx_decl")->getLocStart(),
           "C++ representation used here", DiagnosticIDs::Note);
  }

  if (const auto *SignalHandler = Result.Nodes.getNodeAs<FunctionDecl>(
          "signal_handler_with_call_expr")) {
    CallGraphCheck CallChain(SignalHandler, Result.Context);
    if (CallChain.checkAllCall()) {
      diag(SignalHandler->getLocation(), "do not call functions with C++ "
                                         "representations in signal "
                                         "handlers");
      diag(CallChain.callLoc(), "function called here", DiagnosticIDs::Note);
      if (CallChain.cxxRepLoc().isValid())
        diag(CallChain.cxxRepLoc(), "C++ representation used here",
             DiagnosticIDs::Note);
    }
  }
}

} // namespace cert
} // namespace tidy
} // namespace clang
