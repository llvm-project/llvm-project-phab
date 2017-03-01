//== ArrayBoundChecker.cpp ------------------------------*- C++ -*--==//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines ArrayBoundChecker, which is a path-sensitive check
// which looks for an out-of-bound array element access.
//
//===----------------------------------------------------------------------===//

#include "ClangSACheckers.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugType.h"
#include "clang/StaticAnalyzer/Core/Checker.h"
#include "clang/StaticAnalyzer/Core/CheckerManager.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/ExprEngine.h"

using namespace clang;
using namespace ento;

namespace {
class ArrayBoundChecker :
    public Checker<check::Location, check::PreStmt<ArraySubscriptExpr>> {
  mutable std::unique_ptr<BuiltinBug> BT;
  mutable std::unique_ptr<BuiltinBug> BT_VLA;

public:
  void checkLocation(SVal l, bool isLoad, const Stmt* S,
                     CheckerContext &C) const;
  void checkPreStmt(const ArraySubscriptExpr *A, CheckerContext &C) const;
};
}

void ArrayBoundChecker::checkLocation(SVal l, bool isLoad, const Stmt* LoadS,
                                      CheckerContext &C) const {

  // Check for out of bound array element access.
  const MemRegion *R = l.getAsRegion();
  if (!R)
    return;

  const ElementRegion *ER = dyn_cast<ElementRegion>(R);
  if (!ER)
    return;

  // Get the index of the accessed element.
  DefinedOrUnknownSVal Idx = ER->getIndex().castAs<DefinedOrUnknownSVal>();

  // Zero index is always in bound, this also passes ElementRegions created for
  // pointer casts.
  if (Idx.isZeroConstant())
    return;

  ProgramStateRef state = C.getState();

  // Get the size of the array.
  DefinedOrUnknownSVal NumElements
    = C.getStoreManager().getSizeInElements(state, ER->getSuperRegion(),
                                            ER->getValueType());

  ProgramStateRef StInBound = state->assumeInBound(Idx, NumElements, true);
  ProgramStateRef StOutBound = state->assumeInBound(Idx, NumElements, false);
  if (StOutBound && !StInBound) {
    ExplodedNode *N = C.generateErrorNode(StOutBound);
    if (!N)
      return;

    if (!BT)
      BT.reset(new BuiltinBug(
          this, "Out-of-bound array access",
          "Access out-of-bound array element (buffer overflow)"));

    // FIXME: It would be nice to eventually make this diagnostic more clear,
    // e.g., by referencing the original declaration or by saying *why* this
    // reference is outside the range.

    // Generate a report for this bug.
    auto report = llvm::make_unique<BugReport>(*BT, BT->getDescription(), N);

    report->addRange(LoadS->getSourceRange());
    C.emitReport(std::move(report));
    return;
  }

  // Array bound check succeeded.  From this point forward the array bound
  // should always succeed.
  C.addTransition(StInBound);
}

void ArrayBoundChecker::checkPreStmt(const ArraySubscriptExpr *A, CheckerContext &C) const
{
  const Expr *Base = A->getBase()->IgnoreImpCasts();

  ASTContext &Ctx = C.getASTContext();
  const VariableArrayType *VLA = Ctx.getAsVariableArrayType(Base->getType());
  if (!VLA)
    return;

  ProgramStateRef State = C.getState();
  SVal sizeV = State->getSVal(VLA->getSizeExpr(), C.getLocationContext());
  SVal idxV = State->getSVal(A->getIdx(), C.getLocationContext());

  // Is idx greater than size?
  SValBuilder &Bldr = C.getSValBuilder();
  SVal GE = Bldr.evalBinOp(State, BO_GE, idxV, sizeV, Bldr.getConditionType());
  if (!GE.isConstant(1))
    return;

  ExplodedNode *N = C.generateErrorNode(State);
  if (!N)
    return;

  if (!BT_VLA)
    BT_VLA.reset(new BuiltinBug(
      this, "Out-of-bound VLA access",
      "Out-of-bounds VLA access (symbolically this index is greater than the size)"));
  // Generate a report for this bug.
  auto report = llvm::make_unique<BugReport>(*BT_VLA, BT_VLA->getDescription(), N);
  report->addRange(A->getSourceRange());
  C.emitReport(std::move(report));
}

void ento::registerArrayBoundChecker(CheckerManager &mgr) {
  mgr.registerChecker<ArrayBoundChecker>();
}
