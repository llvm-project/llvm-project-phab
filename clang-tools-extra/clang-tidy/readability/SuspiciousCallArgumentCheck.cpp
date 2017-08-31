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
#include "clang/AST/Type.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include <sstream>

using namespace clang::ast_matchers;

namespace {
llvm::StringMap<StringRef> AbbreviationDictionary;
} // namespace

namespace clang {
namespace tidy {
namespace readability {

static bool applyEqualityHeuristic(StringRef Arg, StringRef Param) {
  return Arg.equals_lower(Param);
}

static bool applyAbbreviationHeuristic(StringRef Arg, StringRef Param) {
  if (AbbreviationDictionary.find(Arg) != AbbreviationDictionary.end()) {
    if (Param.compare(AbbreviationDictionary.lookup(Arg)) == 0)
      return true;
  }

  if (AbbreviationDictionary.find(Param) != AbbreviationDictionary.end()) {
    if (Arg.compare(AbbreviationDictionary.lookup(Param)) == 0)
      return true;
  }

  return false;
}

// Check whether the shorter String is a prefix of the longer String.
static bool applyPrefixHeuristic(StringRef Arg, StringRef Param,
                                 unsigned threshold) {
  StringRef Shorter = Arg.size() < Param.size() ? Arg : Param;
  StringRef Longer = Arg.size() >= Param.size() ? Arg : Param;

  double PrefixMatch = 0;
  if (Longer.startswith_lower(Shorter))
    PrefixMatch = (double)Shorter.size() / Longer.size() * 100;

  return PrefixMatch > threshold;
}

// Check whether the shorter String is a suffix of the longer String.
static bool applySuffixHeuristic(StringRef Arg, StringRef Param,
                                 unsigned threshold) {
  StringRef Shorter = Arg.size() < Param.size() ? Arg : Param;
  StringRef Longer = Arg.size() >= Param.size() ? Arg : Param;

  double SuffixMatch = 0;
  if (Longer.endswith_lower(Shorter))
    SuffixMatch = (double)Shorter.size() / Longer.size() * 100;

  return SuffixMatch > threshold;
}

static bool applySubstringHeuristic(StringRef Arg, StringRef Param,
                                    unsigned threshold) {
  unsigned MaxLength = 0;
  llvm::SmallVector<unsigned, SuspiciousCallArgumentCheck::VectorSmallSize>
      Current(Param.size());
  llvm::SmallVector<unsigned, SuspiciousCallArgumentCheck::VectorSmallSize>
      Previous(Param.size());

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

  size_t LongerLength = std::max(Arg.size(), Param.size());
  double SubstringRatio = (double)MaxLength / LongerLength * 100;

  return SubstringRatio > threshold;
}

static bool applyLevenshteinHeuristic(StringRef Arg, StringRef Param,
                                      unsigned threshold) {
  unsigned Dist = Arg.edit_distance(Param);
  size_t LongerLength = std::max(Arg.size(), Param.size());
  Dist = (1 - (double)Dist / LongerLength) * 100;
  return Dist > threshold;
}

// https://en.wikipedia.org/wiki/Jaro%E2%80%93Winkler_distance
static bool applyJaroWinklerHeuristic(StringRef Arg, StringRef Param,
                                      unsigned threshold) {
  int Match = 0, Transpos = 0;
  int ArgLen = Arg.size();
  int ParamLen = Param.size();
  llvm::SmallVector<int, SuspiciousCallArgumentCheck::VectorSmallSize> ArgFlags(
      ArgLen);
  llvm::SmallVector<int, SuspiciousCallArgumentCheck::VectorSmallSize>
      ParamFlags(ParamLen);
  int Range = std::max(0, std::max(ArgLen, ParamLen) / 2 - 1);

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
    return false;

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
  Dist = (Dist + (l * 0.1 * (1 - Dist))) * 100;

  return Dist > threshold;
}

// https://en.wikipedia.org/wiki/S%C3%B8rensen%E2%80%93Dice_coefficient
static bool applyDiceHeuristic(StringRef Arg, StringRef Param,
                               unsigned threshold) {
  double Dice = 0;

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
  Dice =
      (Intersection * 2.0) / (Arg_bigrams.size() + Param_bigrams.size()) * 100;

  return Dice > threshold;
}

// Checks if ArgType binds to ParamType ragerding reference-ness and
// cv-qualifiers.
static bool areRefAndQualCompatible(QualType ArgType, QualType ParamType) {
  return !ParamType->isReferenceType() ||
         ParamType.getNonReferenceType().isAtLeastAsQualifiedAs(
             ArgType.getNonReferenceType());
}

static bool isPointerOrArray(const QualType &TypeToCheck) {
  return TypeToCheck->isAnyPointerType() || TypeToCheck->isArrayType();
}

// Checks whether ArgType is an array type identical to ParamType`s array type.
// Enforces array elements` qualifier compatibility as well.
static bool isCompatibleWithArrayReference(const QualType &ArgType,
                                           const QualType &ParamType) {
  if (!ArgType->isArrayType())
    return false;
  // Here, qualifiers belong to the elements of the arrays.
  if (!ParamType.isAtLeastAsQualifiedAs(ArgType))
    return false;

  return ParamType.getUnqualifiedType() == ArgType.getUnqualifiedType();
}

static void convertToPointeeOrArrayElementQualType(QualType &TypeToConvert) {
  unsigned CVRqualifiers = 0;
  // Save array element qualifiers, since getElementType() removes qualifiers
  // from array elements.
  if (TypeToConvert->isArrayType())
    CVRqualifiers = TypeToConvert.getLocalQualifiers().getCVRQualifiers();
  TypeToConvert = TypeToConvert->isPointerType()
                      ? TypeToConvert->getPointeeType()
                      : TypeToConvert->getAsArrayTypeUnsafe()->getElementType();
  TypeToConvert = TypeToConvert.withCVRQualifiers(CVRqualifiers);
}

// Checks if multilevel pointers` qualifiers compatibility continues on the
// current pointer evel.
// For multilevel pointers, C++ permits conversion, if every cv-qualifier in
// ArgType also appears in the corresponding position in ParamType,
// and if PramType has a cv-qualifier that's not in ArgType, then every * in
// ParamType to the right
// of that cv-qualifier, except the last one, must also be const-qualified.
static bool arePointersStillQualCompatible(QualType ArgType, QualType ParamType,
                                           bool &IsParamContinuouslyConst) {
  // The types are compatible, if the parameter is at least as qualified as the
  // argument, and if it is more qualified, it has to be const on upper pointer
  // levels.
  bool AreTypesQualCompatible =
      ParamType.isAtLeastAsQualifiedAs(ArgType) &&
      (!ParamType.hasQualifiers() || IsParamContinuouslyConst);
  // Check whether the parameter's constness continues at the current pointer
  // level.
  IsParamContinuouslyConst &= ParamType.isConstQualified();

  return AreTypesQualCompatible;
}

// Checks whether multilevel pointers are compatible in terms of levels,
// qualifiers and pointee type.
static bool arePointerTypesCompatible(QualType ArgType, QualType ParamType,
                                      bool IsParamContinuouslyConst) {

  if (!arePointersStillQualCompatible(ArgType, ParamType,
                                      IsParamContinuouslyConst))
    return false;

  do {
    // Step down one pointer level.
    convertToPointeeOrArrayElementQualType(ArgType);
    convertToPointeeOrArrayElementQualType(ParamType);

    // Check whether cv-qualifiers premit compatibility on
    // current level.
    if (!arePointersStillQualCompatible(ArgType, ParamType,
                                        IsParamContinuouslyConst))
      return false;

    if (ParamType.getUnqualifiedType() == ArgType.getUnqualifiedType())
      return true;

  } while (ParamType->isAnyPointerType() && ArgType->isAnyPointerType());
  // The final type does not match, or pointer levels differ.
  return false;
}

// Checks whether ArgType converts implicitly to ParamType.
static bool areTypesCompatible(QualType ArgType, QualType ParamType,
                               const ASTContext *Ctx) {
  if (ArgType.isNull() || ParamType.isNull())
    return false;

  ArgType = ArgType.getCanonicalType();
  ParamType = ParamType.getCanonicalType();

  if (ArgType == ParamType)
    return true;

  // Check for constness and reference compatibility.
  if (!areRefAndQualCompatible(ArgType, ParamType))
    return false;

  bool IsParamReference = ParamType->isReferenceType();

  // Reference-ness has already been checked ad should be removed
  // before further checking.
  ArgType = ArgType.getNonReferenceType();
  ParamType = ParamType.getNonReferenceType();

  bool IsParamContinuouslyConst =
      !IsParamReference || ParamType.getNonReferenceType().isConstQualified();

  if (ParamType.getUnqualifiedType() == ArgType.getUnqualifiedType())
    return true;

  // Arithmetic types are interconvertible, except scoped enums.
  if (ParamType->isArithmeticType() && ArgType->isArithmeticType()) {
    if (ParamType->isEnumeralType() &&
            ParamType->getAs<EnumType>()->getDecl()->isScoped() ||
        ArgType->isEnumeralType() &&
            ArgType->getAs<EnumType>()->getDecl()->isScoped())
      return false;

    return true;
  }

  // Check if the argument and the param are both function types (the parameter
  // decayed to
  // a function pointer).
  if (ArgType->isFunctionType() && ParamType->isFunctionPointerType()) {
    ParamType = ParamType->getPointeeType();
    return ArgType == ParamType;
  }

  // Arrays or pointer arguments convert to array or pointer parameters.
  if (!(isPointerOrArray(ArgType) && isPointerOrArray(ParamType)))
    return false;

  // When ParamType is an array reference, ArgType has to be of the same sized,
  // array type with cv-compatible elements.
  if (IsParamReference && ParamType->isArrayType())
    return isCompatibleWithArrayReference(ArgType, ParamType);

  // Remove the first level of indirection.
  convertToPointeeOrArrayElementQualType(ArgType);
  convertToPointeeOrArrayElementQualType(ParamType);

  // Check qualifier compatibility on the next level.
  if (!ParamType.isAtLeastAsQualifiedAs(ArgType))
    return false;

  if (ParamType.getUnqualifiedType() == ArgType.getUnqualifiedType())
    return true;

  // Check whether ParamType and ArgType were both pointers to a class or a
  // struct, and check for inheritance.
  if (ParamType->isStructureOrClassType() && ArgType->isStructureOrClassType())
    return ArgType->getAsCXXRecordDecl()->isDerivedFrom(
        ParamType->getAsCXXRecordDecl());

  // Unless argument and param are both multilevel pointers, the types are not
  // convertible.
  if (!(ParamType->isAnyPointerType() && ArgType->isAnyPointerType()))
    return false;

  return arePointerTypesCompatible(ArgType, ParamType,
                                   IsParamContinuouslyConst);
}

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

