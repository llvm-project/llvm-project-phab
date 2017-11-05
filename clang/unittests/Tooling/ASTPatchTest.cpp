//===- unittest/Tooling/ASTPatchTest.cpp ----------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "clang/Tooling/ASTDiff/ASTPatch.h"
#include "clang/Tooling/ASTDiff/ASTDiff.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Program.h"
#include "gtest/gtest.h"
#include <fstream>

using namespace clang;
using namespace tooling;

std::string ReadShellCommand(const Twine &Command) {
  char Buffer[128];
  std::string Result;
  std::shared_ptr<FILE> Pipe(popen(Command.str().data(), "r"), pclose);
  if (!Pipe)
    return Result;
  while (!feof(Pipe.get())) {
    if (fgets(Buffer, 128, Pipe.get()) != nullptr)
      Result += Buffer;
  }
  return Result;
}

class ASTPatchTest : public ::testing::Test {
  llvm::SmallString<256> TargetFile, ExpectedFile;
  std::array<std::string, 1> TargetFileArray;

public:
  void SetUp() override {
    std::string Suffix = "cpp";
    ASSERT_FALSE(llvm::sys::fs::createTemporaryFile(
        "clang-libtooling-patch-target", Suffix, TargetFile));
    ASSERT_FALSE(llvm::sys::fs::createTemporaryFile(
        "clang-libtooling-patch-expected", Suffix, ExpectedFile));
    TargetFileArray[0] = TargetFile.str();
  }
  void TearDown() override {
    llvm::sys::fs::remove(TargetFile);
    llvm::sys::fs::remove(ExpectedFile);
  }

  void WriteFile(StringRef Filename, StringRef Contents) {
    std::ofstream OS(Filename);
    OS << Contents.str();
    assert(OS.good());
  }

  std::string ReadFile(StringRef Filename) {
    std::ifstream IS(Filename);
    std::stringstream OS;
    OS << IS.rdbuf();
    assert(IS.good());
    return OS.str();
  }

  std::string formatExpected(StringRef Code) {
    WriteFile(ExpectedFile, Code);
    return ReadShellCommand("clang-format " + ExpectedFile);
  }

  llvm::Expected<std::string> patchResult(const char *SrcCode,
                                          const char *DstCode,
                                          const char *TargetCode) {
    std::unique_ptr<ASTUnit> SrcAST = buildASTFromCode(SrcCode),
                             DstAST = buildASTFromCode(DstCode);
    if (!SrcAST || !DstAST) {
      if (!SrcAST)
        llvm::errs() << "Failed to build AST from code:\n" << SrcCode << "\n";
      if (!DstAST)
        llvm::errs() << "Failed to build AST from code:\n" << DstCode << "\n";
      return llvm::make_error<diff::PatchingError>(
          diff::patching_error::failed_to_build_AST);
    }

    diff::SyntaxTree Src(*SrcAST);
    diff::SyntaxTree Dst(*DstAST);

    WriteFile(TargetFile, TargetCode);
    FixedCompilationDatabase Compilations(".", std::vector<std::string>());
    RefactoringTool TargetTool(Compilations, TargetFileArray);
    diff::ComparisonOptions Options;

    if (auto Err = diff::patch(TargetTool, Src, Dst, Options, /*Debug=*/false))
      return std::move(Err);
    return ReadShellCommand("clang-format " + TargetFile);
  }

#define APPEND_NEWLINE(x) x "\n"
// use macros for this to make test failures have proper line numbers
#define PATCH(Src, Dst, Target, ExpectedResult)                                \
  {                                                                            \
    llvm::Expected<std::string> Result = patchResult(                          \
        APPEND_NEWLINE(Src), APPEND_NEWLINE(Dst), APPEND_NEWLINE(Target));     \
    ASSERT_TRUE(bool(Result));                                                 \
    EXPECT_EQ(Result.get(), formatExpected(APPEND_NEWLINE(ExpectedResult)));   \
  }
#define PATCH_ERROR(Src, Dst, Target, ErrorCode)                               \
  {                                                                            \
    llvm::Expected<std::string> Result = patchResult(Src, Dst, Target);        \
    ASSERT_FALSE(bool(Result));                                                \
    llvm::handleAllErrors(Result.takeError(),                                  \
                          [&](const diff::PatchingError &PE) {                 \
                            EXPECT_EQ(PE.get(), ErrorCode);                    \
                          });                                                  \
  }
};

