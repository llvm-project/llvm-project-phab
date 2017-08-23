//===--- UnnecessaryIntermediateVarCheck.cpp -
//clang-tidy----------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "UnnecessaryIntermediateVarCheck.h"
#include "../utils/LexerUtils.h"
#include "../utils/Matchers.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace readability {

void UnnecessaryIntermediateVarCheck::registerMatchers(MatchFinder *Finder) {
  // We match a direct declaration reference expression pointing
  // to the variable declaration 1 as LHS.
  const auto DirectDeclRefExprLHS1 =
      ignoringImplicit(ignoringParenCasts(declRefExpr(
          to(equalsBoundNode("varDecl1")), expr().bind("declRefExprLHS1"))));

  // We match a direct declaration reference expression pointing
  // to the variable declaration 1 as RHS.
  const auto DirectDeclRefExprRHS1 =
      ignoringImplicit(ignoringParenCasts(declRefExpr(
          to(equalsBoundNode("varDecl1")), expr().bind("declRefExprRHS1"))));

  // We match a declaration reference expression in any descendant
  // pointing to variable declaration 1.
  const auto NoIndirectDeclRefExpr1 =
      unless(hasDescendant(declRefExpr(to(equalsBoundNode("varDecl1")))));

  // We match a direct declaration reference expression pointing
  // to the variable declaration 2 as LHS.
  const auto DirectDeclRefExprLHS2 =
      ignoringImplicit(ignoringParenCasts(declRefExpr(
          to(equalsBoundNode("varDecl2")), expr().bind("declRefExprLHS2"))));

  // We match a direct declaration reference expression pointing
  // to the variable declaration 2 as RHS.
  const auto DirectDeclRefExprRHS2 =
      ignoringImplicit(ignoringParenCasts(declRefExpr(
          to(equalsBoundNode("varDecl2")), expr().bind("declRefExprRHS2"))));

  // We match a declaration reference expression in any descendant
  // pointing to variable declaration 2.
  const auto NoIndirectDeclRefExpr2 =
      unless(hasDescendant(declRefExpr(to(equalsBoundNode("varDecl2")))));

  const auto HasVarDecl1 =
      // We match a single declaration which is a variable declaration,
      hasSingleDecl(varDecl(
          // which has an initializer,
          hasInitializer(allOf(NoIndirectDeclRefExpr2, expr().bind("init1"))),
          // and which isn't static.
          unless(isStaticStorageClass()), decl().bind("varDecl1")));

  const auto HasVarDecl2 =
      // We match a single declaration which is a variable declaration,
      hasSingleDecl(varDecl(
          // which has an initializer,
          hasInitializer(allOf(NoIndirectDeclRefExpr1, expr().bind("init2"))),
          // and which isn't static.
          unless(isStaticStorageClass()), decl().bind("varDecl2")));

  const auto ReturnStmt1 =
      // We match a return statement,
      returnStmt(
          stmt().bind("returnStmt1"),

          // which has a return value which is a binary operator,
          hasReturnValue(ignoringImplicit(ignoringParenCasts(
              binaryOperator(expr().bind("binOp"),

                             // which is a comparison operator,
                             matchers::isComparisonOperator(),

                             // which may contain a direct reference to var decl
                             // 1 on only one side.
                             anyOf(allOf(hasLHS(DirectDeclRefExprLHS1),
                                         hasRHS(NoIndirectDeclRefExpr1)),
                                   allOf(hasLHS(NoIndirectDeclRefExpr1),
                                         hasRHS(DirectDeclRefExprRHS1))))))));

  const auto ReturnStmt2 =
      // We match a return statement,
      returnStmt(
          stmt().bind("returnStmt2"),

          // which has a return value which is a binary operator,
          hasReturnValue(ignoringImplicit(ignoringParenCasts(binaryOperator(
              expr().bind("binOp"),

              // which is a comparison operator,
              matchers::isComparisonOperator(),

              // which may contain a direct reference to a var decl on one side,
              // as long as there is no indirect reference to the same var decl
              // on the other size.
              anyOf(
                  allOf(
                      hasLHS(DirectDeclRefExprLHS1),
                      hasRHS(allOf(NoIndirectDeclRefExpr1,
                                   anyOf(DirectDeclRefExprRHS2, anything())))),

                  allOf(
                      hasLHS(DirectDeclRefExprLHS2),
                      hasRHS(allOf(NoIndirectDeclRefExpr2,
                                   anyOf(DirectDeclRefExprRHS1, anything())))),

                  allOf(hasLHS(allOf(NoIndirectDeclRefExpr1,
                                     anyOf(DirectDeclRefExprLHS2, anything()))),
                        hasRHS(DirectDeclRefExprRHS1)),

                  allOf(hasLHS(allOf(NoIndirectDeclRefExpr2,
                                     anyOf(DirectDeclRefExprLHS1, anything()))),
                        hasRHS(DirectDeclRefExprRHS2))))))));

  Finder->addMatcher(
      // We match a declaration statement,
      declStmt(
          stmt().bind("declStmt1"),

          // which contains a single variable declaration,
          HasVarDecl1,

          // and which has a successor,
          matchers::hasSuccessor(anyOf(
              // which is another declaration statement,
              declStmt(stmt().bind("declStmt2"),

                       // which contains a single variable declaration,
                       HasVarDecl2,

                       // and which has a successor which is a return statement
                       // which may contain var decl 1 or 2.
                       matchers::hasSuccessor(ReturnStmt2)),
              // or which is a return statement only containing var decl 1.
              ReturnStmt1))),
      this);
}

