//=== FloatingPointMathChecker.cpp --------------------------------*- C++
//-*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This checker evaluates calls to floating-point math functions for domain
// errors.
//
//===----------------------------------------------------------------------===//

#include "ClangSACheckers.h"
#include "clang/Basic/Builtins.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugType.h"
#include "clang/StaticAnalyzer/Core/Checker.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CallEvent.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"

using namespace clang;
using namespace ento;

namespace {
class FloatingPointMathChecker : public Checker<check::PreCall> {
  mutable std::unique_ptr<BuiltinBug> BT;

  void reportError(ProgramStateRef State, CheckerContext &C) const;

  void inline handleAssumption(ProgramStateRef T, ProgramStateRef F,
                               ProgramStateRef State, CheckerContext &C) const;

public:
  void checkPreCall(const CallEvent &Call, CheckerContext &C) const;
};

} // end anonymous namespace

void FloatingPointMathChecker::reportError(ProgramStateRef State,
                                           CheckerContext &C) const {
  if (ExplodedNode *N = C.generateNonFatalErrorNode(State)) {
    if (!BT)
      BT.reset(new BuiltinBug(this, "Domain/range error"));
    C.emitReport(llvm::make_unique<BugReport>(
        *BT, "Argument value is out of valid domain for function call", N));
  }
}

void FloatingPointMathChecker::handleAssumption(ProgramStateRef T,
                                                ProgramStateRef F,
                                                ProgramStateRef State,
                                                CheckerContext &C) const {
  if (F) {
    reportError(State, C);

    if (!T)
      C.addTransition(F);
  }

  if (!F && T)
    C.addTransition(T);
}

