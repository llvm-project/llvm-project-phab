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

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace misc {

void SuspiciousMemsetUsageCheck::registerMatchers(MatchFinder *Finder) {
  const auto ClassWVirtualFunc = cxxRecordDecl(
      anyOf(hasMethod(isVirtual()),
            isDerivedFrom(cxxRecordDecl(hasMethod(isVirtual())))));

  const auto MemsetThisCall =
      callExpr(callee(functionDecl(hasName("::memset"))),
               hasArgument(0, cxxThisExpr().bind("this-dest")),
               unless(isInTemplateInstantiation()));

  Finder->addMatcher(
      callExpr(
          callee(functionDecl(hasName("::memset"))),
          hasArgument(1, characterLiteral(equals(static_cast<unsigned>('0')))
                             .bind("char-zero-fill")),
          unless(hasArgument(0, hasType(pointsTo(isAnyCharacter()))))),
      this);

  Finder->addMatcher(
      callExpr(callee(functionDecl(hasName("::memset"))),
               hasArgument(1, integerLiteral().bind("num-fill"))),
      this);

  Finder->addMatcher(
      cxxMethodDecl(
          hasParent(ClassWVirtualFunc),
          anyOf(hasDescendant(MemsetThisCall),
                hasDescendant(lambdaExpr(hasDescendant(MemsetThisCall)))),
          unless(hasDescendant(cxxRecordDecl(hasDescendant(MemsetThisCall))))),
      this);
}

void SuspiciousMemsetUsageCheck::check(const MatchFinder::MatchResult &Result) {
  // Case 1: Fill value of memset is a character '0'.
  // Possibly an integer zero was intended.
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

  // Case 2: Fill value of memset is larger in size than an
  // unsigned char, which means it gets truncated during conversion.
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

  // Case 3: Destination pointer of memset is a this pointer.
  // If the class containing the memset call has a virtual method,
  // memset is likely to corrupt the virtual method table.
  // Inner structs form an exception.
  else if (const auto *ThisDest =
               Result.Nodes.getNodeAs<CXXThisExpr>("this-dest")) {

    diag(ThisDest->getLocStart(), "memset used on this pointer potentially "
                                  "corrupts virtual method table");
  }
}

} // namespace misc
} // namespace tidy
} // namespace clang