void UnnecessaryIntermediateVarCheck::storeOptions(
    ClangTidyOptions::OptionMap &Opts) {
  Options.store(Opts, "MaximumLineLength", MaximumLineLength);
}

void UnnecessaryIntermediateVarCheck::emitMainWarning(const VarDecl *VarDecl1,
                                                      const VarDecl *VarDecl2) {
  diag(VarDecl1->getLocation(), "unnecessary intermediate variable %0",
       DiagnosticIDs::Warning)
      << VarDecl1;

  if (VarDecl2) {
    diag(VarDecl2->getLocation(), "and so is %0", DiagnosticIDs::Warning)
        << VarDecl2;
  }
}

void UnnecessaryIntermediateVarCheck::emitUsageInComparisonNote(
    const DeclRefExpr *VarRef, const bool IsPlural) {
  diag(VarRef->getLocation(),
       "because %0 only used when returning the result of this comparison",
       DiagnosticIDs::Note)
      << (IsPlural ? "they are" : "it is");
}

void UnnecessaryIntermediateVarCheck::emitVarDeclRemovalNote(
    const VarDecl *VarDecl1, const DeclStmt *DeclStmt1, const VarDecl *VarDecl2,
    const DeclStmt *DeclStmt2) {
  diag(VarDecl1->getLocation(), "consider removing %0 variable declaration",
       DiagnosticIDs::Note)
      << ((VarDecl2 && DeclStmt2) ? "both this" : "the")
      << FixItHint::CreateRemoval(DeclStmt1->getSourceRange());

  if (VarDecl2 && DeclStmt2) {
    diag(VarDecl2->getLocation(), "and this one", DiagnosticIDs::Note)
        << FixItHint::CreateRemoval(DeclStmt2->getSourceRange());
  }
}

void UnnecessaryIntermediateVarCheck::emitReturnReplacementNote(
    const Expr *LHS, const StringRef LHSReplacement, const Expr *RHS,
    const StringRef RHSReplacement, const BinaryOperator *BinOpToReverse) {
  auto Diag =
      diag(LHS->getLocStart(),
           "and directly using the variable initialization expression%0 here",
           DiagnosticIDs::Note)
      << ((isa<DeclRefExpr>(LHS) && RHS && isa<DeclRefExpr>(RHS)) ? "s" : "")
      << FixItHint::CreateReplacement(LHS->getSourceRange(), LHSReplacement);

  if (RHS) {
    Diag << FixItHint::CreateReplacement(RHS->getSourceRange(), RHSReplacement);
  }

  if (BinOpToReverse) {
    const auto ReversedBinOpText = BinaryOperator::getOpcodeStr(
        BinaryOperator::reverseComparisonOp(BinOpToReverse->getOpcode()));

    Diag << FixItHint::CreateReplacement(
        SourceRange(BinOpToReverse->getOperatorLoc(),
                    BinOpToReverse->getOperatorLoc().getLocWithOffset(
                        BinOpToReverse->getOpcodeStr().size())),
        ReversedBinOpText);
  }
}