TEST_F(ASTPatchTest, Delete) {
  PATCH(R"(void f() { { int x = 1; } })",
        R"(void f() { })",
        R"(void f() { { int x = 2; } })",
        R"(void f() {  })");
  PATCH(R"(void foo(){})",
        R"()",
        R"(int x; void foo() {;;} int y;)",
        R"(int x;  int y;)");
}
TEST_F(ASTPatchTest, DeleteCallArguments) {
  PATCH(R"(void foo(...); void test1() { foo ( 1 + 1); })",
        R"(void foo(...); void test1() { foo ( ); })",
        R"(void foo(...); void test2() { foo ( 1 + 1 ); })",
        R"(void foo(...); void test2() { foo (  ); })");
}
TEST_F(ASTPatchTest, DeleteParmVarDecl) {
  PATCH(R"(void foo(int a);)",
        R"(void foo();)",
        R"(void bar(int x);)",
        R"(void bar();)");
}
TEST_F(ASTPatchTest, Insert) {
  PATCH(R"(class C {              C() {} };)",
        R"(class C { int b;       C() {} };)",
        R"(class C { int c;       C() {} };)",
        R"(class C { int c;int b; C() {} };)");
  PATCH(R"(class C {        C() {} };)",
        R"(class C { int b; C() {} };)",
        R"(class C {        C() {} };)",
        R"(class C { int b; C() {} };)");
  PATCH(R"(class C { int x;              };)",
        R"(class C { int x;int b;        };)",
        R"(class C { int x ;int c;        };)",
        R"(class C { int x;int b;int c;  };)");
  PATCH(R"(class C { int x;              };)",
        R"(class C { int x;int b;        };)",
        R"(class C { int x; int c;        };)",
        R"(class C { int x;int b;int c;  };)");
  PATCH(R"(class C { int x;              };)",
        R"(class C { int x;int b;        };)",
        R"(class C { int x;int c;        };)",
        R"(class C { int x;int b;int c;  };)");
  PATCH(R"(int a;)",
        R"(int a; int x();)",
        R"(int a;)",
        R"(int a; int x();)");
  PATCH(R"(int a; int b;)",
        R"(int a; int x; int b;)",
        R"(int a; int b;)",
        R"(int a; int x; int b;)");
  PATCH(R"(int b;)",
        R"(int x; int b;)",
        R"(int b;)",
        R"(int x; int b;)");
  PATCH(R"(void f() {   int x = 1 + 1;   })",
        R"(void f() { { int x = 1 + 1; } })",
        R"(void f() {   int x = 1 + 1;   })",
        R"(void f() { { int x = 1 + 1; } })");
}
TEST_F(ASTPatchTest, InsertNoParent) {
  PATCH(R"(void f() { })",
        R"(void f() { int x; })",
        R"()",
        R"()");
}
TEST_F(ASTPatchTest, InsertTopLevel) {
  PATCH(R"(namespace a {})",
        R"(namespace a {} void x();)",
        R"(namespace a {})",
        R"(namespace a {} void x();)");
}
TEST_F(ASTPatchTest, Move) {
  PATCH(R"(namespace a {  void f(){} })",
        R"(namespace a {} void f(){}  )",
        R"(namespace a {  void f(){} })",
        R"(namespace a {} void f(){}  )");
  PATCH(R"(namespace a {  void f(){} } int x;)",
        R"(namespace a {} void f(){}   int x;)",
        R"(namespace a {  void f(){} } int x;)",
        R"(namespace a {} void f(){}   int x;)");
  PATCH(R"(namespace a { namespace { } })",
        R"(namespace a { })",
        R"(namespace a { namespace { } })",
        R"(namespace a { })");
  PATCH(R"(namespace { int x = 1 + 1; })",
        R"(namespace { int x = 1 + 1; int y;})",
        R"(namespace { int x = 1 + 1; })",
        R"(namespace { int x = 1 + 1; int y;})");
  PATCH(R"(namespace { int y; int x = 1 + 1; })",
        R"(namespace { int x = 1 + 1; int y; })",
        R"(namespace { int y; int x = 1 + 1; })",
        R"(namespace { int x = 1 + 1; int y; })");
  PATCH(R"(void f() { ; int x = 1 + 1; })",
        R"(void f() { int x = 1 + 1; ; })",
        R"(void f() { ; int x = 1 + 1; })",
        R"(void f() { int x = 1 + 1; ; })");
  PATCH(R"(void f() { {{;;;}}        })",
        R"(void f() { {{{;;;}}}      })",
        R"(void f() { {{;;;}}        })",
        R"(void f() { {{{;;;}}}      })");
}
TEST_F(ASTPatchTest, MoveNoSource) {
  PATCH(R"(void f() { })",
        R"(void f() { int x; })",
        R"()",
        R"()");
}
TEST_F(ASTPatchTest, MoveNoTarget) {
  PATCH(R"(int x; void f() { })",
        R"(void f() { int x; })",
        R"(int x;)",
        R"()");
}
TEST_F(ASTPatchTest, Newline) {
  PATCH(R"(void f(){
;
})",
        R"(void f(){
;
int x;
})",
        R"(void f(){
;
})",
        R"(void f(){
;
int x;
})");
}
TEST_F(ASTPatchTest, Nothing) {
  PATCH(R"()",
        R"()",
        R"()",
        R"()");
}
TEST_F(ASTPatchTest, Update) {
  PATCH(R"(class A { int x; };)",
        R"(class A { int x; };)",
        R"(class A { int y; };)",
        R"(class A { int y; };)");
}
TEST_F(ASTPatchTest, UpdateMove) {
  PATCH(R"(void f() { { int x = 1; } })",
        R"(void f() { })",
        R"(void f() { { int x = 2; } })",
        R"(void f() {  })");
  PATCH(R"(void f() {;;} namespace {})",
        R"(namespace { void f() {;;} })",
        R"(void g() {;;} namespace {})",
        R"(namespace { void g() {;;} })");
}
