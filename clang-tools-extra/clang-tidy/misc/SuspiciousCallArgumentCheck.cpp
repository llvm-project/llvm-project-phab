//===--- SuspiciousCallArgumentCheck.cpp - clang-tidy----------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "SuspiciousCallArgumentCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include <sstream>

using namespace clang::ast_matchers;

namespace {

llvm::StringMap<StringRef> AbbrDict;

// https://en.wikipedia.org/wiki/S%C3%B8rensen%E2%80%93Dice_coefficient
// Get the Dice coefficient of two strings.
double diceMatch(StringRef Arg, StringRef Param) {

  if (Arg.size() < 3 || Param.size() < 3)
    return 0;

  llvm::StringSet<> Arg_bigrams;
  llvm::StringSet<> Param_bigrams;

  // Extract character bigrams from Arg.
  for (unsigned i = 0; i < (Arg.size() - 1); i++) {
    Arg_bigrams.insert(Arg.substr(i, 2));
  }

  // Extract character bigrams from Param.
  for (unsigned i = 0; i < (Param.size() - 1); i++) {
    Param_bigrams.insert(Param.substr(i, 2));
  }

  int Intersection = 0;

  // Find the intersection between the two sets.
  for (auto IT = Param_bigrams.begin(); IT != Param_bigrams.end(); IT++) {
    Intersection += Arg_bigrams.count((IT->getKey()));
  }

  // Calculate dice coefficient.
  double Dice =
      (Intersection * 2.0) / (Arg_bigrams.size() + Param_bigrams.size());

  return Dice * 100;
}

// https://en.wikipedia.org/wiki/Jaro%E2%80%93Winkler_distance
// Get the Jaro distance of two strings.
double jaroWinklerMatch(StringRef Arg, StringRef Param) {

  if (Arg.size() < 3 || Param.size() < 3)
    return 0;

  int Match = 0, Transpos = 0;
  int ArgLen = Arg.size();
  int ParamLen = Param.size();
  llvm::SmallVector<int, 8> ArgFlags(ArgLen);
  llvm::SmallVector<int, 8> ParamFlags(ParamLen);
  int Range = std::max(0, std::max(ArgLen, ParamLen) / 2 - 1);

  if (!ArgLen || !ParamLen)
    return 0.0;

  // Calculate matching characters.
  for (int i = 0; i < ParamLen; i++) {
    for (int j = std::max(i - Range, 0), l = std::min(i + Range + 1, ArgLen);
         j < l; j++) {
      if (tolower(Param[i]) == tolower(Arg[j]) && !ArgFlags[j]) {
        ArgFlags[j] = 1;
        ParamFlags[i] = 1;
        Match++;
        break;
      }
    }
  }

  if (!Match)
    return 0.0;

  // Calculate character transpositions.
  int l = 0;
  for (int i = 0; i < ParamLen; i++) {
    if (ParamFlags[i] == 1) {
      int j;
      for (j = l; j < ArgLen; j++) {
        if (ArgFlags[j] == 1) {
          l = j + 1;
          break;
        }
      }
      if (tolower(Param[i]) != tolower(Arg[j]))
        Transpos++;
    }
  }
  Transpos /= 2;

  // Jaro distance.
  double Dist = (((double)Match / ArgLen) + ((double)Match / ParamLen) +
                 ((double)(Match - Transpos) / Match)) /
                3.0;

  // Calculate common string prefix up to 4 chars.
  l = 0;
  for (int i = 0; i < std::min(std::min(ArgLen, ParamLen), 4); i++)
    if (tolower(Arg[i]) == tolower(Param[i]))
      l++;

  // Jaro-Winkler distance.
  Dist = Dist + (l * 0.1 * (1 - Dist));

  return Dist * 100;
}

bool isAnyAbbreviation(StringRef Arg, StringRef Param) {

  if (AbbrDict.count(Arg) == 1) {
    if (Param.str() == AbbrDict.lookup(Arg).str())
      return true;
  }

  if (AbbrDict.count(Param) == 1) {
    if (Arg.str() == AbbrDict.lookup(Param).str())
      return true;
  }

  return false;
}

// Get the Levenshtein distance of two strings.
double levenshteinDist(StringRef Arg, StringRef Param) {

  if (Arg.size() < 3 || Param.size() < 3)
    return 0;

  double Dist = Arg.edit_distance(Param);

  StringRef Longer = Arg.size() >= Param.size() ? Arg : Param;

  return (1 - Dist / Longer.size()) * 100;
}

// Check whether the shorter String is a prefix of the longer String.
double prefixMatch(StringRef Arg, StringRef Param) {

  if (Arg.size() < 3 || Param.size() < 3)
    return 0;

  StringRef Shorter = Arg.size() < Param.size() ? Arg : Param;
  StringRef Longer = Arg.size() >= Param.size() ? Arg : Param;

  if (Longer.startswith_lower(Shorter))
    return (double)Shorter.size() / Longer.size() * 100;

  return 0;
}

// Check whether the shorter String is a suffix of the longer String.
double suffixMatch(StringRef Arg, StringRef Param) {

  if (Arg.size() < 3 || Param.size() < 3)
    return 0;

  StringRef Shorter = Arg.size() < Param.size() ? Arg : Param;
  StringRef Longer = Arg.size() >= Param.size() ? Arg : Param;

  if (Longer.endswith_lower(Shorter))
    return (double)Shorter.size() / Longer.size() * 100;

  return 0;
}

double longestCommonSubstringRatio(StringRef Arg, StringRef Param) {

  if (Arg.size() < 3 || Param.size() < 3)
    return 0;

  int MaxLength = 0;
  llvm::SmallVector<int, 8> Current(Param.size());
  llvm::SmallVector<int, 8> Previous(Param.size());

  for (unsigned i = 0; i < Arg.size(); ++i) {
    for (unsigned j = 0; j < Param.size(); ++j) {
      if (Arg[i] == Param[j]) {
        if (i == 0 || j == 0) {
          Current[j] = 1;
        } else {
          Current[j] = 1 + Previous[j - 1];
        }

        MaxLength = std::max(MaxLength, Current[j]);
      } else {
        Current[j] = 0;
      }
    }

    Current.swap(Previous);
  }

  StringRef Longer = Arg.size() >= Param.size() ? Arg : Param;
  return (double)MaxLength / Longer.size() * 100;
}

} // namespace