void UnnecessaryIntermediateVarCheck::check(
    const MatchFinder::MatchResult &Result) {
  const DeclStmt *DeclarationStmt1 =
      Result.Nodes.getNodeAs<DeclStmt>("declStmt1");
  const Expr *Init1 = Result.Nodes.getNodeAs<Expr>("init1");
  const VarDecl *VariableDecl1 = Result.Nodes.getNodeAs<VarDecl>("varDecl1");
  const DeclRefExpr *VarRefLHS1 =
      Result.Nodes.getNodeAs<DeclRefExpr>("declRefExprLHS1");
  const DeclRefExpr *VarRefRHS1 =
      Result.Nodes.getNodeAs<DeclRefExpr>("declRefExprRHS1");

  // If we already checked this declaration in a 2-decl match, skip it.
  if (CheckedDeclStmt.count(DeclarationStmt1)) {
    return;
  }

  const DeclStmt *DeclarationStmt2 =
      Result.Nodes.getNodeAs<DeclStmt>("declStmt2");
  const Expr *Init2 = Result.Nodes.getNodeAs<Expr>("init2");
  const VarDecl *VariableDecl2 = Result.Nodes.getNodeAs<VarDecl>("varDecl2");
  const DeclRefExpr *VarRefLHS2 =
      Result.Nodes.getNodeAs<DeclRefExpr>("declRefExprLHS2");
  const DeclRefExpr *VarRefRHS2 =
      Result.Nodes.getNodeAs<DeclRefExpr>("declRefExprRHS2");

  // Add the second declaration to the cache to make sure it doesn't get
  // matches individually afterwards.
  CheckedDeclStmt.insert(DeclarationStmt2);

  const ReturnStmt *Return1 = Result.Nodes.getNodeAs<ReturnStmt>("returnStmt1");
  const ReturnStmt *Return2 = Result.Nodes.getNodeAs<ReturnStmt>("returnStmt2");

  const BinaryOperator *BinOp = Result.Nodes.getNodeAs<BinaryOperator>("binOp");

  if (Return1) {
    // This is the case where we only have one variable declaration before the
    // return statement.

    // First we get the source code of the initializer expression of the
    // variable declaration.
    const auto Init1TextOpt =
        utils::lexer::getStmtText(Init1, Result.Context->getSourceManager());
    if (!Init1TextOpt) {
      return;
    }
    auto Init1Text = (*Init1TextOpt).str();

    // If the expression is a binary operator, we wrap it in parentheses to keep
    // the same operator precendence.
    if (isa<BinaryOperator>(Init1->IgnoreImplicit())) {
      Init1Text = "(" + Init1Text + ")";
    }

    // Next we compute the return indentation length and the return length to be
    // able to know what length the return statement will have once the fixes
    // are applied.
    const auto ReturnIndentTextLength =
        Lexer::getIndentationForLine(Return1->getLocStart(),
                                     Result.Context->getSourceManager())
            .size();

    const auto ReturnTextLength =
        (*utils::lexer::getStmtText(Return1,
                                    Result.Context->getSourceManager()))
            .size();

    const auto NewReturnLength = ReturnIndentTextLength + ReturnTextLength -
                                 VariableDecl1->getName().size() +
                                 Init1Text.size();

    // If the new length is over the statement limit, then folding the
    // expression wouldn't really benefit readability. Therefore we abort.
    if (NewReturnLength > MaximumLineLength) {
      return;
    }

    // Otherwise, we're all good and we emit the diagnostics along with the fix
    // it hints.

    emitMainWarning(VariableDecl1);

    if (VarRefLHS1) {
      emitUsageInComparisonNote(VarRefLHS1, false);
      emitVarDeclRemovalNote(VariableDecl1, DeclarationStmt1);
      emitReturnReplacementNote(VarRefLHS1, Init1Text);
    } else if (VarRefRHS1) {
      // If the variable is on the RHS of the comparison, we need to reverse the
      // operands of the binary operator to keep the same execution order.
      const auto LHSTextOpt = utils::lexer::getStmtText(
          BinOp->getLHS(), Result.Context->getSourceManager());
      if (!LHSTextOpt) {
        return;
      }

      emitUsageInComparisonNote(VarRefRHS1, false);
      emitVarDeclRemovalNote(VariableDecl1, DeclarationStmt1);
      emitReturnReplacementNote(BinOp->getLHS(), Init1Text, VarRefRHS1,
                                *LHSTextOpt, BinOp);
    } else {
      return;
    }
  } else if (Return2) {
    // This is the case where there are two variable declarations before the
    // return statement.
    const bool HasVarRef1 = VarRefLHS1 || VarRefRHS1;
    const bool HasVarRef2 = VarRefLHS2 || VarRefRHS2;

    // First we get the source code of the initializer expressions of the
    // variable declarations.
    const auto Init1TextOpt =
        utils::lexer::getStmtText(Init1, Result.Context->getSourceManager());
    if (!Init1TextOpt) {
      return;
    }
    auto Init1Text = (*Init1TextOpt).str();

    const auto Init2TextOpt =
        utils::lexer::getStmtText(Init2, Result.Context->getSourceManager());
    if (!Init2TextOpt) {
      return;
    }
    auto Init2Text = (*Init2TextOpt).str();

    // If the expressiond are binary operators, we wrap them in parentheses to
    // keep the same operator precendence.
    if (isa<BinaryOperator>(Init1->IgnoreImplicit())) {
      Init1Text = "(" + Init1Text + ")";
    }

    if (isa<BinaryOperator>(Init2->IgnoreImplicit())) {
      Init2Text = "(" + Init2Text + ")";
    }

    // Next we compute the return indentation length and the return length to be
    // able to know what length the return statement will have once the fixes
    // are applied.
    const auto ReturnIndentTextLength =
        Lexer::getIndentationForLine(Return2->getLocStart(),
                                     Result.Context->getSourceManager())
            .size();

    const auto ReturnTextLength =
        (*utils::lexer::getStmtText(Return2,
                                    Result.Context->getSourceManager()))
            .size();

    auto NewReturnLength = ReturnIndentTextLength + ReturnTextLength;

    if (HasVarRef1) {
      NewReturnLength -= VariableDecl1->getName().size();
      NewReturnLength += Init1Text.size();
    }

    if (HasVarRef2) {
      NewReturnLength -= VariableDecl2->getName().size();
      NewReturnLength += Init2Text.size();
    }

    // If the new length is over the statement limit, then folding the
    // expression wouldn't really benefit readability. Therefore we abort.
    if (NewReturnLength > MaximumLineLength) {
      return;
    }

    // Otherwise, we're all good and we emit the diagnostics along with the fix
    // it hints.

    if (HasVarRef1 && HasVarRef2) {
      emitMainWarning(VariableDecl1, VariableDecl2);
    } else if (HasVarRef1) {
      emitMainWarning(VariableDecl1);
    } else if (HasVarRef2) {
      emitMainWarning(VariableDecl2);
    }

    if (VarRefLHS1 && VarRefRHS2) {
      emitUsageInComparisonNote(VarRefLHS1, true);
      emitVarDeclRemovalNote(VariableDecl1, DeclarationStmt1, VariableDecl2,
                             DeclarationStmt2);
      emitReturnReplacementNote(VarRefLHS1, Init1Text, VarRefRHS2, Init2Text);
    } else if (VarRefLHS2 && VarRefRHS1) {
      emitUsageInComparisonNote(VarRefLHS2, true);
      emitVarDeclRemovalNote(VariableDecl1, DeclarationStmt1, VariableDecl2,
                             DeclarationStmt2);
      // Here we reverse the operands because we want to keep the same execution
      // order.
      emitReturnReplacementNote(VarRefLHS2, Init1Text, VarRefRHS1, Init2Text,
                                BinOp);
    } else if (VarRefLHS1 && !VarRefRHS2) {
      emitUsageInComparisonNote(VarRefLHS1, false);
      emitVarDeclRemovalNote(VariableDecl1, DeclarationStmt1);
      emitReturnReplacementNote(VarRefLHS1, Init1Text);
    } else if (!VarRefLHS1 && VarRefRHS2) {
      // If the variable is on the RHS of the comparison, we need to reverse the
      // operands of the binary operator to keep the same execution order.
      const auto LHSTextOpt = utils::lexer::getStmtText(
          BinOp->getLHS(), Result.Context->getSourceManager());
      if (!LHSTextOpt) {
        return;
      }

      emitUsageInComparisonNote(VarRefRHS2, false);
      emitVarDeclRemovalNote(VariableDecl2, DeclarationStmt2);
      emitReturnReplacementNote(BinOp->getLHS(), Init2Text, VarRefRHS2,
                                *LHSTextOpt, BinOp);
    } else if (VarRefLHS2 && !VarRefRHS1) {
      emitUsageInComparisonNote(VarRefLHS2, false);
      emitVarDeclRemovalNote(VariableDecl2, DeclarationStmt2);
      emitReturnReplacementNote(VarRefLHS2, Init2Text);
    } else if (!VarRefLHS2 && VarRefRHS1) {
      // If the variable is on the RHS of the comparison, we need to reverse the
      // operands of the binary operator to keep the same execution order.
      const auto LHSTextOpt = utils::lexer::getStmtText(
          BinOp->getLHS(), Result.Context->getSourceManager());
      if (!LHSTextOpt) {
        return;
      }

      emitUsageInComparisonNote(VarRefRHS1, false);
      emitVarDeclRemovalNote(VariableDecl1, DeclarationStmt1);
      emitReturnReplacementNote(BinOp->getLHS(), Init1Text, VarRefRHS1,
                                *LHSTextOpt, BinOp);
    }
  }
}

} // namespace readability
} // namespace tidy
} // namespace clang