void FloatingPointMathChecker::checkPreCall(const CallEvent &Call,
                                            CheckerContext &C) const {
  ProgramStateRef State = C.getState();
  const Decl *D = Call.getDecl();
  if (!D)
    return;
  const FunctionDecl *FD = dyn_cast<FunctionDecl>(D);
  if (!FD)
    return;

  ConstraintManager &CM = C.getConstraintManager();
  SValBuilder &SVB = C.getSValBuilder();
  BasicValueFactory &BVF = SVB.getBasicValueFactory();
  ASTContext &Ctx = SVB.getContext();
  QualType Int32Ty = Ctx.getIntTypeForBitwidth(32, true);

  switch (FD->getBuiltinID()) {
  default:
    return;

  // acos(x): -1 <= x <= 1
  case Builtin::BIacos:
  case Builtin::BIacosf:
  case Builtin::BIacosl:
  case Builtin::BIasin:
  case Builtin::BIasinf:
  case Builtin::BIasinl:
  case Builtin::BI__builtin_acos:
  case Builtin::BI__builtin_acosf:
  case Builtin::BI__builtin_acosl:
  case Builtin::BI__builtin_asin:
  case Builtin::BI__builtin_asinf:
  case Builtin::BI__builtin_asinl: {
    assert(Call.getNumArgs() == 1);
    SVal V = Call.getArgSVal(0);
    llvm::APSInt From = BVF.getValue(-1, Int32Ty);
    llvm::APSInt To = BVF.getValue(1, Int32Ty);

    // Skip if known to be NaN, otherwise assume to be not NaN
    SVal notNaN = SVB.evalBinOp(State, BO_EQ, V, V, SVB.getConditionType());
    if (notNaN.isUnknown() || notNaN.isZeroConstant()) {
      return;
    }

    ProgramStateRef StIR, StOR;
    // This relies on the constraint manager promoting from APSInt to APFloat
    // because the type of V is floating-point.
    std::tie(StIR, StOR) =
        CM.assumeInclusiveRangeDual(State, V.castAs<NonLoc>(), From, To);
    return handleAssumption(StIR, StOR, State, C);
  }

  // acosh(x): x >= 1
  case Builtin::BIacosh:
  case Builtin::BIacoshf:
  case Builtin::BIacoshl:
  case Builtin::BI__builtin_acosh:
  case Builtin::BI__builtin_acoshf:
  case Builtin::BI__builtin_acoshl: {
    assert(Call.getNumArgs() == 1);
    SVal V = Call.getArgSVal(0);
    llvm::APFloat From = BVF.getValue(
        1, Ctx.getFloatTypeSemantics(Call.getArgExpr(0)->getType()));
    DefinedOrUnknownSVal SV = SVB.makeFloatVal(From);

    SVal notNaN = SVB.evalBinOp(State, BO_EQ, V, V, SVB.getConditionType());
    if (notNaN.isUnknown() || notNaN.isZeroConstant()) {
      return;
    }

    SVal isGE = SVB.evalBinOp(State, BO_GE, V, SV, SVB.getConditionType());
    if (isGE.isZeroConstant())
      return reportError(State, C);

    ProgramStateRef StT, StF;
    std::tie(StT, StF) = CM.assumeDual(State, isGE.castAs<DefinedSVal>());
    return handleAssumption(StT, StF, State, C);
  }

  // atanh(x): -1 < x < 1
  case Builtin::BIatanh:
  case Builtin::BIatanhf:
  case Builtin::BIatanhl:
  case Builtin::BI__builtin_atanh:
  case Builtin::BI__builtin_atanhf:
  case Builtin::BI__builtin_atanhl: {
    assert(Call.getNumArgs() == 1);
    SVal V = Call.getArgSVal(0);
    llvm::APFloat From = BVF.getValue(
        -1, Ctx.getFloatTypeSemantics(Call.getArgExpr(0)->getType()));
    llvm::APFloat To = BVF.getValue(
        1, Ctx.getFloatTypeSemantics(Call.getArgExpr(0)->getType()));

    SVal notNaN = SVB.evalBinOp(State, BO_EQ, V, V, SVB.getConditionType());
    if (notNaN.isUnknown() || notNaN.isZeroConstant()) {
      return;
    }

    DefinedOrUnknownSVal SFrom = SVB.makeFloatVal(From);
    SVal isGT = SVB.evalBinOp(State, BO_GT, V, SFrom, SVB.getConditionType());
    if (isGT.isZeroConstant())
      return reportError(State, C);

    DefinedOrUnknownSVal STo = SVB.makeFloatVal(To);
    SVal isLT = SVB.evalBinOp(State, BO_LT, V, STo, SVB.getConditionType());
    if (isGT.isConstant(1) && isLT.isZeroConstant())
      return reportError(State, C);

    // TODO: FIXME
    // This should be BO_LAnd, but logical operations are handled much earlier
    // during ExplodedGraph generation. However, since both sides are
    // Boolean/Int1Ty, we can use bitwise and.
    // If/when this is fixed, also remove the explicit short-circuits above.
    SVal isIR =
        SVB.evalBinOp(State, BO_And, isGT, isLT, SVB.getConditionType());
    if (isIR.isZeroConstant())
      return reportError(State, C);

    ProgramStateRef StIR, StOR;
    std::tie(StIR, StOR) = CM.assumeDual(State, isIR.castAs<DefinedSVal>());
    return handleAssumption(StIR, StOR, State, C);
  }

  // log(x): x >= 0
  case Builtin::BIlog:
  case Builtin::BIlogf:
  case Builtin::BIlogl:
  case Builtin::BIlog2:
  case Builtin::BIlog2f:
  case Builtin::BIlog2l:
  case Builtin::BIlog10:
  case Builtin::BIlog10f:
  case Builtin::BIlog10l:
  case Builtin::BIsqrt:
  case Builtin::BIsqrtf:
  case Builtin::BIsqrtl:
  case Builtin::BI__builtin_log:
  case Builtin::BI__builtin_logf:
  case Builtin::BI__builtin_logl:
  case Builtin::BI__builtin_log2:
  case Builtin::BI__builtin_log2f:
  case Builtin::BI__builtin_log2l:
  case Builtin::BI__builtin_log10:
  case Builtin::BI__builtin_log10f:
  case Builtin::BI__builtin_log10l:
  case Builtin::BI__builtin_sqrt:
  case Builtin::BI__builtin_sqrtf:
  case Builtin::BI__builtin_sqrtl: {
    assert(Call.getNumArgs() == 1);
    SVal V = Call.getArgSVal(0);
    llvm::APFloat From = BVF.getValue(
        0, Ctx.getFloatTypeSemantics(Call.getArgExpr(0)->getType()));
    DefinedOrUnknownSVal SV = SVB.makeFloatVal(From);

    SVal notNaN = SVB.evalBinOp(State, BO_EQ, V, V, SVB.getConditionType());
    if (notNaN.isUnknown() || notNaN.isZeroConstant()) {
      return;
    }

    SVal isGE = SVB.evalBinOp(State, BO_GE, V, SV, SVB.getConditionType());
    if (isGE.isZeroConstant())
      return reportError(State, C);

    ProgramStateRef StT, StF;
    std::tie(StT, StF) = CM.assumeDual(State, isGE.castAs<DefinedSVal>());
    return handleAssumption(StT, StF, State, C);
  }

  // log1p(x): x >= -1
  case Builtin::BIlog1p:
  case Builtin::BIlog1pf:
  case Builtin::BIlog1pl:
  case Builtin::BI__builtin_log1p:
  case Builtin::BI__builtin_log1pf:
  case Builtin::BI__builtin_log1pl: {
    assert(Call.getNumArgs() == 1);
    SVal V = Call.getArgSVal(0);
    llvm::APFloat From = BVF.getValue(
        -1, Ctx.getFloatTypeSemantics(Call.getArgExpr(0)->getType()));
    DefinedOrUnknownSVal SV = SVB.makeFloatVal(From);

    SVal notNaN = SVB.evalBinOp(State, BO_EQ, V, V, SVB.getConditionType());
    if (notNaN.isUnknown() || notNaN.isZeroConstant()) {
      return;
    }

    SVal isGE = SVB.evalBinOp(State, BO_GE, V, SV, SVB.getConditionType());
    if (isGE.isZeroConstant())
      return reportError(State, C);

    ProgramStateRef StT, StF;
    std::tie(StT, StF) = CM.assumeDual(State, isGE.castAs<DefinedSVal>());
    return handleAssumption(StT, StF, State, C);
  }

  // logb(x): x != 0
  case Builtin::BIlogb:
  case Builtin::BIlogbf:
  case Builtin::BIlogbl:
  case Builtin::BI__builtin_logb:
  case Builtin::BI__builtin_logbf:
  case Builtin::BI__builtin_logbl: {
    assert(Call.getNumArgs() == 1);
    SVal V = Call.getArgSVal(0);
    llvm::APFloat From = BVF.getValue(
        0, Ctx.getFloatTypeSemantics(Call.getArgExpr(0)->getType()));
    DefinedOrUnknownSVal SV = SVB.makeFloatVal(From);

    SVal isNE = SVB.evalBinOp(State, BO_NE, V, SV, SVB.getConditionType());
    if (isNE.isZeroConstant())
      return reportError(State, C);

    ProgramStateRef StT, StF;
    std::tie(StT, StF) = CM.assumeDual(State, isNE.castAs<DefinedSVal>());
    return handleAssumption(StT, StF, State, C);
  }

  // TODO:
  // pow(x, y) : x > 0 || (x == 0 && y > 0) || (x < 0 && (int) y)
  // case Builtin::BIpow:
  // case Builtin::BIpowf:
  // case Builtin::BIpowl:
  // case Builtin::BI__builtin_pow:
  // case Builtin::BI__builtin_powf:
  // case Builtin::BI__builtin_powl:

  // TODO:
  // lgamma(x) : x != 0 && !(x < 0 && (int) x)
  // case Builtin::BIlgamma:
  // case Builtin::BIlgammaf:
  // case Builtin::BIlgammal:
  // case Builtin::BItgamma:
  // case Builtin::BItgammaf:
  // case Builtin::BItgammal:
  // case Builtin::BI__builtin_lgamma:
  // case Builtin::BI__builtin_lgammaf:
  // case Builtin::BI__builtin_lgammal:
  // case Builtin::BI__builtin_tgamma:
  // case Builtin::BI__builtin_tgammaf:
  // case Builtin::BI__builtin_tgammal:

  // fmod(x,y) : y != 0
  case Builtin::BIfmod:
  case Builtin::BIfmodf:
  case Builtin::BIfmodl:
  case Builtin::BIremainder:
  case Builtin::BIremainderf:
  case Builtin::BIremainderl:
  case Builtin::BI__builtin_fmod:
  case Builtin::BI__builtin_fmodf:
  case Builtin::BI__builtin_fmodl:
  case Builtin::BI__builtin_remainder:
  case Builtin::BI__builtin_remainderf:
  case Builtin::BI__builtin_remainderl:
  case Builtin::BI__builtin_remquo:
  case Builtin::BI__builtin_remquof:
  case Builtin::BI__builtin_remquol: {
    assert(Call.getNumArgs() >= 2);
    SVal V = Call.getArgSVal(1);
    llvm::APFloat From = BVF.getValue(
        0, Ctx.getFloatTypeSemantics(Call.getArgExpr(1)->getType()));
    DefinedOrUnknownSVal SV = SVB.makeFloatVal(From);

    SVal isNE = SVB.evalBinOp(State, BO_NE, V, SV, SVB.getConditionType());
    if (isNE.isZeroConstant())
      return reportError(State, C);

    ProgramStateRef StT, StF;
    std::tie(StT, StF) = CM.assumeDual(State, isNE.castAs<DefinedSVal>());
    return handleAssumption(StT, StF, State, C);
  }
  }
}

void ento::registerFloatingPointMathChecker(CheckerManager &mgr) {
  mgr.registerChecker<FloatingPointMathChecker>();
}
