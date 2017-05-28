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
#include "clang/Lex/Lexer.h"

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

static bool isWithinRange(const Token& Tok, SourceRange Range,
                          const SourceManager &Sources) {
  return (Sources.isBeforeInTranslationUnit(
              Range.getBegin(), Tok.getLocation()) &&
          Sources.isBeforeInTranslationUnit(
              Tok.getLocation(), Range.getEnd()));
}

// Looks for const tokens within SearchRange, ignoring those within
// ReturnRange. If exactly one is found, it is returned.
static llvm::Optional<Token> findConstToken(
    SourceRange SearchRange, SourceRange ReturnRange,
    const MatchFinder::MatchResult &Result) {
  const SourceManager &Sources = *Result.SourceManager;
  FileID FID;
  unsigned Offset;
  std::tie(FID, Offset) = Sources.getDecomposedLoc(SearchRange.getBegin());
  StringRef FileData = Sources.getBufferData(FID);

  Lexer RawLexer(Sources.getLocForStartOfFile(FID),
                 Result.Context->getLangOpts(),
                 FileData.begin(), FileData.begin() + Offset,
                 FileData.end());

  Token Tok;
  llvm::Optional<Token> ConstTok;
  int ConstFound = 0;
  while (!RawLexer.LexFromRawLexer(Tok)) {
    // Stop at the end of SearchRange.
    if (Sources.isBeforeInTranslationUnit(
        SearchRange.getEnd(), Tok.getLocation()))
      break;
    if (Tok.is(tok::raw_identifier)) {
      IdentifierInfo &Info = Result.Context->Idents.get(StringRef(
          Sources.getCharacterData(Tok.getLocation()), Tok.getLength()));
      Tok.setIdentifierInfo(&Info);
      Tok.setKind(Info.getTokenID());
    }
    // Ignore consts inside ReturnRange.
    if (isWithinRange(Tok, ReturnRange, Sources))
      continue;
    if (Tok.is(tok::kw_const)) {
      ++ConstFound;
      ConstTok = Tok;
    }
  }

  if (ConstFound > 1)
    ConstTok.reset();

  return ConstTok;
}

void ConstValueReturnCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *Func = Result.Nodes.getNodeAs<FunctionDecl>("func");
  auto Diag = diag(Func->getLocation(),
                   "function %0 has a const value return type; "
                   "this can cause unnecessary copies")
      << Func;

  auto ReturnType = Func->getReturnType();
  if (!ReturnType.isLocalConstQualified())
    return;

  const auto FuncRange = Func->getSourceRange();
  if (FuncRange.isInvalid())
    return;

  // When using trailing return type syntax, an invalid range is returned,
  // so we cannot generate the fixit.
  const auto ReturnRange = Func->getReturnTypeSourceRange();
  if (ReturnRange.isInvalid())
    return;

  // getReturnTypeSourceRange does not include the qualifiers, so we have to
  // find the "const" ourselves.
  auto NameLoc = Func->getNameInfo().getLoc();
  auto MaybeConst = findConstToken({FuncRange.getBegin(), NameLoc},
                                   ReturnRange, Result);
  if (!MaybeConst)
    return;

  auto TokLoc = MaybeConst->getLocation();
  Diag << FixItHint::CreateRemoval(
      CharSourceRange::getTokenRange(TokLoc, TokLoc));
}

} // namespace readability
} // namespace tidy
} // namespace clang
