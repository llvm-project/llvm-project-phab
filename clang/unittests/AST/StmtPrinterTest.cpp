//===- unittests/AST/StmtPrinterTest.cpp --- Statement printer tests ------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains tests for Stmt::printPretty() and related methods.
//
// Search this file for WRONG to see test cases that are producing something
// completely wrong, invalid C++ or just misleading.
//
// These tests have a coding convention:
// * statements to be printed should be contained within a function named 'A'
//   unless it should have some special name (e.g., 'operator+');
// * additional helper declarations are 'Z', 'Y', 'X' and so on.
//
//===----------------------------------------------------------------------===//

#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/ADT/SmallString.h"
#include "gtest/gtest.h"

using namespace clang;
using namespace ast_matchers;
using namespace tooling;

namespace {

void PrintStmt(raw_ostream &Out, const ASTContext *Context, const Stmt *S) {
  assert(S != nullptr && "Expected non-null Stmt");
  PrintingPolicy Policy = Context->getPrintingPolicy();
  S->printPretty(Out, /*Helper*/ nullptr, Policy);
}

class PrintMatch : public MatchFinder::MatchCallback {
  SmallString<1024> Printed;
  unsigned NumFoundStmts;

public:
  PrintMatch() : NumFoundStmts(0) {}

  void run(const MatchFinder::MatchResult &Result) override {
    const Stmt *S = Result.Nodes.getNodeAs<Stmt>("id");
    if (!S)
      return;
    NumFoundStmts++;
    if (NumFoundStmts > 1)
      return;

    llvm::raw_svector_ostream Out(Printed);
    PrintStmt(Out, Result.Context, S);
  }

  StringRef getPrinted() const {
    return Printed;
  }

  unsigned getNumFoundStmts() const {
    return NumFoundStmts;
  }
};

template <typename T>
::testing::AssertionResult
PrintedStmtMatchesInternal(StringRef Code, const std::vector<std::string> &Args,
                           const T &NodeMatch, StringRef ExpectedPrinted) {
  PrintMatch Printer;
  MatchFinder Finder;
  Finder.addMatcher(NodeMatch, &Printer);
  std::unique_ptr<FrontendActionFactory> Factory(
      newFrontendActionFactory(&Finder));

  if (!runToolOnCodeWithArgs(Factory->create(), Code, Args))
    return testing::AssertionFailure()
      << "Parsing error in \"" << Code.str() << "\"";

  if (Printer.getNumFoundStmts() == 0)
    return testing::AssertionFailure()
        << "Matcher didn't find any statements";

  if (Printer.getNumFoundStmts() > 1)
    return testing::AssertionFailure()
        << "Matcher should match only one statement "
           "(found " << Printer.getNumFoundStmts() << ")";

  if (Printer.getPrinted() != ExpectedPrinted)
    return ::testing::AssertionFailure()
      << "Expected \"" << ExpectedPrinted.str() << "\", "
         "got \"" << Printer.getPrinted().str() << "\"";

  return ::testing::AssertionSuccess();
}

enum class StdVer { CXX98, CXX11, CXX14, CXX17, CXX2a };

DeclarationMatcher FunctionBodyMatcher(StringRef ContainingFunction) {
  return functionDecl(hasName(ContainingFunction),
                      has(compoundStmt(has(stmt().bind("id")))));
}

template <typename T>
::testing::AssertionResult
PrintedStmtMatches(StdVer Standard, StringRef Code,
                   const T &NodeMatch, StringRef ExpectedPrinted) {
  const char *StdOpt;
  switch (Standard) {
  case StdVer::CXX98: StdOpt = "-std=c++98"; break;
  case StdVer::CXX11: StdOpt = "-std=c++11"; break;
  case StdVer::CXX14: StdOpt = "-std=c++14"; break;
  case StdVer::CXX17: StdOpt = "-std=c++17"; break;
  case StdVer::CXX2a: StdOpt = "-std=c++2a"; break;
  }
  return PrintedStmtMatchesInternal(Code, {StdOpt, "-Wno-unused-value"},
                                    NodeMatch, ExpectedPrinted);
}

template <typename T>
::testing::AssertionResult
PrintedStmtMSMatches(StringRef Code,
                     const T &NodeMatch, StringRef ExpectedPrinted) {
  std::vector<std::string> Args = {
    "-std=c++98",
    "-target", "i686-pc-win32",
    "-fms-extensions",
    "-Wno-unused-value",
  };
  return PrintedStmtMatchesInternal(Code, Args, NodeMatch, ExpectedPrinted);
}

} // unnamed namespace

