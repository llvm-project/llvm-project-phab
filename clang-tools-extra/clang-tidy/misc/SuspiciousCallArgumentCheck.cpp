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
#include <algorithm>

using namespace clang::ast_matchers;

namespace {

// Struct to store information about the relation of params and args.
struct ComparisonInfo {
  bool Equal;
  int LevenshteinDistance;
  int Prefix;
  int Suffix;
  int ArgLength;
  int ParamLength;

  ComparisonInfo(bool Eq = false, int Ld = 32767, int Pr = 0, int Su = 0,
                 int Al = 0, int Pl = 0)
      : Equal(Eq), LevenshteinDistance(Ld), Prefix(Pr), Suffix(Su),
        ArgLength(Al), ParamLength(Pl) {}
};

bool checkSimilarity(ComparisonInfo LeftArgToRightParam,
                     ComparisonInfo LeftArgToLeftParam,
                     ComparisonInfo RightArgToRightParam,
                     ComparisonInfo RightArgToLeftParam) {

  if (LeftArgToLeftParam.Equal || RightArgToRightParam.Equal)
    return false;

  // Left to right.

  // Can't compare short arguments
  if (LeftArgToLeftParam.ArgLength >= 3) {
    // Equality.
    if (LeftArgToRightParam.Equal)
      return true;

    // Prefix, suffix.
    bool LeftSimilarToRight =
        LeftArgToRightParam.Prefix > 0 || LeftArgToRightParam.Suffix > 0;

    bool LeftSimilarToLeft =
        LeftArgToLeftParam.Prefix > 0 || LeftArgToLeftParam.Suffix > 0;

    // Levenshtein.
    bool LeftArgIsNearest = LeftArgToRightParam.LevenshteinDistance <
                                LeftArgToLeftParam.LevenshteinDistance &&
                            LeftArgToRightParam.LevenshteinDistance <
                                RightArgToRightParam.LevenshteinDistance &&
                            LeftArgToRightParam.LevenshteinDistance <
                                std::min(3, LeftArgToRightParam.ArgLength / 2);

    if ((LeftSimilarToRight || LeftArgIsNearest) && !LeftSimilarToLeft)
      return true;
  }

  // Right to left.

  // Can't compare short arguments
  if (RightArgToRightParam.ArgLength >= 3) {
    // Equality.
    if (RightArgToLeftParam.Equal)
      return true;

    // Prefix, suffix.
    bool RightSimilarToLeft =
        RightArgToLeftParam.Prefix > 0 || RightArgToLeftParam.Suffix > 0;
    bool RightSimilarToRight =
        RightArgToRightParam.Prefix > 0 || RightArgToRightParam.Suffix > 0;

    // Levenshtein.
    bool RightArgIsNearest = RightArgToLeftParam.LevenshteinDistance <
                                 LeftArgToLeftParam.LevenshteinDistance &&
                             RightArgToLeftParam.LevenshteinDistance <
                                 RightArgToRightParam.LevenshteinDistance &&
                             RightArgToLeftParam.LevenshteinDistance <
                                 std::min(3, RightArgToLeftParam.ArgLength / 2);

    if ((RightSimilarToLeft || RightArgIsNearest) && !RightSimilarToRight)
      return true;
  }

  return false;
}

int prefixMatch(StringRef Arg, StringRef Param) {

  if (!(Arg.size() >= 3 && Param.size() >= 3))
    return 0;

  StringRef Shorter = Arg.size() < Param.size() ? Arg : Param;
  StringRef Longer = Arg.size() >= Param.size() ? Arg : Param;

  if (Longer.startswith_lower(Shorter))
    return Shorter.size();

  return 0;
}

int suffixMatch(StringRef Arg, StringRef Param) {

  if (!(Arg.size() >= 3 && Param.size() >= 3))
    return 0;

  StringRef Shorter = Arg.size() < Param.size() ? Arg : Param;
  StringRef Longer = Arg.size() >= Param.size() ? Arg : Param;

  if (Longer.endswith_lower(Shorter))
    return Shorter.size();

  return 0;
}

} // namespace

namespace clang {
namespace tidy {

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
  int ExtraArgIndex = 0;

  if (const auto *MethodDecl = dyn_cast<CXXMethodDecl>(CalleeFuncDecl)) {
    if (MethodDecl->getParent()->isLambda())
      ExtraArgIndex = 1;
  }

  // Get param identifiers.
  SmallVector<StringRef, 8> ParamNames;
  for (unsigned i = 0, e = CalleeFuncDecl->getNumParams(); i != e; ++i) {
    const ParmVarDecl *PVD = CalleeFuncDecl->getParamDecl(i);
    IdentifierInfo *II = PVD->getIdentifier();
    if (!II)
      return;

    ParamNames.push_back(II->getName());
  }

  if (ParamNames.empty())
    return;

  // Get arg names.
  SmallVector<StringRef, 8> Args;
  for (int i = 0 + ExtraArgIndex, j = MatchedCallExpr->getNumArgs(); i < j;
       i++) {
    if (const auto *DeclRef = dyn_cast<DeclRefExpr>(
            MatchedCallExpr->getArg(i)->IgnoreImpCasts()->IgnoreParens())) {
      if (const auto *Var = dyn_cast<VarDecl>(DeclRef->getDecl())) {
        Args.push_back(Var->getName());
        continue;
      }
    }

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
                         Args[i].edit_distance(ParamNames[j]),
                         prefixMatch(Args[i], ParamNames[j]),
                         suffixMatch(Args[i], ParamNames[j]), Args[i].size(),
                         ParamNames[j].size()));
    }
  }

  // Check similarity.
  for (unsigned i = 0; i < MatrixSize; i++) {
    for (unsigned j = i + 1; j < MatrixSize; j++) {
      if (checkSimilarity(InfoMatrix[i][j], InfoMatrix[i][i], InfoMatrix[j][j],
                          InfoMatrix[j][i])) {
        StringRef WarningMessages[3] = {
            StringRef("%0 (%1) is swapped with %2 (%3)."),
            StringRef("literal (%1) is potentially swapped with %2 (%3)."),
            StringRef("%0 (%1) is potentially swapped with literal (%3).")};
        StringRef Message = WarningMessages[0];

        if (Args[i].empty()) {
          Message = WarningMessages[1];
        } else if (Args[j].empty()) {
          Message = WarningMessages[2];
        }

        // TODO: Underline swapped arguments.
        // Warning at the function call.
        diag(MatchedCallExpr->getLocStart(), Message.str())
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

} // namespace tidy
} // namespace clang