namespace clang {
namespace tidy {

// Struct to store information about the relation of params and args.
struct ComparisonInfo {
  bool Equal;
  bool Abbreviation;
  double LevenshteinDistance;
  double Prefix;
  double Suffix;
  double LongestCommonSubStr;
  double JaroWinkler;
  double Dice;
  int ArgLength;
  int ParamLength;
  bool TypeMatch;

  ComparisonInfo(bool Eq = false, bool Ab = false, double Ld = 0, double Pr = 0,
                 double Su = 0, double Lcs = 0, double Jw = 0,
                 double Di = 0, int Al = 0, int Pl = 0, bool Tm = true)
      : Equal(Eq), Abbreviation(Ab), LevenshteinDistance(Ld), Prefix(Pr),
        Suffix(Su), LongestCommonSubStr(Lcs), JaroWinkler(Jw),
        Dice(Di), ArgLength(Al), ParamLength(Pl), TypeMatch(Tm) {}
};

// Constructor.
SuspiciousCallArgumentCheck::SuspiciousCallArgumentCheck(
    StringRef Name, ClangTidyContext *Context)
    : ClangTidyCheck(Name, Context), Equality(Options.get("Equality", true)),
      Abbreviation(Options.get("Abbreviation", true)),
      Levenshtein(Options.get("Levenshtein", true)),
      Prefix(Options.get("Prefix", true)), Suffix(Options.get("Suffix", true)),
      Substring(Options.get("Substring", true)),
      JaroWinkler(Options.get("JaroWinkler", true)),
      Dice(Options.get("Dice", true)),
      LevenshteinUpperBound(Options.get("LevenshteinUpperBound", 66)),
      PrefixUpperBound(Options.get("PrefixUpperBound", 30)),
      SuffixUpperBound(Options.get("SuffixUpperBound", 30)),
      SubstringUpperBound(Options.get("SubstringUpperBound", 50)),
      JaroWinklerUpperBound(Options.get("JaroWinklerUpperBound", 85)),
      DiceUpperBound(Options.get("DiceUpperBound", 70)),
      LevenshteinLowerBound(Options.get("LevenshteinLowerBound", 50)),
      PrefixLowerBound(Options.get("PrefixLowerBound", 25)),
      SuffixLowerBound(Options.get("SuffixLowerBound", 25)),
      SubstringLowerBound(Options.get("SubstringLowerBound", 40)),
      JaroWinklerLowerBound(Options.get("JaroWinklerLowerBound", 75)),
      DiceLowerBound(Options.get("DiceLowerBound", 60)) {

  AbbrDict.insert(std::make_pair(StringRef("ptr"), StringRef("pointer")));
  AbbrDict.insert(std::make_pair(StringRef("len"), StringRef("length")));
  AbbrDict.insert(std::make_pair(StringRef("ind"), StringRef("index")));
  AbbrDict.insert(std::make_pair(StringRef("addr"), StringRef("address")));
  AbbrDict.insert(std::make_pair(StringRef("arr"), StringRef("array")));
  AbbrDict.insert(std::make_pair(StringRef("cpy"), StringRef("copy")));
  AbbrDict.insert(std::make_pair(StringRef("src"), StringRef("source")));
  AbbrDict.insert(std::make_pair(StringRef("val"), StringRef("value")));
  AbbrDict.insert(std::make_pair(StringRef("ln"), StringRef("line")));
  AbbrDict.insert(std::make_pair(StringRef("num"), StringRef("number")));
  AbbrDict.insert(std::make_pair(StringRef("nr"), StringRef("number")));
  AbbrDict.insert(std::make_pair(StringRef("stmt"), StringRef("statement")));
  AbbrDict.insert(std::make_pair(StringRef("var"), StringRef("variable")));
  AbbrDict.insert(std::make_pair(StringRef("buf"), StringRef("buffer")));
  AbbrDict.insert(std::make_pair(StringRef("txt"), StringRef("text")));
  AbbrDict.insert(std::make_pair(StringRef("dest"), StringRef("destination")));
  AbbrDict.insert(std::make_pair(StringRef("elem"), StringRef("element")));
  AbbrDict.insert(std::make_pair(StringRef("wdth"), StringRef("width")));
  AbbrDict.insert(std::make_pair(StringRef("hght"), StringRef("height")));
  AbbrDict.insert(std::make_pair(StringRef("cnt"), StringRef("count")));
  AbbrDict.insert(std::make_pair(StringRef("i"), StringRef("index")));
}

// Options.
void SuspiciousCallArgumentCheck::storeOptions(
    ClangTidyOptions::OptionMap &Opts) {

  Options.store(Opts, "Equality", Equality);
  Options.store(Opts, "Abbreviation", Abbreviation);
  Options.store(Opts, "Levenshtein", Levenshtein);
  Options.store(Opts, "Prefix", Prefix);
  Options.store(Opts, "Suffix", Suffix);
  Options.store(Opts, "Substring", Substring);
  Options.store(Opts, "JaroWinkler", JaroWinkler);
  Options.store(Opts, "Dice", Dice);

  Options.store(Opts, "LevenshteinUpperBound", LevenshteinUpperBound);
  Options.store(Opts, "PrefixUpperBound", PrefixUpperBound);
  Options.store(Opts, "SuffixUpperBound", SuffixUpperBound);
  Options.store(Opts, "SubstringUpperBound", SubstringUpperBound);
  Options.store(Opts, "JaroWinklerUpperBound", JaroWinklerUpperBound);
  Options.store(Opts, "DiceUpperBound", DiceUpperBound);

  Options.store(Opts, "LevenshteinLowerBound", LevenshteinLowerBound);
  Options.store(Opts, "PrefixLowerBound", PrefixLowerBound);
  Options.store(Opts, "SuffixLowerBound", SuffixLowerBound);
  Options.store(Opts, "SubstringLowerBound", SubstringLowerBound);
  Options.store(Opts, "JaroWinklerLowerBound", JaroWinklerLowerBound);
  Options.store(Opts, "DiceLowerBound", DiceLowerBound);
}

// Matcher.
void SuspiciousCallArgumentCheck::registerMatchers(MatchFinder *Finder) {
  // Only match calls with at least 2 argument.
  Finder->addMatcher(
      callExpr(unless(argumentCountIs(0)), unless(argumentCountIs(1)))
          .bind("functionCall"),
      this);
}

void SuspiciousCallArgumentCheck::check(
    const MatchFinder::MatchResult &Result) {
  const auto *MatchedCallExpr =
      Result.Nodes.getNodeAs<CallExpr>("functionCall");

  const Decl *CalleeDecl = MatchedCallExpr->getCalleeDecl();
  if (!CalleeDecl)
    return;

  const FunctionDecl *CalleeFuncDecl = CalleeDecl->getAsFunction();
  if (!CalleeFuncDecl)
    return;

  // Lambda functions first argument are themself.
  int InitialArgIndex = 0;

  if (const auto *MethodDecl = dyn_cast<CXXMethodDecl>(CalleeFuncDecl)) {
    if (MethodDecl->getParent()->isLambda())
      InitialArgIndex = 1;
  }

  // Get param identifiers.
  SmallVector<StringRef, 8> ParamNames;
  SmallVector<const Type *, 8> ParamTypes;
  for (unsigned i = 0, e = CalleeFuncDecl->getNumParams(); i != e; ++i) {
    const ParmVarDecl *PVD = CalleeFuncDecl->getParamDecl(i);

    if (auto *ParamType =
            PVD->getOriginalType().getNonReferenceType().getTypePtrOrNull()) {
      ParamTypes.push_back(ParamType->getUnqualifiedDesugaredType());
    } else {
      ParamTypes.push_back(nullptr);
    }

    IdentifierInfo *II = PVD->getIdentifier();
    if (!II)
      return;

    ParamNames.push_back(II->getName());
  }

  if (ParamNames.empty())
    return;

  // Get arg names.
  SmallVector<StringRef, 8> Args;
  SmallVector<const Type *, 8> ArgTypes;
  for (int i = InitialArgIndex, j = MatchedCallExpr->getNumArgs(); i < j; i++) {
    if (const auto *DeclRef = dyn_cast<DeclRefExpr>(
            MatchedCallExpr->getArg(i)->IgnoreParenImpCasts())) {
      if (const auto *Var = dyn_cast<VarDecl>(DeclRef->getDecl())) {

        if (auto *ArgType =
                Var->getType().getNonReferenceType().getTypePtrOrNull()) {
          ArgTypes.push_back(ArgType->getUnqualifiedDesugaredType());
        } else {
          ArgTypes.push_back(nullptr);
        }

        Args.push_back(Var->getName());
        continue;
      }
    }

    ArgTypes.push_back(nullptr);
    Args.push_back(StringRef());
  }

  if (Args.empty())
    return;

  // In case of variadic functions.
  unsigned MatrixSize = ParamNames.size();

  // Create infomatrix.
  SmallVector<SmallVector<ComparisonInfo, 8>, 8> InfoMatrix;
  for (unsigned i = 0; i < MatrixSize; i++) {
    InfoMatrix.push_back(SmallVector<ComparisonInfo, 8>());
    for (unsigned j = 0; j < MatrixSize; j++) {
      if (Args[i].empty()) {
        InfoMatrix[i].push_back(ComparisonInfo());
        continue;
      }

      InfoMatrix[i].push_back(
          ComparisonInfo(Args[i].equals_lower(ParamNames[j]),
                         isAnyAbbreviation(Args[i], ParamNames[j]),
                         levenshteinDist(Args[i], ParamNames[j]),
                         prefixMatch(Args[i], ParamNames[j]),
                         suffixMatch(Args[i], ParamNames[j]),
                         longestCommonSubstringRatio(Args[i], ParamNames[j]),
                         jaroWinklerMatch(Args[i], ParamNames[j]),
                         diceMatch(Args[i], ParamNames[j]), Args[i].size(),
                         ParamNames[j].size(), ArgTypes[i] == ParamTypes[j]));
    }
  }

  // Check similarity.
  for (unsigned i = 0; i < MatrixSize; i++) {
    for (unsigned j = i + 1; j < MatrixSize; j++) {

      if (checkSimilarity(InfoMatrix[i][j], InfoMatrix[i][i], InfoMatrix[j][j],
                          InfoMatrix[j][i])) {

        // TODO: Underline swapped arguments.
        // Warning at the function call.
        diag(MatchedCallExpr->getLocStart(),
             "%0 (%1) is swapped with %2 (%3).")
            << Args[i] << ParamNames[i] << Args[j] << ParamNames[j];

        // Note at the functions declaration.
        diag(CalleeFuncDecl->getLocStart(), "%0 is declared here:",
             DiagnosticIDs::Note)
            << CalleeFuncDecl->getNameInfo().getName().getAsString();

        return;
      }
    }
  }
}

// Symmetric check of 2 parameters and 2 arguments.
bool SuspiciousCallArgumentCheck::checkSimilarity(
    ComparisonInfo LeftArgToRightParam, ComparisonInfo LeftArgToLeftParam,
    ComparisonInfo RightArgToRightParam, ComparisonInfo RightArgToLeftParam) {

  return compare(LeftArgToRightParam, LeftArgToLeftParam,
                 RightArgToRightParam) ||
         compare(RightArgToLeftParam, RightArgToRightParam, LeftArgToLeftParam);
}

// Compare an argument and a parameter using all enabled heuristics.
bool SuspiciousCallArgumentCheck::compare(
    ComparisonInfo OriginalArgToOtherParam,
    ComparisonInfo OriginalArgToOriginalParam,
    ComparisonInfo OtherArgToOtherParam) {

  if (OtherArgToOtherParam.ParamLength < 3 ||
      OriginalArgToOtherParam.ArgLength < 3 ||
      OriginalArgToOriginalParam.ParamLength < 3 ||
      OtherArgToOtherParam.ArgLength < 3)
    return false;

  // TypeMatch
  if (!OriginalArgToOtherParam.TypeMatch &&
      OriginalArgToOriginalParam.TypeMatch && OtherArgToOtherParam.TypeMatch)
    return false;

  // Equality.
  SmallVector<bool, 3> EqualityResults =
      equalityHeur(OriginalArgToOtherParam, OriginalArgToOriginalParam,
                   OtherArgToOtherParam);

  // Abbreviation.
  SmallVector<bool, 3> AbbreviationResults =
      abbreviationHeur(OriginalArgToOtherParam, OriginalArgToOriginalParam,
                       OtherArgToOtherParam);

  // Prefix.
  SmallVector<bool, 3> PrefixResults =
      prefixHeur(OriginalArgToOtherParam, OriginalArgToOriginalParam,
                 OtherArgToOtherParam);

  // Suffix.
  SmallVector<bool, 3> SuffixResults =
      suffixHeur(OriginalArgToOtherParam, OriginalArgToOriginalParam,
                 OtherArgToOtherParam);

  // Levenshtein.
  SmallVector<bool, 3> LevenshteinResults =
      levenshteinHeur(OriginalArgToOtherParam, OriginalArgToOriginalParam,
                      OtherArgToOtherParam);

  // Longest common substring.
  SmallVector<bool, 3> SubstringResults =
      substringHeur(OriginalArgToOtherParam, OriginalArgToOriginalParam,
                    OtherArgToOtherParam);

  // Jaro-Winkler.
  SmallVector<bool, 3> JaroWinklerResults =
      jaroWinklerHeur(OriginalArgToOtherParam, OriginalArgToOriginalParam,
                      OtherArgToOtherParam);

  // Dice coefficient.
  SmallVector<bool, 3> DiceResults =
      diceHeur(OriginalArgToOtherParam, OriginalArgToOriginalParam,
               OtherArgToOtherParam);

  // [0] == OriginalArgToOtherParam
  // [1] == OriginalArgToOriginalParam
  // [2] == OtherArgToOtherParam
  if ((EqualityResults[0] || AbbreviationResults[0] || PrefixResults[0] ||
       SuffixResults[0] || LevenshteinResults[0] || SubstringResults[0] ||
       JaroWinklerResults[0] || DiceResults[0]) &&
      !EqualityResults[1] && !EqualityResults[2] && !AbbreviationResults[1] &&
      !AbbreviationResults[2] && !PrefixResults[1] && !PrefixResults[2] &&
      !SuffixResults[1] && !SuffixResults[2] && !LevenshteinResults[1] &&
      !LevenshteinResults[2] && !SubstringResults[1] && !SubstringResults[2] &&
      !JaroWinklerResults[1] &&
      !JaroWinklerResults[2] && !DiceResults[1] && !DiceResults[2]) {

    return true;
  }

  return false;
}

// Equality heuristic.
SmallVector<bool, 3> SuspiciousCallArgumentCheck::equalityHeur(
    ComparisonInfo OriginalArgToOtherParam,
    ComparisonInfo OriginalArgToOriginalParam,
    ComparisonInfo OtherArgToOtherParam) {

  SmallVector<bool, 3> results;

  results.push_back(Equality && OriginalArgToOtherParam.Equal);

  results.push_back(Equality && OriginalArgToOriginalParam.Equal);
  results.push_back(Equality && OtherArgToOtherParam.Equal);

  return results;
}

// Abbreviation heuristic.
SmallVector<bool, 3> SuspiciousCallArgumentCheck::abbreviationHeur(
    ComparisonInfo OriginalArgToOtherParam,
    ComparisonInfo OriginalArgToOriginalParam,
    ComparisonInfo OtherArgToOtherParam) {

  SmallVector<bool, 3> results;

  results.push_back(Abbreviation && OriginalArgToOtherParam.Abbreviation);

  results.push_back(Abbreviation && OriginalArgToOriginalParam.Abbreviation);
  results.push_back(Abbreviation && OtherArgToOtherParam.Abbreviation);

  return results;
}

// Prefix heuristic.
SmallVector<bool, 3> SuspiciousCallArgumentCheck::prefixHeur(
    ComparisonInfo OriginalArgToOtherParam,
    ComparisonInfo OriginalArgToOriginalParam,
    ComparisonInfo OtherArgToOtherParam) {

  SmallVector<bool, 3> results;

  results.push_back(Prefix &&
                    OriginalArgToOtherParam.Prefix > PrefixUpperBound);

  results.push_back(Prefix &&
                    OriginalArgToOriginalParam.Prefix > PrefixLowerBound);
  results.push_back(Prefix && OtherArgToOtherParam.Prefix > PrefixLowerBound);

  return results;
}

// Suffix heuristic.
SmallVector<bool, 3> SuspiciousCallArgumentCheck::suffixHeur(
    ComparisonInfo OriginalArgToOtherParam,
    ComparisonInfo OriginalArgToOriginalParam,
    ComparisonInfo OtherArgToOtherParam) {

  SmallVector<bool, 3> results;

  results.push_back(Suffix &&
                    OriginalArgToOtherParam.Suffix > SuffixUpperBound);

  results.push_back(Suffix &&
                    OriginalArgToOriginalParam.Suffix > SuffixLowerBound);
  results.push_back(Suffix && OtherArgToOtherParam.Suffix > SuffixLowerBound);

  return results;
}

// Levenshtein heuristic.
SmallVector<bool, 3> SuspiciousCallArgumentCheck::levenshteinHeur(
    ComparisonInfo OriginalArgToOtherParam,
    ComparisonInfo OriginalArgToOriginalParam,
    ComparisonInfo OtherArgToOtherParam) {

  SmallVector<bool, 3> results;

  results.push_back(Levenshtein &&
                    OriginalArgToOtherParam.LevenshteinDistance >
                        LevenshteinUpperBound);

  results.push_back(Levenshtein &&
                    OriginalArgToOriginalParam.LevenshteinDistance >
                        LevenshteinLowerBound);
  results.push_back(Levenshtein &&
                    OtherArgToOtherParam.LevenshteinDistance >
                        LevenshteinLowerBound);

  return results;
}

// Longest common substring heuristic.
SmallVector<bool, 3> SuspiciousCallArgumentCheck::substringHeur(
    ComparisonInfo OriginalArgToOtherParam,
    ComparisonInfo OriginalArgToOriginalParam,
    ComparisonInfo OtherArgToOtherParam) {

  SmallVector<bool, 3> results;

  results.push_back(Substring &&
                    OriginalArgToOtherParam.LongestCommonSubStr >
                        SubstringUpperBound);

  results.push_back(Substring &&
                    OriginalArgToOriginalParam.LongestCommonSubStr >
                        SubstringLowerBound);
  results.push_back(Substring &&
                    OtherArgToOtherParam.LongestCommonSubStr >
                        SubstringLowerBound);

  return results;
}

// JaroWinkler heuristic.
SmallVector<bool, 3> SuspiciousCallArgumentCheck::jaroWinklerHeur(
    ComparisonInfo OriginalArgToOtherParam,
    ComparisonInfo OriginalArgToOriginalParam,
    ComparisonInfo OtherArgToOtherParam) {

  SmallVector<bool, 3> results;

  results.push_back(JaroWinkler &&
                    OriginalArgToOtherParam.JaroWinkler >
                        JaroWinklerUpperBound);

  results.push_back(JaroWinkler &&
                    OriginalArgToOriginalParam.JaroWinkler >
                        JaroWinklerLowerBound);
  results.push_back(JaroWinkler &&
                    OtherArgToOtherParam.JaroWinkler > JaroWinklerLowerBound);

  return results;
}

// Dice heuristic.
SmallVector<bool, 3>
SuspiciousCallArgumentCheck::diceHeur(ComparisonInfo OriginalArgToOtherParam,
                                      ComparisonInfo OriginalArgToOriginalParam,
                                      ComparisonInfo OtherArgToOtherParam) {

  SmallVector<bool, 3> results;

  results.push_back(Dice && OriginalArgToOtherParam.Dice > DiceUpperBound);

  results.push_back(Dice && OriginalArgToOriginalParam.Dice > DiceLowerBound);
  results.push_back(Dice && OtherArgToOtherParam.Dice > DiceLowerBound);

  return results;
}

} // namespace tidy
} // namespace clang