  AbbreviationDictionary.insert(std::make_pair("ptr", "pointer"));
  AbbreviationDictionary.insert(std::make_pair("len", "length"));
  AbbreviationDictionary.insert(std::make_pair("addr", "address"));
  AbbreviationDictionary.insert(std::make_pair("arr", "array"));
  AbbreviationDictionary.insert(std::make_pair("cpy", "copy"));
  AbbreviationDictionary.insert(std::make_pair("src", "source"));
  AbbreviationDictionary.insert(std::make_pair("val", "value"));
  AbbreviationDictionary.insert(std::make_pair("ln", "line"));
  AbbreviationDictionary.insert(std::make_pair("col", "column"));
  AbbreviationDictionary.insert(std::make_pair("num", "number"));
  AbbreviationDictionary.insert(std::make_pair("nr", "number"));
  AbbreviationDictionary.insert(std::make_pair("stmt", "statement"));
  AbbreviationDictionary.insert(std::make_pair("var", "variable"));
  AbbreviationDictionary.insert(std::make_pair("vec", "vector"));
  AbbreviationDictionary.insert(std::make_pair("buf", "buffer"));
  AbbreviationDictionary.insert(std::make_pair("txt", "text"));
  AbbreviationDictionary.insert(std::make_pair("dest", "destination"));
  AbbreviationDictionary.insert(std::make_pair("elem", "element"));
  AbbreviationDictionary.insert(std::make_pair("wdth", "width"));
  AbbreviationDictionary.insert(std::make_pair("hght", "height"));
  AbbreviationDictionary.insert(std::make_pair("cnt", "count"));
  AbbreviationDictionary.insert(std::make_pair("i", "index"));
  AbbreviationDictionary.insert(std::make_pair("idx", "index"));
  AbbreviationDictionary.insert(std::make_pair("ind", "index"));
  AbbreviationDictionary.insert(std::make_pair("attr", "atttribute"));
  AbbreviationDictionary.insert(std::make_pair("pos", "position"));
  AbbreviationDictionary.insert(std::make_pair("lst", "list"));
  AbbreviationDictionary.insert(std::make_pair("str", "string"));
  AbbreviationDictionary.insert(std::make_pair("srvr", "server"));
  AbbreviationDictionary.insert(std::make_pair("clnt", "client"));

