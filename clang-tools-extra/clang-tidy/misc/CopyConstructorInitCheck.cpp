//===--- CopyConstructorInitCheck.cpp - clang-tidy-------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "CopyConstructorInitCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Lex/Lexer.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace misc {

AST_MATCHER(CXXCtorInitializer, ctorInit) {
  return cxxCtorInitializer(
             isBaseInitializer(),
             withInitializer(
                 cxxConstructExpr(unless(hasDescendant(implicitCastExpr())))
                     .bind("cruct-expr")))
      .bind("init")
      .matches(Node, Finder, Builder);
}

void CopyConstructorInitCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(
      cxxConstructorDecl(isCopyConstructor(),
                         hasAnyConstructorInitializer(ctorInit())).bind("ctor"),
      this);
}

void CopyConstructorInitCheck::check(
    const MatchFinder::MatchResult &Result) {
  const auto *Ctor = Result.Nodes.getStmtAs<CXXConstructorDecl>("ctor");

  // We match here because we want one warning (and FixIt) for every ctor.
  const auto Matches =
      match(cxxConstructorDecl(isCopyConstructor(),
                               forEachConstructorInitializer(ctorInit())),
            *Ctor, *Result.Context);

  std::string FixItInitList;
  for (const auto &Match : Matches) {
    const auto *Init = Match.getNodeAs<CXXCtorInitializer>("init");

    // Valid when the initializer is written manually (not generated).
    if (Init->getSourceRange().isValid()) {
      const auto *CExpr = Match.getNodeAs<CXXConstructExpr>("cruct-expr");
      diag(CExpr->getLocEnd(),
           "calling an inherited constructor other than the copy constructor")
          << FixItHint::CreateInsertion(
              CExpr->getLocEnd(), Ctor->getParamDecl(0)->getNameAsString());
    } else {
      FixItInitList +=
          Init->getBaseClass()->getAsCXXRecordDecl()->getNameAsString();

      // We want to write in the FixIt the template arguments too.
      if (const auto *Decl = dyn_cast<ClassTemplateSpecializationDecl>(
              Init->getBaseClass()->getAsCXXRecordDecl())) {
        FixItInitList += "<";

        const ArrayRef<TemplateArgument> Args =
            Decl->getTemplateArgs().asArray();
        for (const auto &Arg : Args)
          FixItInitList += Arg.getAsType().getAsString() + ", ";

        FixItInitList = FixItInitList.substr(0, FixItInitList.size() - 2);
        FixItInitList += ">";
      }

      FixItInitList += "(" + Ctor->getParamDecl(0)->getNameAsString() + "), ";
    }
  }
  // Early return if there were just missing parameters.
  if (FixItInitList.empty())
    return;
  FixItInitList = FixItInitList.substr(0, FixItInitList.size() - 2);

  auto &SM = Result.Context->getSourceManager();
  SourceLocation StartLoc = Ctor->getLocation();
  StringRef Buffer = SM.getBufferData(SM.getFileID(StartLoc));
  const char *StartChar = SM.getCharacterData(StartLoc);

  Lexer Lex(StartLoc, (Result.Context)->getLangOpts(), StartChar, StartChar,
            Buffer.end());
  Token Tok;
  // Loop until the beginning of the initialization list (if exists).
  while (!Tok.is(tok::colon) && !Tok.is(tok::l_brace))
    Lex.LexFromRawLexer(Tok);

  std::string FixItMsg;
  // There is not initialization list in this constructor.
  if (Tok.is(tok::l_brace))
    FixItMsg += ": ";
  FixItMsg += FixItInitList;
  // We want to apply the missing constructor at the beginning of the
  // initialization list.
  if (Tok.is(tok::colon)) {
    Lex.LexFromRawLexer(Tok);
    FixItMsg += ", ";
  }

  diag(Tok.getLocation(),
       "calling an inherited constructor other than the copy constructor")
      << FixItHint::CreateInsertion(Tok.getLocation(), FixItMsg);
}

} // namespace misc
} // namespace tidy
} // namespace clang
