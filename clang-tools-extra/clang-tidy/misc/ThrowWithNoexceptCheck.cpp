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
      cxxThrowExpr(stmt(forFunction(functionDecl(isNoThrow()).bind("func"))))
          .bind("throw"),
      this);
}

// FIXME: Move to utils as it is used in multiple checks.
// Looks for the token matching the predicate and returns the range of the found
// token including trailing whitespace.
static SourceRange findToken(const SourceManager &Sources, LangOptions LangOpts,
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
static SourceRange parenExprRange(const SourceManager &Sources,
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
  assert(Tok.is(tok::l_paren));
  int OpenParenCount = 1;
  do {
    Lex.LexFromRawLexer(Tok);
    if (Tok.is(tok::r_paren)) {
      --OpenParenCount;
      if (OpenParenCount == 0) {
        Token NextTok;
        Lex.LexFromRawLexer(NextTok);
        return SourceRange(StartLoc, NextTok.getLocation());
      }
    } else if (Tok.is(tok::l_paren)) {
      ++OpenParenCount;
    }
  } while (Tok.isNot(tok::eof) && Tok.getLocation() < EndLoc);

  return SourceRange();
}

static SourceRange findNoExceptRange(const ASTContext &Context,
                                     const SourceManager &Sources,
                                     const FunctionDecl &Function) {
  // FIXME: Support dynamic exception specification (e.g. throw())
  /* Find noexcept token */
  auto isKWNoexcept = [](const Token &Tok) {
    return Tok.is(tok::raw_identifier) && Tok.getRawIdentifier() == "noexcept";
  };
  SourceRange NoExceptTokenRange =
      findToken(Sources, Context.getLangOpts(), Function.getOuterLocStart(),
                Function.getLocEnd(), isKWNoexcept);

  if (!NoExceptTokenRange.isValid())
    return SourceRange();

  const auto *ProtoType = Function.getType()->getAs<FunctionProtoType>();
  const auto NoexceptSpec = ProtoType->getNoexceptSpec(Context);
  if (NoexceptSpec != FunctionProtoType::NR_Nothrow) {
    /* We have noexcept that doesn't evaluate to true,
     * something strange or malformed. */
    return SourceRange();
  } else if (ProtoType->getNoexceptExpr() == nullptr) {
    /* We have noexcept without the expression inside */
    return NoExceptTokenRange;
  }

  /* Now we have to merge */
  SourceRange ExprRange =
      parenExprRange(Sources, Context.getLangOpts(),
                     NoExceptTokenRange.getEnd(), Function.getLocEnd());

  if (!ExprRange.isValid())
    return SourceRange();

  return SourceRange(NoExceptTokenRange.getBegin(), ExprRange.getEnd());
}

void ThrowWithNoexceptCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *Function = Result.Nodes.getNodeAs<FunctionDecl>("func");
  const auto *Throw = Result.Nodes.getNodeAs<Stmt>("throw");
  diag(Throw->getLocStart(), "'throw' expression in a function declared with a "
                             "non-throwing exception specification");

  llvm::SmallVector<SourceRange, 5> NoExceptRanges;
  for (const auto *Redecl : Function->redecls()) {
    SourceRange NoExceptRange =
        findNoExceptRange(*Result.Context, *Result.SourceManager, *Redecl);

    if (NoExceptRange.isValid()) {
      NoExceptRanges.push_back(NoExceptRange);
    } else {
      /* If a single one is not valid, we cannot apply the fix as we need to
       * remove noexcept in all declarations for the fix to be effective. */
      NoExceptRanges.clear();
      break;
    }
  }

  for (const auto &NoExceptRange : NoExceptRanges) {
    // FIXME use DiagnosticIDs::Level::Note
    diag(NoExceptRange.getBegin(), "In function declared no-throw here:")
        << FixItHint::CreateRemoval(CharSourceRange::getCharRange(
               NoExceptRange.getBegin(), NoExceptRange.getEnd()));
  }
}

} // namespace misc
} // namespace tidy
} // namespace clang
