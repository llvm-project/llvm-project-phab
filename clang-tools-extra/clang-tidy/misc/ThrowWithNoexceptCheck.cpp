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
      cxxThrowExpr(stmt(hasAncestor(functionDecl(isNoThrow()).bind("func"))))
          .bind("throw"),
      this);
}

// Looks for the token matching the predicate and returns the range of the found
// token including trailing whitespace.
static SourceRange FindToken(const SourceManager &Sources, LangOptions LangOpts,
                             SourceLocation StartLoc, SourceLocation EndLoc,
                             bool (*Pred)(const Token &)) {
  if (StartLoc.isMacroID() || EndLoc.isMacroID())
    return SourceRange();
  FileID File = Sources.getFileID(Sources.getSpellingLoc(StartLoc));
  StringRef Buf = Sources.getBufferData(File);
  const char *StartChar = Sources.getCharacterData(StartLoc);
  Lexer Lex(StartLoc, LangOpts, StartChar, StartChar, Buf.end());
  Lex.SetCommentRetentionState(true);
  Token Tok;
  do {
    Lex.LexFromRawLexer(Tok);
    if (Pred(Tok)) {
      Token NextTok;
      Lex.LexFromRawLexer(NextTok);
      return SourceRange(Tok.getLocation(), NextTok.getLocation());
    }
  } while (Tok.isNot(tok::eof) && Tok.getLocation() < EndLoc);

  return SourceRange();
}

// Finds expression enclosed in parentheses, starting at StartLoc.
// Returns the range of the expression including trailing whitespace.
static SourceRange ParenExprRange(const SourceManager &Sources,
                                  LangOptions LangOpts, SourceLocation StartLoc,
                                  SourceLocation EndLoc) {
  if (StartLoc.isMacroID() || EndLoc.isMacroID())
    return SourceRange();
  FileID File = Sources.getFileID(Sources.getSpellingLoc(StartLoc));
  StringRef Buf = Sources.getBufferData(File);
  const char *StartChar = Sources.getCharacterData(StartLoc);
  Lexer Lex(StartLoc, LangOpts, StartChar, StartChar, Buf.end());
  Lex.SetCommentRetentionState(true);
  Token Tok;
  Lex.LexFromRawLexer(Tok);
  if (!Tok.is(tok::l_paren)) {
    return SourceRange();
    assert(false);
  }
  int OpenCnt = 1;
  do {
    Lex.LexFromRawLexer(Tok);
    if (Tok.is(tok::r_paren)) {
      --OpenCnt;
      if (OpenCnt == 0) {
        Token NextTok;
        Lex.LexFromRawLexer(NextTok);
        return SourceRange(StartLoc, NextTok.getLocation());
      }
    } else if (Tok.is(tok::l_paren)) {
      ++OpenCnt;
    }
  } while (Tok.isNot(tok::eof) && Tok.getLocation() < EndLoc);

  return SourceRange();
}

SourceRange FindNoExceptRange(const ASTContext &Context,
                              const SourceManager &Sources,
                              const FunctionDecl &Function) {
  /* Find noexcept token */
  auto isKWNoexcept = [](const Token &Tok) {
    return Tok.is(tok::raw_identifier) && Tok.getRawIdentifier() == "noexcept";
  };
  SourceRange NoExceptTokenRange =
      FindToken(Sources, Context.getLangOpts(), Function.getOuterLocStart(),
                Function.getLocEnd(), isKWNoexcept);

  if (!NoExceptTokenRange.isValid()) {
    return SourceRange();
  }

  const auto *ProtoType = Function.getType()->getAs<FunctionProtoType>();
  const auto NoexceptSpec = ProtoType->getNoexceptSpec(Context);
  if (NoexceptSpec != FunctionProtoType::NR_Nothrow) {
    /* We have noexcept without the expression inside */
    return SourceRange();
  } else if (ProtoType->getNoexceptExpr() == nullptr) {
    return NoExceptTokenRange;
  }

  /* Now we have to merge */
  SourceRange ExprRange =
      ParenExprRange(Sources, Context.getLangOpts(),
                     NoExceptTokenRange.getEnd(), Function.getLocEnd());

  if (!ExprRange.isValid()) {
    return SourceRange();
  }

  return SourceRange(NoExceptTokenRange.getBegin(), ExprRange.getEnd());
}

void ThrowWithNoexceptCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *Function = Result.Nodes.getNodeAs<FunctionDecl>("func");
  const auto *Throw = Result.Nodes.getNodeAs<Stmt>("throw");
  diag(Throw->getLocStart(), "throw in function declared no-throw");
  for (auto Redecl : Function->redecls()) {
    SourceRange NoExceptRange =
        FindNoExceptRange(*Result.Context, *Result.SourceManager, *Redecl);
    if (NoExceptRange.isValid()) {
      diag(NoExceptRange.getBegin(), "In function declared no-throw here:")
          << FixItHint::CreateRemoval(CharSourceRange::getCharRange(
                 NoExceptRange.getBegin(), NoExceptRange.getEnd()));
    }
  }
}

} // namespace misc
} // namespace tidy
} // namespace clang
