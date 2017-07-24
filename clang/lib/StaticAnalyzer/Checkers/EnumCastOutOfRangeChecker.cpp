//===- EnumCastOutOfRangeChecker.cpp ---------------------------*- C++ -*--===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// The EnumCastOutOfRangeChecker is responsible for checking integer to
// enumeration casts that could result in undefined values. This could happen
// if the value that we cast from is out of the value range of the enumeration.
// Reference:
// [ISO/IEC 14882-2014] ISO/IEC 14882-2014.
//   Programming Languages — C++, Fourth Edition. 2014.
// C++ Standard, [dcl.enum], in paragraph 8, which defines the range of an enum
// C++ Standard, [expr.static.cast], paragraph 10, which defines the behaviour
//   of casting an integer value that is out of range
//===----------------------------------------------------------------------===//

#include "ClangSACheckers.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugType.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"

using namespace clang;
using namespace ento;

namespace {
// This evaluator checks 2 SVals for equality. The first SVal is provided via
// the constructor, the second is the parameter of the overloaded () operator.
// It uses the in-built ConstraintManager to resolve the equlity to possible or
// not possible ProgramStates.
class ConstraintBasedEQEvaluator {
private:
  const DefinedOrUnknownSVal CompareValue;

  const ProgramStateRef PS;
  SValBuilder &SVB;

public:
  ConstraintBasedEQEvaluator(CheckerContext &C,
                             const DefinedOrUnknownSVal CompareValue)
      : CompareValue(CompareValue), PS(C.getState()), SVB(C.getSValBuilder()) {}

  bool operator()(const llvm::APSInt &EnumDeclInitValue) {
    DefinedOrUnknownSVal EnumDeclValue = SVB.makeIntVal(EnumDeclInitValue);
    const auto ElemEqualsValueToCast =
        SVB.evalEQ(PS, EnumDeclValue, CompareValue);

    return static_cast<bool>(PS->assume(ElemEqualsValueToCast, true));
  }
};

// This checker checks CastExpr statements.
// If the value provided to the cast is one of the values the enumeration can
// represent, the said value matches the enumeration. If the checker can
// establish the impossibility of matching it gives a warning.
// Being conservative, it does not warn if there is slight possibility the
// value can be matching.
class EnumCastOutOfRangeChecker : public Checker<check::PreStmt<CastExpr>> {
private:
  mutable std::unique_ptr<BuiltinBug> EnumValueCastOutOfRange;
  void reportWarning(CheckerContext &C) const;

public:
  void checkPreStmt(const CastExpr *CE, CheckerContext &C) const;
};

typedef typename llvm::SmallVector<llvm::APSInt, 6> EnumValueVector;

// Collects all of the values an enum can represent (as SVals).
EnumValueVector getDeclValuesForEnum(const EnumDecl *ED) {
  EnumValueVector DeclValues;
  for (const auto *D : ED->decls()) {
    const auto ECD = dyn_cast<EnumConstantDecl>(D);
    DeclValues.push_back(ECD->getInitVal());
  }

  return DeclValues;
}
} // namespace

void EnumCastOutOfRangeChecker::reportWarning(CheckerContext &C) const {
  if (auto N = C.generateNonFatalErrorNode(C.getState())) {
    if (!EnumValueCastOutOfRange)
      EnumValueCastOutOfRange.reset(
          new BuiltinBug(this, "Enum cast out of range",
                         "The value provided to the cast expression is not in "
                         "the valid range of values for the enum."));
    C.emitReport(llvm::make_unique<BugReport>(
        *EnumValueCastOutOfRange, EnumValueCastOutOfRange->getDescription(),
        N));
  }
}

void EnumCastOutOfRangeChecker::checkPreStmt(const CastExpr *CE,
                                             CheckerContext &C) const {
  // Get the value of the expression to cast.
  const auto ValueToCastOptional =
      C.getSVal(CE->getSubExpr()).getAs<DefinedOrUnknownSVal>();

  // If the value cannot be reasoned about (not even a DefinedOrUnknownSVal),
  // don't analyze further.
  if (!ValueToCastOptional)
    return;

  const QualType T = CE->getType();
  // Check whether the cast type is an enum.
  if (!T->isEnumeralType())
    return;

  // If the cast is an enum, get its declaration.
  // If the isEnumeralType() returned true, then the declaration must exist
  // even if it is a stub declaration. It is up to the getDeclValuesForEnum()
  // function to handle this.
  const EnumDecl *ED = T->getAs<EnumType>()->getDecl();

  EnumValueVector DeclValues = getDeclValuesForEnum(ED);
  // Check if any of the enum values possibly match.
  bool PossibleValueMatch =
      std::any_of(DeclValues.begin(), DeclValues.end(),
                  ConstraintBasedEQEvaluator(C, *ValueToCastOptional));

  // If there is no value that can possibly match any of the enum values, then
  // warn.
  if (!PossibleValueMatch)
    reportWarning(C);
}

void ento::registerEnumCastOutOfRangeChecker(CheckerManager &mgr) {
  mgr.registerChecker<EnumCastOutOfRangeChecker>();
}
