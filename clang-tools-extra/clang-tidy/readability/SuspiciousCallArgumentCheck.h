//===--- SuspiciousCallArgumentCheck.h - clang-tidy--------------*- C -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_READABILITY_SUSPICIOUS_CALL_ARGUMENT_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_READABILITY_SUSPICIOUS_CALL_ARGUMENT_H

#include "../ClangTidy.h"

namespace clang {
namespace tidy {
namespace readability {

/// This checker finds those function calls where the function arguments are
/// provided in an incorrect order. It compares the name of the given variable
/// to the argument name in the function definition.
/// It issues a message if the given variable name is similar to an another
/// function argument in a function call. It uses case insensitive comparison.
///
/// For the user-facing documentation see:
/// http://clang.llvm.org/extra/clang-tidy/checks/readability-suspicious-call-argument.html
class SuspiciousCallArgumentCheck : public ClangTidyCheck {
public:
  SuspiciousCallArgumentCheck(StringRef Name, ClangTidyContext *Context);
  void storeOptions(ClangTidyOptions::OptionMap &Opts) override;
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;

  const static unsigned VectorSmallSize = 8;

private:
  enum class Heuristic {
    Equality,
    Abbreviation,
    Prefix,
    Suffix,
    Levenshtein,
    Substring,
    JaroWinkler,
    Dice
  };

  enum class Bound { Lower, Upper };

  SmallVector<Heuristic, VectorSmallSize> AppliedHeuristics;

  SmallVector<QualType, VectorSmallSize> ArgTypes;
  SmallVector<StringRef, VectorSmallSize> ArgNames;
  SmallVector<QualType, VectorSmallSize> ParamTypes;
  SmallVector<StringRef, VectorSmallSize> ParamNames;

  const bool Equality;
  const bool Abbreviation;
  const bool Levenshtein;
  const bool Prefix;
  const bool Suffix;
  const bool Substring;
  const bool JaroWinkler;
  const bool Dice;

  const int LevenshteinUpperBound;
  const int PrefixUpperBound;
  const int SuffixUpperBound;
  const int SubstringUpperBound;
  const int JaroWinklerUpperBound;
  const int DiceUpperBound;

  const int LevenshteinLowerBound;
  const int PrefixLowerBound;
  const int SuffixLowerBound;
  const int SubstringLowerBound;
  const int JaroWinklerLowerBound;
  const int DiceLowerBound;

  void setParamNamesAndTypes(const FunctionDecl *CalleeFuncDecl);

  void setArgNamesAndTypes(const CallExpr *MatchedCallExpr,
                           unsigned InitialArgIndex);

  bool areParamAndArgComparable(unsigned Position1, unsigned Position2,
                                const ASTContext *Ctx) const;

  bool areArgsSwapped(unsigned Position1, unsigned Position2) const;

  bool areNamesSimilar(StringRef Arg, StringRef Param, Bound bound) const;
};

} // namespace readability
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_READABILITY_SUSPICIOUS_CALL_ARGUMENT_H