  if (Equality)
    AppliedHeuristics.push_back(Heuristic::Equality);
  if (Abbreviation)
    AppliedHeuristics.push_back(Heuristic::Abbreviation);
  if (Levenshtein)
    AppliedHeuristics.push_back(Heuristic::Levenshtein);
  if (Prefix)
    AppliedHeuristics.push_back(Heuristic::Prefix);
  if (Suffix)
    AppliedHeuristics.push_back(Heuristic::Suffix);
  if (Substring)
    AppliedHeuristics.push_back(Heuristic::Substring);
  if (JaroWinkler)
    AppliedHeuristics.push_back(Heuristic::JaroWinkler);
  if (Dice)
    AppliedHeuristics.push_back(Heuristic::Dice);
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
  // Only match calls with at least 2 arguments.
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

  // Get param attributes.
  setParamNamesAndTypes(CalleeFuncDecl);

  if (ParamNames.empty())
    return;

  // Get Arg attributes.
  // Lambda functions first Args are themselves.
  unsigned InitialArgIndex = 0;

  if (const auto *MethodDecl = dyn_cast<CXXMethodDecl>(CalleeFuncDecl)) {
    if (MethodDecl->getParent()->isLambda())
      InitialArgIndex = 1;
  }

  setArgNamesAndTypes(MatchedCallExpr, InitialArgIndex);

