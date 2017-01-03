//===--- SuspiciousCallArgumentCheck.h - clang-tidy--------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MISC_SUSPICIOUS_CALL_ARGUMENT_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MISC_SUSPICIOUS_CALL_ARGUMENT_H

#include "../ClangTidy.h"

namespace clang {
namespace tidy {

struct ComparisonInfo;

/// This checker finds those function calls where the function arguments are
/// provided in an incorrect order. It compares the name of the given variable
/// to the argument name in the function definition.
/// It issues a message if the given variable name is similar to an another
/// function argument in a function call. It uses case insensitive comparison.
///
/// For the user-facing documentation see:
/// http://clang.llvm.org/extra/clang-tidy/checks/misc-suspicious-call-argument.html
class SuspiciousCallArgumentCheck : public ClangTidyCheck {
public:
  SuspiciousCallArgumentCheck(StringRef Name, ClangTidyContext *Context);
  void storeOptions(ClangTidyOptions::OptionMap &Opts) override;
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;

  bool checkSimilarity(ComparisonInfo LeftArgToRightParam,
                       ComparisonInfo LeftArgToLeftParam,
                       ComparisonInfo RightArgToRightParam,
                       ComparisonInfo RightArgToLeftParam);

  bool compare(ComparisonInfo OriginalArgToOtherParam,
               ComparisonInfo OriginalArgToOriginalParam,
               ComparisonInfo OtherArgToOtherParam);

  SmallVector<bool, 3> equalityHeur(ComparisonInfo OriginalArgToOtherParam,
                                    ComparisonInfo OriginalArgToOriginalParam,
                                    ComparisonInfo OtherArgToOtherParam);

  SmallVector<bool, 3>
  abbreviationHeur(ComparisonInfo OriginalArgToOtherParam,
                   ComparisonInfo OriginalArgToOriginalParam,
                   ComparisonInfo OtherArgToOtherParam);

  SmallVector<bool, 3> prefixHeur(ComparisonInfo OriginalArgToOtherParam,
                                  ComparisonInfo OriginalArgToOriginalParam,
                                  ComparisonInfo OtherArgToOtherParam);

  SmallVector<bool, 3> suffixHeur(ComparisonInfo OriginalArgToOtherParam,
                                  ComparisonInfo OriginalArgToOriginalParam,
                                  ComparisonInfo OtherArgToOtherParam);
  SmallVector<bool, 3>
  levenshteinHeur(ComparisonInfo OriginalArgToOtherParam,
                  ComparisonInfo OriginalArgToOriginalParam,
                  ComparisonInfo OtherArgToOtherParam);

  SmallVector<bool, 3> substringHeur(ComparisonInfo OriginalArgToOtherParam,
                                     ComparisonInfo OriginalArgToOriginalParam,
                                     ComparisonInfo OtherArgToOtherParam);

  SmallVector<bool, 3>
  jaroWinklerHeur(ComparisonInfo OriginalArgToOtherParam,
                  ComparisonInfo OriginalArgToOriginalParam,
                  ComparisonInfo OtherArgToOtherParam);

  SmallVector<bool, 3> diceHeur(ComparisonInfo OriginalArgToOtherParam,
                                ComparisonInfo OriginalArgToOriginalParam,
                                ComparisonInfo OtherArgToOtherParam);

private:
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
};

} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MISC_SUSPICIOUS_CALL_ARGUMENT_H
