//===--- SuspiciousMemsetUsageCheck.cpp - clang-tidy-----------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "SuspiciousMemsetUsageCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Lex/Lexer.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace misc {

void SuspiciousMemsetUsageCheck::registerMatchers(MatchFinder *Finder) {
  // Note: void *memset(void *buffer, int fill_char, size_t byte_count);
  // Look for memset(x, '0', z). Probably memset(x, 0, z) was intended.
  Finder->addMatcher(
      callExpr(
          callee(functionDecl(hasName("::memset"))),
          hasArgument(1, characterLiteral(equals(static_cast<unsigned>('0')))
                             .bind("char-zero-fill")),
          unless(eachOf(hasArgument(0, hasType(pointsTo(isAnyCharacter()))),
                        isInTemplateInstantiation()))),
      this);

  // Look for memset with an integer literal in its fill_char argument.
  // Will check if it gets truncated.
  Finder->addMatcher(callExpr(callee(functionDecl(hasName("::memset"))),
                              hasArgument(1, integerLiteral().bind("num-fill")),
                              unless(isInTemplateInstantiation())),
                     this);

  // Look for memset(x, y, 0) as that is most likely an argument swap.
  Finder->addMatcher(
      callExpr(callee(functionDecl(hasName("::memset"))),
               unless(hasArgument(1, anyOf(characterLiteral(equals(
                                               static_cast<unsigned>('0'))),
                                           integerLiteral()))),
               unless(isInTemplateInstantiation()))
          .bind("call"),
      this);
}

/// \brief Get a StringRef representing a SourceRange.
static StringRef getAsString(const MatchFinder::MatchResult &Result,
                             SourceRange R) {
  const SourceManager &SM = *Result.SourceManager;
  // Don't even try to resolve macro or include contraptions. Not worth emitting
  // a fixit for.
  if (R.getBegin().isMacroID() ||
      !SM.isWrittenInSameFile(R.getBegin(), R.getEnd()))
    return StringRef();

  const char *Begin = SM.getCharacterData(R.getBegin());
  const char *End = SM.getCharacterData(Lexer::getLocForEndOfToken(
      R.getEnd(), 0, SM, Result.Context->getLangOpts()));

  return StringRef(Begin, End - Begin);
}

void SuspiciousMemsetUsageCheck::check(const MatchFinder::MatchResult &Result) {
  // Case 1: fill_char of memset() is a character '0'. Probably an integer zero
  // was intended.
  if (const auto *CharZeroFill =
          Result.Nodes.getNodeAs<CharacterLiteral>("char-zero-fill")) {

    SourceRange CharRange = CharZeroFill->getSourceRange();
    auto Diag =
        diag(CharZeroFill->getLocStart(), "memset fill value is char '0', "
                                          "potentially mistaken for int 0");

    // Only suggest a fix if no macros are involved.
    if (CharRange.getBegin().isMacroID())
      return;
    Diag << FixItHint::CreateReplacement(
        CharSourceRange::getTokenRange(CharRange), "0");
  }

  // Case 2: fill_char of memset() is larger in size than an unsigned char so
  // it gets truncated during conversion.
  else if (const auto *NumFill =
               Result.Nodes.getNodeAs<IntegerLiteral>("num-fill")) {

    llvm::APSInt NumValue;
    const auto UCharMax = (1 << Result.Context->getCharWidth()) - 1;
    if (!NumFill->EvaluateAsInt(NumValue, *Result.Context) ||
        (NumValue >= 0 && NumValue <= UCharMax))
      return;

    diag(NumFill->getLocStart(), "memset fill value is out of unsigned "
                                 "character range, gets truncated");
  }

  // Case 3: byte_count of memset() is zero. This is most likely an argument
  // swap.
  else if (const auto *Call = Result.Nodes.getNodeAs<CallExpr>("call")) {
    const Expr *FillChar = Call->getArg(1);
    const Expr *ByteCount = Call->getArg(2);

    // Return if `byte_count` is not zero at compile time.
    llvm::APSInt Value1, Value2;
    if (ByteCount->isValueDependent() ||
        !ByteCount->EvaluateAsInt(Value2, *Result.Context) || Value2 != 0)
      return;

    // Return if `fill_char` is known to be zero or negative at compile
    // time. In these cases, swapping the args would be a nop, or
    // introduce a definite bug. The code is likely correct.
    if (!FillChar->isValueDependent() &&
        FillChar->EvaluateAsInt(Value1, *Result.Context) &&
        (Value1 == 0 || Value1.isNegative()))
      return;

    // `byte_count` is known to be zero at compile time, and `fill_char` is
    // either not known or known to be a positive integer. Emit a warning
    // and fix-its to swap the arguments.
    auto D = diag(Call->getLocStart(),
                  "memset of size zero, potentially swapped arguments");
    SourceRange LHSRange = FillChar->getSourceRange();
    SourceRange RHSRange = ByteCount->getSourceRange();
    StringRef RHSString = getAsString(Result, RHSRange);
    StringRef LHSString = getAsString(Result, LHSRange);
    if (LHSString.empty() || RHSString.empty())
      return;

    D << FixItHint::CreateReplacement(CharSourceRange::getTokenRange(LHSRange),
                                      RHSString)
      << FixItHint::CreateReplacement(CharSourceRange::getTokenRange(RHSRange),
                                      LHSString);
  }
}

} // namespace misc
} // namespace tidy
} // namespace clang
