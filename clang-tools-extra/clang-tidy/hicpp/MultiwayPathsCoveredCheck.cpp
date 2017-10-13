//===--- MultiwayPathsCoveredCheck.cpp - clang-tidy------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "MultiwayPathsCoveredCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

#include <iostream>

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace hicpp {

void MultiwayPathsCoveredCheck::storeOptions(
    ClangTidyOptions::OptionMap &Opts) {
  Options.store(Opts, "WarnOnMissingElse", WarnOnMissingElse);
}

void MultiwayPathsCoveredCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(
      stmt(allOf(anyOf(switchStmt(forEachSwitchCase(defaultStmt()))
                           .bind("switch-default"),
                       switchStmt(unless(hasDescendant(switchCase())))
                           .bind("degenerate-switch"),
                       // This matcher must be the last one of the three,
                       // otherwise the cases "switch-default" and
                       // "degenerate-switch" are not found correctly.
                       switchStmt(forEachSwitchCase(unless(defaultStmt())))
                           .bind("switch-no-default")),
                 switchStmt(hasCondition(allOf(
                     // Match on switch stmt that have integer conditions
                     // (everything, that is not an enum). Here again, the
                     // ordering is important. Otherwise the matching fails.
                     hasDescendant(declRefExpr().bind("non-enum-condition")),
                     unless(hasDescendant(declRefExpr(hasType(enumType()))
                                              .bind("enum-condition")))))))),
      this);

  /// This option is noisy, therefore matching is configurable.
  if (WarnOnMissingElse) {
    Finder->addMatcher(
        ifStmt(allOf(hasParent(ifStmt()), unless(hasElse(anything()))))
            .bind("else-if"),
        this);
  }
}

namespace {
unsigned countCaseLabels(const SwitchStmt *Switch) {
  unsigned CaseCount = 0;

  const SwitchCase *CurrentCase = Switch->getSwitchCaseList();
  while (CurrentCase) {
    ++CaseCount;
    CurrentCase = CurrentCase->getNextSwitchCase();
  }

  return CaseCount;
}
#if 0
/// Get the number of different values for the Type T, that is switched on.
std::size_t getNumberOfPossibleValues(const Type *T,
                                      const ASTContext &Context) {
  // This Assertion fails in clang and llvm, when matching on enum types.
  assert(T->isIntegralType(Context) &&
         "expecting integral type for discrete set of values");

  return std::numeric_limits<std::size_t>::max();
}
#endif
} // namespace

void MultiwayPathsCoveredCheck::check(const MatchFinder::MatchResult &Result) {
  if (const auto *ElseIfWithoutElse =
          Result.Nodes.getNodeAs<IfStmt>("else-if")) {
    diag(ElseIfWithoutElse->getLocStart(),
         "potential uncovered codepath found; add an ending else branch");
    return;
  }

  if (const auto *SwitchWithDefault =
          Result.Nodes.getNodeAs<SwitchStmt>("switch-default")) {
    const unsigned CaseCount = countCaseLabels(SwitchWithDefault);
    assert(CaseCount > 0 && "Switch stmt with supposedly one default branch "
                            "did not contain case labels");

    // Only the default branch (we explicitly matched for default!) exists.
    if (CaseCount == 1) {
      diag(SwitchWithDefault->getLocStart(),
           "degenerated switch stmt found; only the default branch exists");
      // FIXME: The condition and everything could be removed, since default
      // will be always executed.
    }
    // One case label and a default branch exists.
    else if (CaseCount == 2) {
      diag(SwitchWithDefault->getLocStart(),
           "switch case could be better written with single if stmt");
      // FIXME: Rewrite the switch into an if-stmt.
      // The case lands in
      // if ((condition) == caseLabel) { stmt... }
      // else { default_stmts }
    }
    // Multiply cases and a default branch exist, therefore everything is
    // alright with the switch stmt.
    else {
      return;
    }
    return;
  }

  if (const auto *SwitchWithoutDefault =
          Result.Nodes.getNodeAs<SwitchStmt>("switch-no-default")) {
    // The matcher only works, because some nodes are explicitly matched and
    // bound, but ignored. This is necessary, to build the excluding logic for
    // enums and switch stmts without a default branch.
    assert(!Result.Nodes.getNodeAs<DeclRefExpr>("enum-condition") &&
           "switch over enum is handled by warnings already, explicitly ignore "
           "them");
    assert(!Result.Nodes.getNodeAs<SwitchStmt>("switch-default") &&
           "switch stmts with default branch do cover all paths, explicitly "
           "ignore them");

    const auto *IntegerCondition =
        Result.Nodes.getNodeAs<DeclRefExpr>("non-enum-condition");
    assert(IntegerCondition && "Did not find pure integer condition, but did "
                               "match explicitly only for integers");
#if 0
    // Determining the correct number of values currently not working.
    const std::size_t MaxPathsPossible = getNumberOfPossibleValues(
        IntegerCondition->getType().getCanonicalType().split().Ty,
        *Result.Context);
#endif

    // Check if all paths were covered explicitly.
    const unsigned CaseCount = countCaseLabels(SwitchWithoutDefault);

    // CaseCount == 0 is caught in DegenerateSwitch. Necessary because the
    // matcher used for here does not match on degenerate stmt
    assert(CaseCount > 0 && "Switch stmt without any case found. This case "
                            "should be excluded by the matcher and is handled "
                            "seperatly");

    // Should be written as an IfStmt.
    if (CaseCount == 1) {
      diag(SwitchWithoutDefault->getLocStart(), "switch stmt with only one "
                                                "case found; this can be "
                                                "better written in an if-stmt");
      // FIXME: Automatically transform the switch into an if.
    }
    // Missed some value explicity, therefore add a default branch.
    else if (CaseCount >= 2 /* && CaseCount < MaxPathsPossible*/) {
      diag(SwitchWithoutDefault->getLocStart(),
           "potential uncovered codepath found; add a default branch");
      // FIXME: Automatically add 'default: break;' after the last case.
    }
    // All paths were explicitly covered, therefore no default branch is
    // necessary.
    else {
      return;
    }

    return;
  }

  if (const auto *DegenerateSwitch =
          Result.Nodes.getNodeAs<SwitchStmt>("degenerate-switch")) {
    diag(DegenerateSwitch->getLocStart(), "degenerate switch statement without "
                                          "any cases found; add cases and a "
                                          "default branch");
    // FIXME: Removing would be possible? But i think it should not be removed,
    // since it is most likely an error.
    return;
  }

  llvm_unreachable("matched a case, that was not explicitly handled");
}

} // namespace hicpp
} // namespace tidy
} // namespace clang
