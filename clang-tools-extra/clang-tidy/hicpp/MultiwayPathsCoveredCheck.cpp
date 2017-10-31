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

#include <iostream>
#include <limits>

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
      stmt(allOf(
          anyOf(switchStmt(forEachSwitchCase(defaultStmt()))
                    .bind("switch-default"),
                switchStmt(unless(hasDescendant(switchCase())))
                    .bind("degenerate-switch"),
                // This matcher must be the last one of the three
                // 'switchStmt' options.
                // Otherwise the cases 'switch-default' and
                // 'degenerate-switch' are not found correctly.
                switchStmt(forEachSwitchCase(unless(defaultStmt())))
                    .bind("switch-no-default")),
          switchStmt(hasCondition(allOf(
              // Match on switch statements that have either bitfield or integer
              // condition.
              // The ordering in 'anyOf()' is important, since the last
              // condition is the most general.
              anyOf(ignoringImpCasts(memberExpr(hasDeclaration(
                        fieldDecl(isBitField()).bind("bitfield")))),
                    hasDescendant(declRefExpr().bind("non-enum-condition"))),
              // 'unless()' must be the last match here and must be binded,
              // otherwise the matcher does not work correctly.
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
/// This function calculate 2 ** Bits and returns
/// numeric_limits<std::size_t>::max() if an overflow occured.
std::size_t twoPow(std::size_t Bits) {
  const std::size_t DiscreteValues = 1ul << Bits;

  return Bits > 0 && DiscreteValues <= 1
             ? std::numeric_limits<std::size_t>::max()
             : DiscreteValues;
}
/// Get the number of different values for the Type T, that is switched on.
///
/// \return - 0 if bitcount could not be determined
///         - numeric_limits<std::size_t>::max() when overflow appeared due to
///           more then 64 bits type size.
std::size_t getNumberOfPossibleValues(QualType T, const ASTContext &Context) {
  // Context.getTypeSize(T) returns the number of bits T uses.
  // Calculates the number of discrete values that are representable by this
  // type.
  return T->isIntegralType(Context) ? twoPow(Context.getTypeSize(T))
                                    : twoPow(0ul);
}
} // namespace

void MultiwayPathsCoveredCheck::check(const MatchFinder::MatchResult &Result) {
  if (const auto *ElseIfWithoutElse =
          Result.Nodes.getNodeAs<IfStmt>("else-if")) {
    diag(ElseIfWithoutElse->getLocStart(),
         "potential uncovered codepath; add an ending else branch");
    return;
  }
  // Checks the sanity of 'switch' statements that actually do define
  // a default branch but might be degenerated by having no or only one case.
  if (const auto *SwitchWithDefault =
          Result.Nodes.getNodeAs<SwitchStmt>("switch-default")) {
    handleSwitchWithDefault(SwitchWithDefault);
    return;
  }
  // Checks all 'switch' statements that do not define a default label.
  // Here the heavy lifting happens.
  if (const auto *SwitchWithoutDefault =
          Result.Nodes.getNodeAs<SwitchStmt>("switch-no-default")) {
    handleSwitchWithoutDefault(SwitchWithoutDefault, Result);
    return;
  }
  // Warns for degenerated 'switch' statements that neither define a case nor
  // a default label.
  if (const auto *DegenerateSwitch =
          Result.Nodes.getNodeAs<SwitchStmt>("degenerate-switch")) {
    diag(DegenerateSwitch->getLocStart(), "degenerated switch without labels");
    return;
  }
  llvm_unreachable("matched a case, that was not explicitly handled");
}

void MultiwayPathsCoveredCheck::handleSwitchWithDefault(
    const SwitchStmt *Switch) {
  const unsigned CaseCount = countCaseLabels(Switch);
  assert(CaseCount > 0 && "Switch statementt with supposedly one default "
                          "branch did not contain any case labels");
  if (CaseCount == 1 || CaseCount == 2)
    diag(Switch->getLocStart(),
         CaseCount == 1
             ? "degenerated switch with default label only"
             : "switch could be better written as if/else statement");
}

void MultiwayPathsCoveredCheck::handleSwitchWithoutDefault(
    const SwitchStmt *Switch, const MatchFinder::MatchResult &Result) {
  // The matcher only works, because some nodes are explicitly matched and
  // bound, but ignored. This is necessary, to build the excluding logic for
  // enums and 'switch' statements without a 'default' branch.
  assert(!Result.Nodes.getNodeAs<DeclRefExpr>("enum-condition") &&
         "switch over enum is handled by warnings already, explicitly ignoring "
         "them");
  assert(!Result.Nodes.getNodeAs<SwitchStmt>("switch-default") &&
         "switch stmts with default branch do cover all paths, explicitly "
         "ignoring them");
  // Determine the number of case labels. Since 'default' is not present
  // and duplicating case labels is not allowed this number represents
  // the number of codepaths. It can be directly compared to 'MaxPathsPossible'
  // to see if some cases are missing.
  const unsigned CaseCount = countCaseLabels(Switch);
  // CaseCount == 0 is caught in DegenerateSwitch. Necessary because the
  // matcher used for here does not match on degenerate 'switch'.
  assert(CaseCount > 0 && "Switch statement without any case found. This case "
                          "should be excluded by the matcher and is handled "
                          "seperatly");
  const std::size_t MaxPathsPossible = [&]() {
    if (const auto *IntegerCondition =
            Result.Nodes.getNodeAs<DeclRefExpr>("non-enum-condition"))
      return getNumberOfPossibleValues(IntegerCondition->getType(),
                                       *Result.Context);
    if (const auto *BitfieldDecl =
            Result.Nodes.getNodeAs<FieldDecl>("bitfield"))
      return twoPow(BitfieldDecl->getBitWidthValue(*Result.Context));
    llvm_unreachable("either bitfield or non-enum must be condition");
  }();

  // FIXME: Transform the 'switch' into an 'if' for CaseCount == 1.
  if (CaseCount < MaxPathsPossible)
    diag(Switch->getLocStart(),
         CaseCount == 1 ? "switch with only one case; use if statement"
                        : "potential uncovered codepath; add a default case");
}
} // namespace hicpp
} // namespace tidy
} // namespace clang
