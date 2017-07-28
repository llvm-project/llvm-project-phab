//===- unittests/AST/PostOrderASTVisitor.cpp - Declaration printer tests --===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains tests for the post-order traversing functionality
// of RecursiveASTVisitor.
//
//===----------------------------------------------------------------------===//

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Tooling/Tooling.h"
#include "gtest/gtest.h"
#include <algorithm>

using namespace clang;

namespace {

  class RecordingVisitor
    : public RecursiveASTVisitor<RecordingVisitor> {

    bool VisitPostOrder;
  public:
    explicit RecordingVisitor(bool VisitPostOrder)
      : VisitPostOrder(VisitPostOrder) {
    }

    // List of visited nodes during traversal.
    std::vector<std::string> VisitedNodes;

    bool shouldTraversePostOrder() const { return VisitPostOrder; }

    bool VisitUnaryOperator(UnaryOperator *Op) {
      VisitedNodes.push_back(Op->getOpcodeStr(Op->getOpcode()));
      return true;
    }

    bool VisitBinaryOperator(BinaryOperator *Op) {
      VisitedNodes.push_back(Op->getOpcodeStr());
      return true;
    }

    bool VisitIntegerLiteral(IntegerLiteral *Lit) {
      VisitedNodes.push_back(Lit->getValue().toString(10, false));
      return true;
    }

    bool VisitVarDecl(VarDecl* D) {
      VisitedNodes.push_back(D->getNameAsString());
      return true;
    }

    bool VisitCXXMethodDecl(CXXMethodDecl *D) {
      VisitedNodes.push_back(D->getQualifiedNameAsString());
      return true;
    }

    bool VisitReturnStmt(ReturnStmt *S) {
      VisitedNodes.push_back("return");
      return true;
    }

    bool VisitCXXRecordDecl(CXXRecordDecl *Declaration) {
      VisitedNodes.push_back(Declaration->getQualifiedNameAsString());
      return true;
    }

    bool VisitTemplateTypeParmType(TemplateTypeParmType *T) {
      VisitedNodes.push_back(T->getDecl()->getQualifiedNameAsString());
      return true;
    }
  };


// Serializes the AST. It is not complete! It only serializes the Statement
// and the Declaration nodes.
class ASTSerializerVisitor : public RecursiveASTVisitor<ASTSerializerVisitor> {
private:
  std::vector<void *> &VisitedNodes;
  const bool PostOrderTraverse;

public:
  ASTSerializerVisitor(bool PostOrderTraverse,
                       std::vector<void *> &VisitedNodes)
      : VisitedNodes(VisitedNodes), PostOrderTraverse(PostOrderTraverse) {}

  bool shouldTraversePostOrder() const { return PostOrderTraverse; }

  bool VisitStmt(Stmt *S) {
    VisitedNodes.push_back(S);
    return true;
  }

  bool VisitDecl(Decl *D) {
    VisitedNodes.push_back(D);
    return true;
  }
};

} // anonymous namespace

TEST(RecursiveASTVisitor, PostOrderTraversal) {
  auto ASTUnit = tooling::buildASTFromCode(
    "class A {"
    "  class B {"
    "    int foo() { while(4) { int i = 9; int j = -5; } return (1 + 3) + 2; }"
    "  };"
    "};"
  );
  auto TU = ASTUnit->getASTContext().getTranslationUnitDecl();
  // We traverse the translation unit and store all
  // visited nodes.
  RecordingVisitor Visitor(true);
  Visitor.TraverseTranslationUnitDecl(TU);

  std::vector<std::string> expected = {"4", "9",      "i",         "5",    "-",
                                       "j", "1",      "3",         "+",    "2",
                                       "+", "return", "A::B::foo", "A::B", "A"};
  // Compare the list of actually visited nodes
  // with the expected list of visited nodes.
  ASSERT_EQ(expected.size(), Visitor.VisitedNodes.size());
  for (std::size_t I = 0; I < expected.size(); I++) {
    ASSERT_EQ(expected[I], Visitor.VisitedNodes[I]);
  }
}

TEST(RecursiveASTVisitor, NoPostOrderTraversal) {
  auto ASTUnit = tooling::buildASTFromCode(
    "class A {"
    "  class B {"
    "    int foo() { return 1 + 2; }"
    "  };"
    "};"
  );
  auto TU = ASTUnit->getASTContext().getTranslationUnitDecl();
  // We traverse the translation unit and store all
  // visited nodes.
  RecordingVisitor Visitor(false);
  Visitor.TraverseTranslationUnitDecl(TU);

  std::vector<std::string> expected = {
    "A", "A::B", "A::B::foo", "return", "+", "1", "2"
  };
  // Compare the list of actually visited nodes
  // with the expected list of visited nodes.
  ASSERT_EQ(expected.size(), Visitor.VisitedNodes.size());
  for (std::size_t I = 0; I < expected.size(); I++) {
    ASSERT_EQ(expected[I], Visitor.VisitedNodes[I]);
  }
}

TEST(RecursiveASTVisitor, PrePostComparisonTest) {
  auto ASTUnit = tooling::buildASTFromCode("template <typename> class X {};"
                                           "template class X<int>;");

  auto TU = ASTUnit->getASTContext().getTranslationUnitDecl();

  std::vector<void *> PreorderNodeList, PostorderNodeList;

  ASTSerializerVisitor PreVisitor(false, PreorderNodeList);
  PreVisitor.TraverseTranslationUnitDecl(TU);

  ASTSerializerVisitor PostVisitor(true, PostorderNodeList);
  PostVisitor.TraverseTranslationUnitDecl(TU);

  // The number of visited nodes must be independent of the ordering mode.
  ASSERT_EQ(PreorderNodeList.size(), PostorderNodeList.size());

  std::sort(PreorderNodeList.begin(), PreorderNodeList.end());
  std::sort(PostorderNodeList.begin(), PostorderNodeList.end());

  // Both traversal must visit the same nodes.
  ASSERT_EQ(std::mismatch(PreorderNodeList.begin(), PreorderNodeList.end(),
                          PostorderNodeList.begin())
                .first,
            PreorderNodeList.end());
}