  if (ArgNames.empty())
    return;

  // In case of variadic functions.
  unsigned ParamCount = ParamNames.size();

  // Check similarity.
  for (unsigned i = 0; i < ParamCount; i++) {
    for (unsigned j = i + 1; j < ParamCount; j++) {
      // Do not check if param or arg names are short, or not convertible.
      if (!areParamAndArgComparable(i, j, Result.Context))
        continue;

      if (areArgsSwapped(i, j)) {
        // Warning at the function call.
        diag(MatchedCallExpr->getLocStart(), "%0 (%1) is swapped with %2 (%3).")
            << ArgNames[i] << ParamNames[i] << ArgNames[j] << ParamNames[j];

        // Note at the functions declaration.
        diag(CalleeFuncDecl->getLocStart(), "%0 is declared here:",
             DiagnosticIDs::Note)
            << CalleeFuncDecl->getNameInfo().getName().getAsString();

        return; // TODO: Address this return later
      }
    }
  }
}

void SuspiciousCallArgumentCheck::setParamNamesAndTypes(
    const FunctionDecl *CalleeFuncDecl) {
  // Reset vectors, and fill them with the currently checked function's
  // attributes.
  ParamNames.clear();
  ParamTypes.clear();

  for (unsigned i = 0, e = CalleeFuncDecl->getNumParams(); i != e; ++i) {

    const ParmVarDecl *PVD = CalleeFuncDecl->getParamDecl(i);

    ParamTypes.push_back(PVD->getType());

    IdentifierInfo *II = PVD->getIdentifier();
    if (!II)
      return;

    ParamNames.push_back(II->getName());
  }
}

void SuspiciousCallArgumentCheck::setArgNamesAndTypes(
    const CallExpr *MatchedCallExpr, unsigned InitialArgIndex) {
  // Reset vectors, and fill them with the currently checked function's
  // attributes.
  ArgNames.clear();
  ArgTypes.clear();

  for (unsigned i = InitialArgIndex, j = MatchedCallExpr->getNumArgs(); i < j;
       i++) {
    if (const auto *DeclRef = dyn_cast<DeclRefExpr>(
            MatchedCallExpr->getArg(i)->IgnoreParenImpCasts())) {
      if (const auto *Var = dyn_cast<VarDecl>(DeclRef->getDecl())) {

        ArgTypes.push_back(Var->getType());
        ArgNames.push_back(Var->getName());
        continue;
      } else if (const auto *Var = dyn_cast<FunctionDecl>(DeclRef->getDecl())) {
        ArgTypes.push_back(Var->getType());
        ArgNames.push_back(Var->getName());
        continue;
      }
    }
    ArgTypes.push_back(QualType());
    ArgNames.push_back(StringRef());
  }
}

bool SuspiciousCallArgumentCheck::areParamAndArgComparable(
    unsigned Position1, unsigned Position2, const ASTContext *Ctx) const {
  if (ArgNames[Position1].size() < 3 || ArgNames[Position2].size() < 3 ||
      ParamNames[Position1].size() < 3 || ParamNames[Position2].size() < 3)
    return false;
  if (!areTypesCompatible(ArgTypes[Position1], ParamTypes[Position2], Ctx) ||
      !areTypesCompatible(ArgTypes[Position2], ParamTypes[Position1], Ctx))
    return false;
  return true;
}

bool SuspiciousCallArgumentCheck::areArgsSwapped(unsigned Position1,
                                                 unsigned Position2) const {
  bool param1Arg2NamesSimilar =
      areNamesSimilar(ArgNames[Position2], ParamNames[Position1], Bound::Upper);
  bool param2Arg1NamesSimilar =
      areNamesSimilar(ArgNames[Position1], ParamNames[Position2], Bound::Upper);
  bool param1Arg1NamesSimilar =
      areNamesSimilar(ArgNames[Position1], ParamNames[Position1], Bound::Lower);
  bool param2Arg2NamesSimilar =
      areNamesSimilar(ArgNames[Position2], ParamNames[Position2], Bound::Lower);

  return (param1Arg2NamesSimilar || param2Arg1NamesSimilar) &&
         !param1Arg1NamesSimilar && !param2Arg2NamesSimilar;
}

bool SuspiciousCallArgumentCheck::areNamesSimilar(StringRef Arg,
                                                  StringRef Param,
                                                  Bound bound) const {

  bool areNamesSimilar = false;
  for (Heuristic Heur : AppliedHeuristics)
    switch (Heur) {
    case Heuristic::Equality:
      areNamesSimilar |= applyEqualityHeuristic(Arg, Param);
      break;
    case Heuristic::Abbreviation:
      areNamesSimilar |= applyAbbreviationHeuristic(Arg, Param);
      break;
    case Heuristic::Levenshtein:
      areNamesSimilar |= applyLevenshteinHeuristic(
          Arg, Param, bound == Bound::Upper ? LevenshteinUpperBound
                                            : LevenshteinLowerBound);
      break;
    case Heuristic::Prefix:
      areNamesSimilar |= applyPrefixHeuristic(
          Arg, Param,
          bound == Bound::Upper ? PrefixUpperBound : PrefixLowerBound);
      break;
    case Heuristic::Suffix:
      areNamesSimilar |= applySuffixHeuristic(
          Arg, Param,
          bound == Bound::Upper ? SuffixUpperBound : SuffixLowerBound);
      break;
    case Heuristic::Substring:
      areNamesSimilar |= applySubstringHeuristic(
          Arg, Param,
          bound == Bound::Upper ? SubstringUpperBound : SubstringLowerBound);
      break;
    case Heuristic::JaroWinkler:
      areNamesSimilar |= applyJaroWinklerHeuristic(
          Arg, Param, bound == Bound::Upper ? JaroWinklerUpperBound
                                            : JaroWinklerLowerBound);
      break;
    case Heuristic::Dice:
      areNamesSimilar |= applyDiceHeuristic(
          Arg, Param, bound == Bound::Upper ? DiceUpperBound : DiceLowerBound);
      break;
    default:
      break;
    }
  return areNamesSimilar;
}

} // namespace readability
} // namespace tidy
} // namespace clang
