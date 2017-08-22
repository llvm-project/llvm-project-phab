//===- unittests/Tooling/ASTPatchTest.cpp ---------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Tooling/ASTDiff/ASTPatch.h"
#include "clang/Tooling/Tooling.h"
#include "gtest/gtest.h"

using namespace clang;
using namespace tooling;
using namespace ast_matchers;

namespace {
template <class T> struct DeleteMatch : public MatchFinder::MatchCallback {
  unsigned NumFound = 0;
  bool Success = true;

  void run(const MatchFinder::MatchResult &Result) override {
    auto *Node = Result.Nodes.getNodeAs<T>("id");
    if (!Node)
      return;
    ++NumFound;
    Success = Success && patch::remove<T>(Node, *Result.Context);
  }
};
} // end anonymous namespace

template <class T, class NodeMatcher>
static testing::AssertionResult
isRemovalSuccessful(const NodeMatcher &StmtMatch, StringRef Code) {
  DeleteMatch<T> Deleter;
  MatchFinder Finder;
  Finder.addMatcher(StmtMatch, &Deleter);
  std::unique_ptr<FrontendActionFactory> Factory(
      newFrontendActionFactory(&Finder));
  if (!runToolOnCode(Factory->create(), Code))
    return testing::AssertionFailure()
           << R"(Parsing error in ")" << Code.str() << R"(")";
  if (Deleter.NumFound == 0)
    return testing::AssertionFailure() << "Matcher didn't find any statements";
  return testing::AssertionResult(Deleter.Success);
}

TEST(ASTPatch, RemoveStmt) {
  ASSERT_TRUE(isRemovalSuccessful<Stmt>(returnStmt().bind("id"),
                                        R"(void x(){ return;})"));
}

TEST(ASTPatch, RemoveDecl) {
  ASSERT_TRUE(isRemovalSuccessful<Decl>(varDecl().bind("id"),
                                        R"(int x = 0;)"));
  ASSERT_TRUE(isRemovalSuccessful<Decl>(functionTemplateDecl().bind("id"), R"(
template <class T> struct pred {};
template <class T> pred<pred<T> > swap();
template <class T> pred<pred<T> > swap();
void swap();
)"));
}