TEST(StmtPrinter, TestIntegerLiteral) {
  ASSERT_TRUE(PrintedStmtMatches(StdVer::CXX98,
    "void A() {"
    "  1, -1, 1U, 1u,"
    "  1L, 1l, -1L, 1UL, 1ul,"
    "  1LL, -1LL, 1ULL;"
    "}",
    FunctionBodyMatcher("A"),
    "1 , -1 , 1U , 1U , "
    "1L , 1L , -1L , 1UL , 1UL , "
    "1LL , -1LL , 1ULL"));
    // Should be: with semicolon
}

TEST(StmtPrinter, TestMSIntegerLiteral) {
  ASSERT_TRUE(PrintedStmtMSMatches(
    "void A() {"
    "  1i8, -1i8, 1ui8, "
    "  1i16, -1i16, 1ui16, "
    "  1i32, -1i32, 1ui32, "
    "  1i64, -1i64, 1ui64;"
    "}",
    FunctionBodyMatcher("A"),
    "1i8 , -1i8 , 1Ui8 , "
    "1i16 , -1i16 , 1Ui16 , "
    "1 , -1 , 1U , "
    "1LL , -1LL , 1ULL"));
    // Should be: with semicolon
}

TEST(StmtPrinter, TestFloatingPointLiteral) {
  ASSERT_TRUE(PrintedStmtMatches(StdVer::CXX98,
    "void A() { 1.0f, -1.0f, 1.0, -1.0, 1.0l, -1.0l; }",
    FunctionBodyMatcher("A"),
    "1.F , -1.F , 1. , -1. , 1.L , -1.L"));
    // Should be: with semicolon
}

TEST(StmtPrinter, TestCXXConversionDeclImplicit) {
  ASSERT_TRUE(PrintedStmtMatches(StdVer::CXX98,
    "struct A {"
      "operator void *();"
      "A operator&(A);"
    "};"
    "void bar(void *);"
    "void foo(A a, A b) {"
    "  bar(a & b);"
    "}",
    cxxMemberCallExpr(anything()).bind("id"),
    "a & b"));
}

TEST(StmtPrinter, TestCXXConversionDeclExplicit) {
  ASSERT_TRUE(PrintedStmtMatches(StdVer::CXX11,
    "struct A {"
      "operator void *();"
      "A operator&(A);"
    "};"
    "void bar(void *);"
    "void foo(A a, A b) {"
    "  auto x = (a & b).operator void *();"
    "}",
    cxxMemberCallExpr(anything()).bind("id"),
    "(a & b)"));
    // WRONG; Should be: (a & b).operator void *()
}

TEST(StmtPrinter, TestCXXLamda) {
  ASSERT_TRUE(PrintedStmtMatches(StdVer::CXX11,
    "void A() {"
    "  auto l = [] { };"
    "}",
    lambdaExpr(anything()).bind("id"),
    "[] {\n"
    "}"));

  ASSERT_TRUE(PrintedStmtMatches(StdVer::CXX11,
    "void A() {"
    "  int a = 0, b = 1;"
    "  auto l = [a,b](int c, float d) { };"
    "}",
    lambdaExpr(anything()).bind("id"),
    "[a, b](int c, float d) {\n"
    "}"));

  ASSERT_TRUE(PrintedStmtMatches(StdVer::CXX14,
    "void A() {"
    "  auto l = [](auto a, int b, auto c, int, auto) { };"
    "}",
    lambdaExpr(anything()).bind("id"),
    "[](auto a, int b, auto c, int, auto) {\n"
    "}"));

  ASSERT_TRUE(PrintedStmtMatches(StdVer::CXX2a,
    "void A() {"
    "  auto l = []<typename T1, class T2, int I,"
    "              template<class, typename> class T3>"
    "           (int a, auto, int, auto d) { };"
    "}",
    lambdaExpr(anything()).bind("id"),
    "[]<typename T1, class T2, int I, template <class, typename> class T3>(int a, auto, int, auto d) {\n"
    "}"));
}
