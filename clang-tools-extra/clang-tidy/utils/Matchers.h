//===--- Matchers.h - clang-tidy-------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_UTILS_MATCHERS_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_UTILS_MATCHERS_H

#include "TypeTraits.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Analysis/CFG.h"

namespace clang {
namespace tidy {
namespace matchers {

AST_MATCHER(BinaryOperator, isRelationalOperator) {
  return Node.isRelationalOp();
}

AST_MATCHER(BinaryOperator, isEqualityOperator) { return Node.isEqualityOp(); }

AST_MATCHER(BinaryOperator, isComparisonOperator) {
  return Node.isComparisonOp();
}

AST_MATCHER(QualType, isExpensiveToCopy) {
  llvm::Optional<bool> IsExpensive =
      utils::type_traits::isExpensiveToCopy(Node, Finder->getASTContext());
  return IsExpensive && *IsExpensive;
}

AST_MATCHER(RecordDecl, isTriviallyDefaultConstructible) {
  return utils::type_traits::recordIsTriviallyDefaultConstructible(
      Node, Finder->getASTContext());
}

// Returns QualType matcher for references to const.
AST_MATCHER_FUNCTION(ast_matchers::TypeMatcher, isReferenceToConst) {
  using namespace ast_matchers;
  return referenceType(pointee(qualType(isConstQualified())));
}

// Matches the next statement within the parent statement sequence.
AST_MATCHER_P(Stmt, hasSuccessor,
              ast_matchers::internal::Matcher<Stmt>, InnerMatcher) {
  using namespace ast_matchers;

  // We get the first parent, making sure that we're not in a case statement
  // not in a compound statement directly inside a switch, because this causes
  // the buildCFG call to crash.
  auto Parent = selectFirst<Stmt>(
    "parent",
    match(
      stmt(hasAncestor(stmt(
        unless(caseStmt()),
        unless(compoundStmt(hasParent(switchStmt()))),
        stmt().bind("parent")))),
    Node, Finder->getASTContext()));

  // We build a Control Flow Graph (CFG) from the parent statement.
  std::unique_ptr<CFG> StatementCFG =
    CFG::buildCFG(nullptr, const_cast<Stmt*>(Parent), &Finder->getASTContext(),
                  CFG::BuildOptions());

  if (!StatementCFG) {
    return false;
  }

  // We iterate through all the CFGBlocks, which basically means that we go over
  // all the possible branches of the code and therefore cover all statements.
  for (auto& Block : *StatementCFG) {
    if (!Block) {
      continue;
    }

    // We iterate through all the statements of the block.
    bool ReturnNextStmt = false;
    for (auto& BlockItem : *Block) {
      Optional<CFGStmt> CFGStatement = BlockItem.getAs<CFGStmt>();
      if (!CFGStatement) {
        if (ReturnNextStmt) {
          return false;
        }

        continue;
      }

      // If we found the next statement, we apply the inner matcher and return
      // the result.
      if (ReturnNextStmt) {
        return InnerMatcher.matches(*CFGStatement->getStmt(), Finder, Builder);
      }

      if (CFGStatement->getStmt() == &Node) {
        ReturnNextStmt = true;
      }
    }
  }

  // If we didn't find a successor, we just return false.
  return false;
}

} // namespace matchers
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_UTILS_MATCHERS_H
