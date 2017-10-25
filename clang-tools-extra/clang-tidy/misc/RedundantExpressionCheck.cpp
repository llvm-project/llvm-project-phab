//===--- RedundantExpressionCheck.cpp - clang-tidy-------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "RedundantExpressionCheck.h"
#include "../utils/Matchers.h"
#include "../utils/OptionsUtils.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Basic/LLVM.h"
#include "clang/Basic/SourceLocation.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Lex/Lexer.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/FoldingSet.h"
#include "llvm/Support/Casting.h"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

using namespace clang::ast_matchers;
using namespace clang::tidy::matchers;

namespace clang {
namespace tidy {
namespace misc {

namespace {
using llvm::APSInt;
} // namespace

static const llvm::StringSet<> KnownBannedMacroNames = {"EAGAIN", "EWOULDBLOCK",
                                                        "SIGCLD", "SIGCHLD"};

static bool incrementWithoutOverflow(const APSInt &Value, APSInt &Result) {
  Result = Value;
  ++Result;
  return Value < Result;
}

static bool areEquivalentNameSpecifier(const NestedNameSpecifier *Left,
                                       const NestedNameSpecifier *Right) {
  llvm::FoldingSetNodeID LeftID, RightID;
  Left->Profile(LeftID);
  Right->Profile(RightID);
  return LeftID == RightID;
}

static bool areEquivalentExpr(const Expr *Left, const Expr *Right) {
  if (!Left || !Right)
    return !Left && !Right;

  Left = Left->IgnoreParens();
  Right = Right->IgnoreParens();

  // Compare classes.
  if (Left->getStmtClass() != Right->getStmtClass())
    return false;

  // Compare children.
  Expr::const_child_iterator LeftIter = Left->child_begin();
  Expr::const_child_iterator RightIter = Right->child_begin();
  while (LeftIter != Left->child_end() && RightIter != Right->child_end()) {
    if (!areEquivalentExpr(dyn_cast<Expr>(*LeftIter),
                           dyn_cast<Expr>(*RightIter)))
      return false;
    ++LeftIter;
    ++RightIter;
  }
  if (LeftIter != Left->child_end() || RightIter != Right->child_end())
    return false;

  // Perform extra checks.
  switch (Left->getStmtClass()) {
  default:
    return false;

  case Stmt::CharacterLiteralClass:
    return cast<CharacterLiteral>(Left)->getValue() ==
           cast<CharacterLiteral>(Right)->getValue();

  case Stmt::IntegerLiteralClass: {
    llvm::APInt LeftLit = cast<IntegerLiteral>(Left)->getValue();
    llvm::APInt RightLit = cast<IntegerLiteral>(Right)->getValue();
    return LeftLit.getBitWidth() == RightLit.getBitWidth() &&
           LeftLit == RightLit;
  }
  case Stmt::FloatingLiteralClass:
    return cast<FloatingLiteral>(Left)->getValue().bitwiseIsEqual(
        cast<FloatingLiteral>(Right)->getValue());

  case Stmt::StringLiteralClass:
    return cast<StringLiteral>(Left)->getBytes() ==
           cast<StringLiteral>(Right)->getBytes();

  case Stmt::CXXOperatorCallExprClass:
    return cast<CXXOperatorCallExpr>(Left)->getOperator() ==
           cast<CXXOperatorCallExpr>(Right)->getOperator();

  case Stmt::DependentScopeDeclRefExprClass:
    if (cast<DependentScopeDeclRefExpr>(Left)->getDeclName() !=
        cast<DependentScopeDeclRefExpr>(Right)->getDeclName())
      return false;
    return areEquivalentNameSpecifier(
        cast<DependentScopeDeclRefExpr>(Left)->getQualifier(),
        cast<DependentScopeDeclRefExpr>(Right)->getQualifier());

  case Stmt::DeclRefExprClass:
    return cast<DeclRefExpr>(Left)->getDecl() ==
           cast<DeclRefExpr>(Right)->getDecl();
  case Stmt::MemberExprClass:
    return cast<MemberExpr>(Left)->getMemberDecl() ==
           cast<MemberExpr>(Right)->getMemberDecl();

  case Stmt::CXXFunctionalCastExprClass:
  case Stmt::CStyleCastExprClass:
    return cast<ExplicitCastExpr>(Left)->getTypeAsWritten() ==
           cast<ExplicitCastExpr>(Right)->getTypeAsWritten();

  case Stmt::CallExprClass:
  case Stmt::ImplicitCastExprClass:
  case Stmt::ArraySubscriptExprClass:
    return true;

  case Stmt::UnaryOperatorClass:
    if (cast<UnaryOperator>(Left)->isIncrementDecrementOp())
      return false;
    return cast<UnaryOperator>(Left)->getOpcode() ==
           cast<UnaryOperator>(Right)->getOpcode();

  case Stmt::BinaryOperatorClass:
    return cast<BinaryOperator>(Left)->getOpcode() ==
           cast<BinaryOperator>(Right)->getOpcode();
  }
}

// Returns the first subexpr of an expr that is not a cast or a copy
// constructor's materialize expr.
// Used by matcher `overloadedOperatorsArgumentsAreEquivalent` for stripping
// unnecessary layers from arguments before deciding whether they are
// equivalent.
const Expr *ignoreCastsAndCopyConstructorCalls(const Expr *CallExpression) {
  const Expr *NewExpr;

  if (auto *ConstructorExpr = dyn_cast<CXXConstructExpr>(CallExpression)) {
    if (ConstructorExpr->getNumArgs() == 1)
      NewExpr = ConstructorExpr->getArg(0);
  }

  NewExpr = NewExpr->IgnoreCasts();
  return NewExpr;
}

// For a given expression 'x', returns whether the ranges covered by the
// relational operators are equivalent (i.e.  x <= 4 is equivalent to x < 5).
static bool areEquivalentRanges(BinaryOperatorKind OpcodeLHS,
                                const APSInt &ValueLHS,
                                BinaryOperatorKind OpcodeRHS,
                                const APSInt &ValueRHS) {
  assert(APSInt::compareValues(ValueLHS, ValueRHS) <= 0 &&
         "Values must be ordered");
  // Handle the case where constants are the same: x <= 4  <==>  x <= 4.
  if (APSInt::compareValues(ValueLHS, ValueRHS) == 0)
    return OpcodeLHS == OpcodeRHS;

  // Handle the case where constants are off by one: x <= 4  <==>  x < 5.
  APSInt ValueLHS_plus1;
  return ((OpcodeLHS == BO_LE && OpcodeRHS == BO_LT) ||
          (OpcodeLHS == BO_GT && OpcodeRHS == BO_GE)) &&
         incrementWithoutOverflow(ValueLHS, ValueLHS_plus1) &&
         APSInt::compareValues(ValueLHS_plus1, ValueRHS) == 0;
}

// For a given expression 'x', returns whether the ranges covered by the
// relational operators are fully disjoint (i.e. x < 4  and  x > 7).
static bool areExclusiveRanges(BinaryOperatorKind OpcodeLHS,
                               const APSInt &ValueLHS,
                               BinaryOperatorKind OpcodeRHS,
                               const APSInt &ValueRHS) {
  assert(APSInt::compareValues(ValueLHS, ValueRHS) <= 0 &&
         "Values must be ordered");

  // Handle cases where the constants are the same.
  if (APSInt::compareValues(ValueLHS, ValueRHS) == 0) {
    switch (OpcodeLHS) {
    case BO_EQ:
      return OpcodeRHS == BO_NE || OpcodeRHS == BO_GT || OpcodeRHS == BO_LT;
    case BO_NE:
      return OpcodeRHS == BO_EQ;
    case BO_LE:
      return OpcodeRHS == BO_GT;
    case BO_GE:
      return OpcodeRHS == BO_LT;
    case BO_LT:
      return OpcodeRHS == BO_EQ || OpcodeRHS == BO_GT || OpcodeRHS == BO_GE;
    case BO_GT:
      return OpcodeRHS == BO_EQ || OpcodeRHS == BO_LT || OpcodeRHS == BO_LE;
    default:
      return false;
    }
  }

  // Handle cases where the constants are different.
  if ((OpcodeLHS == BO_EQ || OpcodeLHS == BO_LT || OpcodeLHS == BO_LE) &&
      (OpcodeRHS == BO_EQ || OpcodeRHS == BO_GT || OpcodeRHS == BO_GE))
    return true;

  // Handle the case where constants are off by one: x > 5 && x < 6.
  APSInt ValueLHS_plus1;
  if (OpcodeLHS == BO_GT && OpcodeRHS == BO_LT &&
      incrementWithoutOverflow(ValueLHS, ValueLHS_plus1) &&
      APSInt::compareValues(ValueLHS_plus1, ValueRHS) == 0)
    return true;

  return false;
}

// Returns whether the ranges covered by the union of both relational
// expressions cover the whole domain (i.e. x < 10  and  x > 0).
static bool rangesFullyCoverDomain(BinaryOperatorKind OpcodeLHS,
                                   const APSInt &ValueLHS,
                                   BinaryOperatorKind OpcodeRHS,
                                   const APSInt &ValueRHS) {
  assert(APSInt::compareValues(ValueLHS, ValueRHS) <= 0 &&
         "Values must be ordered");

  // Handle cases where the constants are the same:  x < 5 || x >= 5.
  if (APSInt::compareValues(ValueLHS, ValueRHS) == 0) {
    switch (OpcodeLHS) {
    case BO_EQ:
      return OpcodeRHS == BO_NE;
    case BO_NE:
      return OpcodeRHS == BO_EQ;
    case BO_LE:
      return OpcodeRHS == BO_GT || OpcodeRHS == BO_GE;
    case BO_LT:
      return OpcodeRHS == BO_GE;
    case BO_GE:
      return OpcodeRHS == BO_LT || OpcodeRHS == BO_LE;
    case BO_GT:
      return OpcodeRHS == BO_LE;
    default:
      return false;
    }
  }

  // Handle the case where constants are off by one: x <= 4 || x >= 5.
  APSInt ValueLHS_plus1;
  if (OpcodeLHS == BO_LE && OpcodeRHS == BO_GE &&
      incrementWithoutOverflow(ValueLHS, ValueLHS_plus1) &&
      APSInt::compareValues(ValueLHS_plus1, ValueRHS) == 0)
    return true;

  // Handle cases where the constants are different: x > 4 || x <= 7.
  if ((OpcodeLHS == BO_GT || OpcodeLHS == BO_GE) &&
      (OpcodeRHS == BO_LT || OpcodeRHS == BO_LE))
    return true;

  // Handle cases where constants are different but both ops are !=, like:
  // x != 5 || x != 10
  if (OpcodeLHS == BO_NE && OpcodeRHS == BO_NE)
    return true;

  return false;
}

static bool rangeSubsumesRange(BinaryOperatorKind OpcodeLHS,
                               const APSInt &ValueLHS,
                               BinaryOperatorKind OpcodeRHS,
                               const APSInt &ValueRHS) {
  int Comparison = APSInt::compareValues(ValueLHS, ValueRHS);
  switch (OpcodeLHS) {
  case BO_EQ:
    return OpcodeRHS == BO_EQ && Comparison == 0;
  case BO_NE:
    return (OpcodeRHS == BO_NE && Comparison == 0) ||
           (OpcodeRHS == BO_EQ && Comparison != 0) ||
           (OpcodeRHS == BO_LT && Comparison >= 0) ||
           (OpcodeRHS == BO_LE && Comparison > 0) ||
           (OpcodeRHS == BO_GT && Comparison <= 0) ||
           (OpcodeRHS == BO_GE && Comparison < 0);

  case BO_LT:
    return ((OpcodeRHS == BO_LT && Comparison >= 0) ||
            (OpcodeRHS == BO_LE && Comparison > 0) ||
            (OpcodeRHS == BO_EQ && Comparison > 0));
  case BO_GT:
    return ((OpcodeRHS == BO_GT && Comparison <= 0) ||
            (OpcodeRHS == BO_GE && Comparison < 0) ||
            (OpcodeRHS == BO_EQ && Comparison < 0));
  case BO_LE:
    return (OpcodeRHS == BO_LT || OpcodeRHS == BO_LE || OpcodeRHS == BO_EQ) &&
           Comparison >= 0;
  case BO_GE:
    return (OpcodeRHS == BO_GT || OpcodeRHS == BO_GE || OpcodeRHS == BO_EQ) &&
           Comparison <= 0;
  default:
    return false;
  }
}

static void transformSubToCanonicalAddExpr(BinaryOperatorKind &Opcode,
                                           APSInt &Value) {
  if (Opcode == BO_Sub) {
    Opcode = BO_Add;
    Value = -Value;
  }
}

AST_MATCHER(Expr, isIntegerConstantExpr) {
  if (Node.isInstantiationDependent())
    return false;
  return Node.isIntegerConstantExpr(Finder->getASTContext());
}

AST_MATCHER(BinaryOperator, operandsAreEquivalent) {
  return areEquivalentExpr(Node.getLHS(), Node.getRHS());
}

AST_MATCHER(ConditionalOperator, expressionsAreEquivalent) {
  return areEquivalentExpr(Node.getTrueExpr(), Node.getFalseExpr());
}

AST_MATCHER(CallExpr, parametersAreEquivalent) {
  return Node.getNumArgs() == 2 &&
         areEquivalentExpr(Node.getArg(0), Node.getArg(1));
}

AST_MATCHER(CallExpr, overloadedOperatorsArgumentsAreEquivalent) {
  return (
      Node.getNumArgs() == 2 &&
      areEquivalentExpr(ignoreCastsAndCopyConstructorCalls(Node.getArg(0)),
                        ignoreCastsAndCopyConstructorCalls(Node.getArg(1))));
}

AST_MATCHER(BinaryOperator, binaryOperatorIsInMacro) {
  return Node.getOperatorLoc().isMacroID();
}

AST_MATCHER(ConditionalOperator, conditionalOperatorIsInMacro) {
  return Node.getQuestionLoc().isMacroID() || Node.getColonLoc().isMacroID();
}

AST_MATCHER(Expr, isMacro) { return Node.getExprLoc().isMacroID(); }

AST_MATCHER_P(Expr, expandedByMacro, llvm::StringSet<>, Names) {
  const SourceManager &SM = Finder->getASTContext().getSourceManager();
  const LangOptions &LO = Finder->getASTContext().getLangOpts();
  SourceLocation Loc = Node.getExprLoc();
  while (Loc.isMacroID()) {
    std::string MacroName = Lexer::getImmediateMacroName(Loc, SM, LO);
    if (Names.count(MacroName))
      return true;
    Loc = SM.getImmediateMacroCallerLoc(Loc);
  }
  return false;
}

// Returns a matcher for integer constant expressions.
static ast_matchers::internal::Matcher<Expr>
matchIntegerConstantExpr(StringRef Id) {
  std::string CstId = (Id + "-const").str();
  return expr(isIntegerConstantExpr()).bind(CstId);
}

// Retrieves the integer expression matched by 'matchIntegerConstantExpr' with
// name 'Id' and stores it into 'ConstExpr', the values of the expression is
// stored into `Value`.
static bool retrieveIntegerConstantExpr(const MatchFinder::MatchResult &Result,
                                        StringRef Id, APSInt &Value,
                                        const Expr *&ConstExpr) {
  std::string CstId = (Id + "-const").str();
  ConstExpr = Result.Nodes.getNodeAs<Expr>(CstId);

  if (ConstExpr && ConstExpr->isIntegerConstantExpr(Value, *Result.Context))
    return true;

  return false;
}

// Overloaded `retrieveIntegerConstantExpr` for compatibility.
static bool retrieveIntegerConstantExpr(const MatchFinder::MatchResult &Result,
                                        StringRef Id, APSInt &Value) {
  const Expr *ConstExpr = nullptr;
  return retrieveIntegerConstantExpr(Result, Id, Value, ConstExpr);
}

// Returns a matcher for symbolic expressions (matches every expression except
// ingeter constant expressions).
static ast_matchers::internal::Matcher<Expr> matchSymbolicExpr(StringRef Id) {
  std::string SymId = (Id + "-sym").str();
  return ignoringParenImpCasts(
      expr(unless(isIntegerConstantExpr())).bind(SymId));
}

// Retrieves the expression matched by 'matchSymbolicExpr' with name 'Id' and
// stores it into 'SymExpr'.
static bool retrieveSymbolicExpr(const MatchFinder::MatchResult &Result,
                                 StringRef Id, const Expr *&SymExpr) {
  std::string SymId = (Id + "-sym").str();
  if (const auto *Node = Result.Nodes.getNodeAs<Expr>(SymId)) {
    SymExpr = Node;
    return true;
  }
  return false;
}

// Matches binary operators that are between a symbolic expression and an
// integer constant expression.
static ast_matchers::internal::Matcher<Expr>
matchBinOpIntegerConstantExpr(StringRef Id) {
  const auto BinOpCstExpr =
      expr(
          anyOf(binaryOperator(anyOf(hasOperatorName("+"), hasOperatorName("|"),
                                     hasOperatorName("&")),
                               hasEitherOperand(matchSymbolicExpr(Id)),
                               hasEitherOperand(matchIntegerConstantExpr(Id))),
                binaryOperator(hasOperatorName("-"),
                               hasLHS(matchSymbolicExpr(Id)),
                               hasRHS(matchIntegerConstantExpr(Id)))))
          .bind(Id);
  return ignoringParenImpCasts(BinOpCstExpr);
}

// Retrieves sub-expressions matched by 'matchBinOpIntegerConstantExpr' with
// name 'Id'.
static bool
retrieveBinOpIntegerConstantExpr(const MatchFinder::MatchResult &Result,
                                 StringRef Id, BinaryOperatorKind &Opcode,
                                 const Expr *&Symbol, APSInt &Value) {
  if (const auto *BinExpr = Result.Nodes.getNodeAs<BinaryOperator>(Id)) {
    Opcode = BinExpr->getOpcode();
    return retrieveSymbolicExpr(Result, Id, Symbol) &&
           retrieveIntegerConstantExpr(Result, Id, Value);
  }
  return false;
}

// Matches relational expressions: 'Expr <op> k' (i.e. x < 2, x != 3, 12 <= x).
static ast_matchers::internal::Matcher<Expr>
matchRelationalIntegerConstantExpr(StringRef Id) {
  std::string CastId = (Id + "-cast").str();
  std::string SwapId = (Id + "-swap").str();
  std::string NegateId = (Id + "-negate").str();
  std::string OverloadId = (Id + "-overload").str();

  const auto RelationalExpr = ignoringParenImpCasts(binaryOperator(
      isComparisonOperator(), expr().bind(Id),
      anyOf(allOf(hasLHS(matchSymbolicExpr(Id)),
                  hasRHS(matchIntegerConstantExpr(Id))),
            allOf(hasLHS(matchIntegerConstantExpr(Id)),
                  hasRHS(matchSymbolicExpr(Id)), expr().bind(SwapId)))));

  // A cast can be matched as a comparator to zero. (i.e. if (x) is equivalent
  // to if (x != 0)).
  const auto CastExpr =
      implicitCastExpr(hasCastKind(CK_IntegralToBoolean),
                       hasSourceExpression(matchSymbolicExpr(Id)))
          .bind(CastId);

  const auto NegateRelationalExpr =
      unaryOperator(hasOperatorName("!"),
                    hasUnaryOperand(anyOf(CastExpr, RelationalExpr)))
          .bind(NegateId);

  // Do not bind to double negation, it is uneffective.
  const auto NegateNegateRelationalExpr =
      unaryOperator(hasOperatorName("!"),
                    hasUnaryOperand(unaryOperator(
                        hasOperatorName("!"),
                        hasUnaryOperand(anyOf(CastExpr, RelationalExpr)))));

  const auto OverloadedOperatorExpr =
      cxxOperatorCallExpr(
          anyOf(hasOverloadedOperatorName("=="),
                hasOverloadedOperatorName("!="), hasOverloadedOperatorName("<"),
                hasOverloadedOperatorName("<="), hasOverloadedOperatorName(">"),
                hasOverloadedOperatorName(">=")),
          // Filter noisy false positives.
          unless(isMacro()), unless(isInTemplateInstantiation()))
          .bind(OverloadId);

  return anyOf(RelationalExpr, CastExpr, NegateRelationalExpr,
               NegateNegateRelationalExpr, OverloadedOperatorExpr);
}

// Checks whether a function param is non constant reference type, and may
// be modified in the function.
static bool isParamNonConstReferenceType(QualType ParamType) {
  if (ParamType->isReferenceType() &&
      !ParamType.getNonReferenceType().isConstQualified())
    return true;
  return false;
}

// Checks whether the arguments of an overloaded operator can be modified in the
// function.
// For operators that take an instance and a constant as arguments, only the
// first argument (the instance) needs to be checked, since the constant itself
// is a temporary expression. Whether the second parameter is checked is
// controlled by the parameter `ParamsToCheckCount`.
static bool
canOverloadedOperatorArgsBeModified(const FunctionDecl *OperatorDecl,
                                    int ParamCount, int ParamsToCheckCount) {
  // Overloaded operators declared inside a class have only one param.
  // These functions must be declared const in order to not be able to modify
  // the instance of the class they are called through.
  if (ParamCount == 1 &&
      !cast<FunctionType>(OperatorDecl->getType().getTypePtr())->isConst())
    return true;

  if (isParamNonConstReferenceType(OperatorDecl->getParamDecl(0)->getType()))
    return true;

  if ((ParamsToCheckCount == 2 && ParamCount == 2) &&
      isParamNonConstReferenceType(OperatorDecl->getParamDecl(1)->getType()))
    return true;

  return false;
}

// OverloadedOperatorKind -> BinaryOperatorKind conversion.
static bool convertOverloadedOpToBinOp(OverloadedOperatorKind OverloadedOp,
                                       BinaryOperatorKind &BinaryOp) {
  switch (OverloadedOp) {
  case OO_EqualEqual:
    BinaryOp = BO_EQ;
    break;
  case OO_ExclaimEqual:
    BinaryOp = BO_NE;
    break;
  case OO_LessEqual:
    BinaryOp = BO_LE;
    break;
  case OO_GreaterEqual:
    BinaryOp = BO_GE;
    break;
  case OO_Less:
    BinaryOp = BO_LT;
    break;
  case OO_Greater:
    BinaryOp = BO_GT;
    break;
  default:
    return false;
  }
  return true;
}

// Retrieves sub-expressions matched by 'matchRelationalIntegerConstantExpr'
// with name 'Id'.
static bool retrieveRelationalIntegerConstantExpr(
    const MatchFinder::MatchResult &Result, StringRef Id,
    const Expr *&OperandExpr, BinaryOperatorKind &Opcode, const Expr *&Symbol,
    APSInt &Value, const Expr *&ConstExpr) {
  std::string CastId = (Id + "-cast").str();
  std::string SwapId = (Id + "-swap").str();
  std::string NegateId = (Id + "-negate").str();
  std::string OverloadId = (Id + "-overload").str();

  if (const auto *Bin = Result.Nodes.getNodeAs<BinaryOperator>(Id)) {
    // Operand received with explicit comparator.
    Opcode = Bin->getOpcode();
    OperandExpr = Bin;

    if (!retrieveIntegerConstantExpr(Result, Id, Value, ConstExpr))
      return false;
  } else if (const auto *Cast = Result.Nodes.getNodeAs<CastExpr>(CastId)) {
    // Operand received with implicit comparator (cast).
    Opcode = BO_NE;
    OperandExpr = Cast;
    Value = APSInt(32, false);
  } else if (const auto *OverloadedOperatorExpr =
                 Result.Nodes.getNodeAs<CXXOperatorCallExpr>(OverloadId)) {
    const auto *OverloadedFunctionDecl =
        cast<FunctionDecl>(OverloadedOperatorExpr->getCalleeDecl());
    if (!OverloadedFunctionDecl || OverloadedOperatorExpr->getNumArgs() != 2)
      return false;

    if (canOverloadedOperatorArgsBeModified(
            OverloadedFunctionDecl, OverloadedFunctionDecl->getNumParams(), 1))
      return false;

    if (!OverloadedOperatorExpr->getArg(1)->isIntegerConstantExpr(
            Value, *Result.Context))
      return false;

    Symbol =
        ignoreCastsAndCopyConstructorCalls(OverloadedOperatorExpr->getArg(0));

    OperandExpr = OverloadedOperatorExpr;

    return convertOverloadedOpToBinOp(OverloadedOperatorExpr->getOperator(),
                                      Opcode);
  } else {
    return false;
  }

  if (!retrieveSymbolicExpr(Result, Id, Symbol))
    return false;

  if (Result.Nodes.getNodeAs<Expr>(SwapId))
    Opcode = BinaryOperator::reverseComparisonOp(Opcode);
  if (Result.Nodes.getNodeAs<Expr>(NegateId))
    Opcode = BinaryOperator::negateComparisonOp(Opcode);

  return true;
}

// Checks for expressions like (X == 4) && (Y != 9).
static bool areSidesBinaryConstExpressions(const BinaryOperator *&BinOp,
                                           const ASTContext *AstCtx) {
  if (!isa<BinaryOperator>(BinOp->getLHS()) ||
      !isa<BinaryOperator>(BinOp->getRHS()))
    return false;
  BinaryOperator *LhsBinOp = dyn_cast<BinaryOperator>(BinOp->getLHS());
  BinaryOperator *RhsBinOp = dyn_cast<BinaryOperator>(BinOp->getRHS());
  if ((LhsBinOp->getLHS()->isIntegerConstantExpr(*AstCtx) ||
       LhsBinOp->getRHS()->isIntegerConstantExpr(*AstCtx)) &&
      (RhsBinOp->getLHS()->isIntegerConstantExpr(*AstCtx) ||
       RhsBinOp->getRHS()->isIntegerConstantExpr(*AstCtx)))
    return true;
  return false;
}

// Retrieves integer constant subexpressions from binary operator expressions
// that have two equivalent sides.
// E.g.: from (X == 5) && (X == 5) retrieves 5 and 5.
static bool retrieveConstExprFromBothSides(const BinaryOperator *&BinOp,
                                           BinaryOperatorKind &MainOpcode,
                                           BinaryOperatorKind &SideOpcode,
                                           const Expr *&LhsConst,
                                           const Expr *&RhsConst,
                                           const ASTContext *AstCtx) {
  assert(areSidesBinaryConstExpressions(BinOp, AstCtx) &&
         "Both sides of binary operator must be constant expressions!");

  MainOpcode = BinOp->getOpcode();

  BinaryOperator *BinOpLhs = dyn_cast<BinaryOperator>(BinOp->getLHS());
  BinaryOperator *BinOpRhs = dyn_cast<BinaryOperator>(BinOp->getRHS());

  LhsConst = BinOpLhs->getLHS()->isIntegerConstantExpr(*AstCtx)
                 ? BinOpLhs->getLHS()
                 : BinOpLhs->getRHS();
  RhsConst = BinOpRhs->getLHS()->isIntegerConstantExpr(*AstCtx)
                 ? BinOpRhs->getLHS()
                 : BinOpRhs->getRHS();

  if (!LhsConst || !RhsConst)
    return false;

  assert((BinOpLhs->getOpcode() == BinOpRhs->getOpcode()) &&
         "Sides of the binary operator must be equivalent expressions!");

  SideOpcode = BinOpLhs->getOpcode();

  return true;
}

static bool areExprsFromDifferentMacros(const Expr *LhsExpr,
                                        const Expr *RhsExpr,
                                        const ASTContext *AstCtx) {
  if (!LhsExpr || !RhsExpr)
    return false;

  SourceLocation LhsLoc = LhsExpr->getExprLoc();
  SourceLocation RhsLoc = RhsExpr->getExprLoc();

  if (!LhsLoc.isMacroID() || !RhsLoc.isMacroID())
    return false;

  const SourceManager &SM = AstCtx->getSourceManager();
  const LangOptions &LO = AstCtx->getLangOpts();

  StringRef LhsMacroName = Lexer::getImmediateMacroName(LhsLoc, SM, LO);
  StringRef RhsMacroName = Lexer::getImmediateMacroName(RhsLoc, SM, LO);

  if (LhsMacroName == RhsMacroName)
    return false;
  return true;
}

static bool areExprsMacroAndNonMacro(const Expr *&LhsExpr,
                                     const Expr *&RhsExpr) {
  if (!LhsExpr || !RhsExpr)
    return false;

  SourceLocation LhsLoc = LhsExpr->getExprLoc();
  SourceLocation RhsLoc = RhsExpr->getExprLoc();

  if ((LhsLoc.isMacroID() && !RhsLoc.isMacroID()) ||
      (!LhsLoc.isMacroID() && RhsLoc.isMacroID()))
    return true;
  return false;
}

static bool isLessOperator(BinaryOperatorKind &Opcode) {
  return Opcode == BO_LT || Opcode == BO_LE;
}

static bool isGreaterOperator(BinaryOperatorKind &Opcode) {
  return Opcode == BO_GT || Opcode == BO_GE;
}

// Checks whether a set of operators are able to create a meaningful and
// non-redundant expression.
// The function is used by `isIntendedMacroExpression` to filter operator sets
// like (<, &&, <=), that always introduce redundant expressions.
static bool isOperatorSetMeaningful(BinaryOperatorKind &Opcode,
                                    BinaryOperatorKind &LhsOpcode,
                                    BinaryOperatorKind &RhsOpcode) {
  if (!BinaryOperator::isLogicalOp(Opcode))
    return false;

  if ((isLessOperator(LhsOpcode) && isGreaterOperator(RhsOpcode)) ||
      (isLessOperator(RhsOpcode) && isGreaterOperator(LhsOpcode)))
    return true;

  if (Opcode == BO_LAnd) {
    if (LhsOpcode == BO_NE &&
        (RhsOpcode == BO_NE || BinaryOperator::isRelationalOp(RhsOpcode)))
      return true;
    if (RhsOpcode == BO_NE && BinaryOperator::isRelationalOp(LhsOpcode))
      return true;
    return false;
  }

  if (Opcode == BO_LOr) {
    if (LhsOpcode == BO_EQ &&
        (RhsOpcode == BO_EQ || BinaryOperator::isRelationalOp(RhsOpcode)))
      return true;
    if (RhsOpcode == BO_EQ && BinaryOperator::isRelationalOp(LhsOpcode))
      return true;
    return false;
  }

  return false;
}

// Checks whether an expression containing one or two macros,
// like: ((Expr <op> M1/K) <REL> (Expr <op> M2)), can be a meaningful one
// based on the operators.
// A macro expression is considered meaningful, if the macro or macros can hold
// values, with which the expression cannot be simplified further.
// The decison is independent from the actual values of the macros, since
// they may vary across builds.
// Examples:
// The function discards  (X < M1 && X <= M2), because it can always be
// simplified to X < M, where M is the smaller one of the two macro
// values.
// An expresion that can be considered meaningful and intended is:
// (X < M1 && X > M2), even when the two ranges are exclusive.
static bool isIntendedMacroExpression(BinaryOperatorKind &Opcode,
                                      BinaryOperatorKind &LhsOpcode,
                                      const Expr *&LhsExpr,
                                      BinaryOperatorKind &RhsOpcode,
                                      const Expr *&RhsExpr,
                                      const ASTContext *AstCtx) {
  if ((areExprsFromDifferentMacros(LhsExpr, RhsExpr, AstCtx) ||
       areExprsMacroAndNonMacro(LhsExpr, RhsExpr)) &&
      isOperatorSetMeaningful(Opcode, LhsOpcode, RhsOpcode))
    return true;
  return false;
}

void RedundantExpressionCheck::registerMatchers(MatchFinder *Finder) {
  const auto AnyLiteralExpr = ignoringParenImpCasts(
      anyOf(cxxBoolLiteral(), characterLiteral(), integerLiteral()));

  const auto BannedIntegerLiteral =
      integerLiteral(expandedByMacro(KnownBannedMacroNames));

  // Binary with equivalent operands, like (X != 2 && X != 2).
  Finder->addMatcher(
      binaryOperator(anyOf(hasOperatorName("-"), hasOperatorName("/"),
                           hasOperatorName("%"), hasOperatorName("|"),
                           hasOperatorName("&"), hasOperatorName("^"),
                           matchers::isComparisonOperator(),
                           hasOperatorName("&&"), hasOperatorName("||"),
                           hasOperatorName("=")),
                     operandsAreEquivalent(),
                     // Filter noisy false positives.
                     unless(isInTemplateInstantiation()),
                     unless(binaryOperatorIsInMacro()),
                     unless(hasType(realFloatingPointType())),
                     unless(hasEitherOperand(hasType(realFloatingPointType()))),
                     unless(hasLHS(AnyLiteralExpr)),
                     unless(hasDescendant(BannedIntegerLiteral)))
          .bind("binary"),
      this);

  // Conditional (trenary) operator with equivalent operands, like (Y ? X : X).
  Finder->addMatcher(conditionalOperator(expressionsAreEquivalent(),
                                         // Filter noisy false positives.
                                         unless(conditionalOperatorIsInMacro()),
                                         unless(isInTemplateInstantiation()))
                         .bind("cond"),
                     this);

  // Overloaded operators with equivalent operands.
  Finder->addMatcher(
      cxxOperatorCallExpr(
          anyOf(
              hasOverloadedOperatorName("-"), hasOverloadedOperatorName("/"),
              hasOverloadedOperatorName("%"), hasOverloadedOperatorName("|"),
              hasOverloadedOperatorName("&"), hasOverloadedOperatorName("^"),
              hasOverloadedOperatorName("=="), hasOverloadedOperatorName("!="),
              hasOverloadedOperatorName("<"), hasOverloadedOperatorName("<="),
              hasOverloadedOperatorName(">"), hasOverloadedOperatorName(">="),
              hasOverloadedOperatorName("&&"), hasOverloadedOperatorName("||"),
              hasOverloadedOperatorName("=")),
          overloadedOperatorsArgumentsAreEquivalent(),
          // Filter noisy false positives.
          unless(isMacro()), unless(isInTemplateInstantiation()))
          .bind("call"),
      this);

  const auto IneffBitwiseConst = matchIntegerConstantExpr("ineff-bitwise");
  const auto IneffBitwiseSymExpr = matchSymbolicExpr("ineff-bitwise");

  // Match ineffective or redundant bitwise operator expressions like: x & 0 or x |= ~0.
  Finder->addMatcher(
      binaryOperator(anyOf(hasOperatorName("|"), hasOperatorName("&"),
                           hasOperatorName("|="), hasOperatorName("&=")),
                     hasEitherOperand(IneffBitwiseConst),
                     hasEitherOperand(IneffBitwiseSymExpr))
          .bind("ineffective-bitwise"),
      this);

  // Match common expressions and apply more checks to find redundant
  // sub-expressions.
  //   a) Expr <op> K1 == K2
  //   b) Expr <op> K1 == Expr
  //   c) Expr <op> K1 == Expr <op> K2
  // see: 'checkArithmeticExpr' and 'checkBitwiseExpr'
  const auto BinOpCstLeft = matchBinOpIntegerConstantExpr("lhs");
  const auto BinOpCstRight = matchBinOpIntegerConstantExpr("rhs");
  const auto CstRight = matchIntegerConstantExpr("rhs");
  const auto SymRight = matchSymbolicExpr("rhs");

  // Match expressions like: x <op> 0xFF == 0xF00.
  Finder->addMatcher(binaryOperator(isComparisonOperator(),
                                    hasEitherOperand(BinOpCstLeft),
                                    hasEitherOperand(CstRight))
                         .bind("binop-const-compare-to-const"),
                     this);

  // Match expressions like: x <op> 0xFF == x.
  Finder->addMatcher(
      binaryOperator(isComparisonOperator(),
                     anyOf(allOf(hasLHS(BinOpCstLeft), hasRHS(SymRight)),
                           allOf(hasLHS(SymRight), hasRHS(BinOpCstLeft))))
          .bind("binop-const-compare-to-sym"),
      this);

  // Match expressions like: x <op> 10 == x <op> 12.
  Finder->addMatcher(binaryOperator(isComparisonOperator(),
                                    hasLHS(BinOpCstLeft), hasRHS(BinOpCstRight),
                                    // Already reported as redundant.
                                    unless(operandsAreEquivalent()))
                         .bind("binop-const-compare-to-binop-const"),
                     this);

  // Match relational expressions combined with logical operators and find
  // redundant sub-expressions.
  // see: 'checkRelationalExpr'

  // Match expressions like: x < 2 && x > 2.
  const auto ComparisonLeft = matchRelationalIntegerConstantExpr("lhs");
  const auto ComparisonRight = matchRelationalIntegerConstantExpr("rhs");
  Finder->addMatcher(
      binaryOperator(anyOf(hasOperatorName("||"), hasOperatorName("&&")),
                     hasLHS(ComparisonLeft), hasRHS(ComparisonRight),
                     // Already reported as redundant.
                     unless(operandsAreEquivalent()))
          .bind("comparisons-of-symbol-and-const"),
      this);
}

void RedundantExpressionCheck::checkArithmeticExpr(
    const MatchFinder::MatchResult &Result) {
  APSInt LhsValue, RhsValue;
  const Expr *LhsSymbol = nullptr, *RhsSymbol = nullptr;
  BinaryOperatorKind LhsOpcode, RhsOpcode;

  if (const auto *ComparisonOperator = Result.Nodes.getNodeAs<BinaryOperator>(
          "binop-const-compare-to-sym")) {
    BinaryOperatorKind Opcode = ComparisonOperator->getOpcode();
    if (!retrieveBinOpIntegerConstantExpr(Result, "lhs", LhsOpcode, LhsSymbol,
                                          LhsValue) ||
        !retrieveSymbolicExpr(Result, "rhs", RhsSymbol) ||
        !areEquivalentExpr(LhsSymbol, RhsSymbol))
      return;

    // Check expressions: x + k == x  or  x - k == x.
    if (LhsOpcode == BO_Add || LhsOpcode == BO_Sub) {
      if ((LhsValue != 0 && Opcode == BO_EQ) ||
          (LhsValue == 0 && Opcode == BO_NE))
        diag(ComparisonOperator->getOperatorLoc(),
             "logical expression is always false");
      else if ((LhsValue == 0 && Opcode == BO_EQ) ||
               (LhsValue != 0 && Opcode == BO_NE))
        diag(ComparisonOperator->getOperatorLoc(),
             "logical expression is always true");
    }
  } else if (const auto *ComparisonOperator =
                 Result.Nodes.getNodeAs<BinaryOperator>(
                     "binop-const-compare-to-binop-const")) {
    BinaryOperatorKind Opcode = ComparisonOperator->getOpcode();

    if (!retrieveBinOpIntegerConstantExpr(Result, "lhs", LhsOpcode, LhsSymbol,
                                          LhsValue) ||
        !retrieveBinOpIntegerConstantExpr(Result, "rhs", RhsOpcode, RhsSymbol,
                                          RhsValue) ||
        !areEquivalentExpr(LhsSymbol, RhsSymbol))
      return;

    transformSubToCanonicalAddExpr(LhsOpcode, LhsValue);
    transformSubToCanonicalAddExpr(RhsOpcode, RhsValue);

    // Check expressions: x + 1 == x + 2  or  x + 1 != x + 2.
    if (LhsOpcode == BO_Add && RhsOpcode == BO_Add) {
      if ((Opcode == BO_EQ && APSInt::compareValues(LhsValue, RhsValue) == 0) ||
          (Opcode == BO_NE && APSInt::compareValues(LhsValue, RhsValue) != 0)) {
        diag(ComparisonOperator->getOperatorLoc(),
             "logical expression is always true");
      } else if ((Opcode == BO_EQ &&
                  APSInt::compareValues(LhsValue, RhsValue) != 0) ||
                 (Opcode == BO_NE &&
                  APSInt::compareValues(LhsValue, RhsValue) == 0)) {
        diag(ComparisonOperator->getOperatorLoc(),
             "logical expression is always false");
      }
    }
  }
}

static bool exprEvaluatesToZero(BinaryOperatorKind Opcode, APSInt Value){
 if((Opcode == BO_And || Opcode == BO_AndAssign) && Value == 0)
     return true;
 return false;
}

static bool exprEvaluatesToNotZero(BinaryOperatorKind Opcode, APSInt Value){
 if((Opcode == BO_Or || Opcode == BO_OrAssign) && (~Value) == 0)
     return true;
 return false;
}

static bool exprEvaluatesToSymbolic(BinaryOperatorKind Opcode, APSInt Value){
  if ((Opcode == BO_Or || Opcode == BO_OrAssign) && Value == 0)
    return true;
  if ((Opcode == BO_And || Opcode == BO_AndAssign) && (~Value) == 0)
    return true;

  return false;
}

void RedundantExpressionCheck::checkBitwiseExpr(
    const MatchFinder::MatchResult &Result) {
  if (const auto *ComparisonOperator = Result.Nodes.getNodeAs<BinaryOperator>(
          "binop-const-compare-to-const")) {
    BinaryOperatorKind Opcode = ComparisonOperator->getOpcode();

    APSInt LhsValue, RhsValue;
    const Expr *LhsSymbol = nullptr;
    BinaryOperatorKind LhsOpcode;
    if (!retrieveBinOpIntegerConstantExpr(Result, "lhs", LhsOpcode, LhsSymbol,
                                          LhsValue) ||
        !retrieveIntegerConstantExpr(Result, "rhs", RhsValue))
      return;

    uint64_t LhsConstant = LhsValue.getZExtValue();
    uint64_t RhsConstant = RhsValue.getZExtValue();
    SourceLocation Loc = ComparisonOperator->getOperatorLoc();

    // Check expression: x & k1 == k2  (i.e. x & 0xFF == 0xF00)
    if (LhsOpcode == BO_And && (LhsConstant & RhsConstant) != RhsConstant) {
      if (Opcode == BO_EQ)
        diag(Loc, "logical expression is always false");
      else if (Opcode == BO_NE)
        diag(Loc, "logical expression is always true");
    }

    // Check expression: x | k1 == k2  (i.e. x | 0xFF == 0xF00)
    if (LhsOpcode == BO_Or && (LhsConstant | RhsConstant) != RhsConstant) {
      if (Opcode == BO_EQ)
        diag(Loc, "logical expression is always false");
      else if (Opcode == BO_NE)
        diag(Loc, "logical expression is always true");
    }
  } else if (const auto *IneffectiveOperator =
                 Result.Nodes.getNodeAs<BinaryOperator>(
                     "ineffective-bitwise")) {
    APSInt Value;
    const Expr* Sym = nullptr, *ConstExpr = nullptr;
    BinaryOperatorKind Opcode = IneffectiveOperator->getOpcode();

    if (!retrieveSymbolicExpr(Result, "ineff-bitwise", Sym) ||
        !retrieveIntegerConstantExpr(Result, "ineff-bitwise", Value, ConstExpr))
      return;

    if(Value != 0 && (~Value) != 0)
        return;

    SourceLocation Loc = IneffectiveOperator->getOperatorLoc();

    if (exprEvaluatesToZero(Opcode, Value)) {
      diag(Loc, "expression always evaluates to 0");
    } else if (exprEvaluatesToNotZero(Opcode, Value)) {
      SourceRange ConstExprRange(ConstExpr->getLocStart(),
                                 ConstExpr->getLocEnd());
      StringRef ConstExprText = Lexer::getSourceText(
          CharSourceRange::getTokenRange(ConstExprRange), *Result.SourceManager,
          Result.Context->getLangOpts());

      std::string message = ("expression always evaluates to " + ConstExprText).str();
      diag(Loc, message);
    } else if (exprEvaluatesToSymbolic(Opcode, Value)) {

      SourceRange SymExprRange(Sym->getLocStart(), Sym->getLocEnd());

      StringRef ExprText = Lexer::getSourceText(
          CharSourceRange::getTokenRange(SymExprRange), *Result.SourceManager,
          Result.Context->getLangOpts());

      std::string message = ("expression always evaluates to '" + ExprText + "'").str();
      diag(Loc, message);
     }
  }
}

void RedundantExpressionCheck::checkRelationalExpr(
    const MatchFinder::MatchResult &Result) {
  if (const auto *ComparisonOperator = Result.Nodes.getNodeAs<BinaryOperator>(
          "comparisons-of-symbol-and-const")) {
    // Matched expressions are: (x <op> k1) <REL> (x <op> k2).
    // E.g.: (X < 2) && (X > 4)
    BinaryOperatorKind Opcode = ComparisonOperator->getOpcode();

    const Expr *LhsExpr = nullptr, *RhsExpr = nullptr;
    const Expr *LhsSymbol = nullptr, *RhsSymbol = nullptr;
    const Expr *LhsConst = nullptr, *RhsConst = nullptr;
    BinaryOperatorKind LhsOpcode, RhsOpcode;
    APSInt LhsValue, RhsValue;

    if (!retrieveRelationalIntegerConstantExpr(
            Result, "lhs", LhsExpr, LhsOpcode, LhsSymbol, LhsValue, LhsConst) ||
        !retrieveRelationalIntegerConstantExpr(
            Result, "rhs", RhsExpr, RhsOpcode, RhsSymbol, RhsValue, RhsConst) ||
        !areEquivalentExpr(LhsSymbol, RhsSymbol))
      return;

    // Bring expr to a canonical form: smallest constant must be on the left.
    if (APSInt::compareValues(LhsValue, RhsValue) > 0) {
      std::swap(LhsExpr, RhsExpr);
      std::swap(LhsValue, RhsValue);
      std::swap(LhsSymbol, RhsSymbol);
      std::swap(LhsOpcode, RhsOpcode);
    }

    // Do not warn if macro expr can be considered intentional.
    if (isIntendedMacroExpression(Opcode, LhsOpcode, LhsConst, RhsOpcode,
                                  RhsConst, Result.Context))
      return;

    if ((Opcode == BO_LAnd || Opcode == BO_LOr) &&
        areEquivalentRanges(LhsOpcode, LhsValue, RhsOpcode, RhsValue)) {
      diag(ComparisonOperator->getOperatorLoc(),
           "equivalent expression on both sides of logical operator");
      return;
    }

    if (Opcode == BO_LAnd) {
      if (areExclusiveRanges(LhsOpcode, LhsValue, RhsOpcode, RhsValue)) {
        diag(ComparisonOperator->getOperatorLoc(),
             "logical expression is always false");
      } else if (rangeSubsumesRange(LhsOpcode, LhsValue, RhsOpcode, RhsValue)) {
        diag(LhsExpr->getExprLoc(), "expression is redundant");
      } else if (rangeSubsumesRange(RhsOpcode, RhsValue, LhsOpcode, LhsValue)) {
        diag(RhsExpr->getExprLoc(), "expression is redundant");
      }
    }

    if (Opcode == BO_LOr) {
      if (rangesFullyCoverDomain(LhsOpcode, LhsValue, RhsOpcode, RhsValue)) {
        diag(ComparisonOperator->getOperatorLoc(),
             "logical expression is always true");
      } else if (rangeSubsumesRange(LhsOpcode, LhsValue, RhsOpcode, RhsValue)) {
        diag(RhsExpr->getExprLoc(), "expression is redundant");
      } else if (rangeSubsumesRange(RhsOpcode, RhsValue, LhsOpcode, LhsValue)) {
        diag(LhsExpr->getExprLoc(), "expression is redundant");
      }
    }
  }
}

void RedundantExpressionCheck::check(const MatchFinder::MatchResult &Result) {
  if (const auto *BinOp = Result.Nodes.getNodeAs<BinaryOperator>("binary")) {
    // If the expression's constants are macros, check whether they are
    // intentional.
    if (areSidesBinaryConstExpressions(BinOp, Result.Context)) {
      const Expr *LhsConst = nullptr, *RhsConst = nullptr;
      BinaryOperatorKind MainOpcode, SideOpcode;

      if (!retrieveConstExprFromBothSides(BinOp, MainOpcode, SideOpcode,
                                          LhsConst, RhsConst, Result.Context))
        return;

      if (isIntendedMacroExpression(MainOpcode, SideOpcode, LhsConst,
                                    SideOpcode, RhsConst, Result.Context))
        return;
    }

    diag(BinOp->getOperatorLoc(), "both sides of operator are equivalent");
  }

  if (const auto *CondOp =
          Result.Nodes.getNodeAs<ConditionalOperator>("cond")) {
    const auto *TrueExpr = CondOp->getTrueExpr();
    const auto *FalseExpr = CondOp->getFalseExpr();

    if (areExprsFromDifferentMacros(TrueExpr, FalseExpr, Result.Context) ||
        areExprsMacroAndNonMacro(TrueExpr, FalseExpr))
      return;
    diag(CondOp->getColonLoc(),
         "'true' and 'false' expressions are equivalent");
  }

  // Check overloaded operators with equivalent operands.
  if (const auto *Call = Result.Nodes.getNodeAs<CXXOperatorCallExpr>("call")) {
    const auto *OverloadedFunctionDecl =
        cast<FunctionDecl>(Call->getCalleeDecl());

    if (!OverloadedFunctionDecl || Call->getNumArgs() != 2)
      return;

    if (canOverloadedOperatorArgsBeModified(
            OverloadedFunctionDecl, OverloadedFunctionDecl->getNumParams(), 2))
      return;

    diag(Call->getOperatorLoc(),
         "both sides of overloaded operator are equivalent");
  }

  // Check for the following binded expressions:
  // - "binop-const-compare-to-sym",
  // - "binop-const-compare-to-binop-const",
  // Produced message:
  // -> "logical expression is always false/true"
  checkArithmeticExpr(Result);

  // Check for the following binded expression:
  // - "binop-const-compare-to-const",
  // - "ineffective-bitwise"
  // Produced message:
  // -> "logical expression is always false/true"
  // -> "expression always evaluates to ..."
  checkBitwiseExpr(Result);

  // Check for te following binded expression:
  // - "comparisons-of-symbol-and-const",
  // Produced messages:
  // -> "equivalent expression on both sides of logical operator",
  // -> "logical expression is always false/true"
  // -> "expression is redundant"
  checkRelationalExpr(Result);
}

} // namespace misc
} // namespace tidy
} // namespace clang
