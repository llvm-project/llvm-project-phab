//===- unittests/AST/TypePrinterTest.cpp ------ TypePrinter printer tests -===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains tests for TypePrinter::print(...).
//
// These tests have a coding convention:
// * variable whose type to be printed is named 'A' unless it should have some
// special name
// * additional helper classes/namespaces/... are 'Z', 'Y', 'X' and so on.
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

class PrintMatch : public MatchFinder::MatchCallback {
  SmallString<1024> Printed;
  unsigned NumFoundDecls;
  bool SuppressUnwrittenScope;
  ScopePrintingKind::ScopePrintingKind Scope;

public:
  explicit PrintMatch(bool suppressUnwrittenScope,
                      ScopePrintingKind::ScopePrintingKind scope)
    : NumFoundDecls(0), SuppressUnwrittenScope(suppressUnwrittenScope),
      Scope(scope) { }

  void run(const MatchFinder::MatchResult &Result) override {
    const ValueDecl *VD = Result.Nodes.getNodeAs<ValueDecl>("id");
    if (!VD)
      return;
    NumFoundDecls++;
    if (NumFoundDecls > 1)
      return;

    llvm::raw_svector_ostream Out(Printed);
    PrintingPolicy Policy = Result.Context->getPrintingPolicy();
    Policy.SuppressUnwrittenScope = SuppressUnwrittenScope;
    Policy.Scope = Scope;
    QualType Type = VD->getType();
    Type.print(Out, Policy);
  }

  StringRef getPrinted() const {
    return Printed;
  }

  unsigned getNumFoundDecls() const {
    return NumFoundDecls;
  }
};

::testing::AssertionResult
PrintedTypeMatches(StringRef Code, const std::vector<std::string> &Args,
                   bool SuppressUnwrittenScope,
                   const DeclarationMatcher &NodeMatch,
                   StringRef ExpectedPrinted, StringRef FileName,
                   ScopePrintingKind::ScopePrintingKind Scope) {
  PrintMatch Printer(SuppressUnwrittenScope, Scope);
  MatchFinder Finder;
  Finder.addMatcher(NodeMatch, &Printer);
  std::unique_ptr<FrontendActionFactory> Factory =
      newFrontendActionFactory(&Finder);

  if (!runToolOnCodeWithArgs(Factory->create(), Code, Args, FileName))
    return testing::AssertionFailure()
        << "Parsing error in \"" << Code.str() << "\"";

  if (Printer.getNumFoundDecls() == 0)
    return testing::AssertionFailure()
        << "Matcher didn't find any value declarations";

  if (Printer.getNumFoundDecls() > 1)
    return testing::AssertionFailure()
        << "Matcher should match only one value declaration "
           "(found " << Printer.getNumFoundDecls() << ")";

  if (Printer.getPrinted() != ExpectedPrinted)
    return ::testing::AssertionFailure()
        << "Expected \"" << ExpectedPrinted.str() << "\", "
           "got \"" << Printer.getPrinted().str() << "\"";

  return ::testing::AssertionSuccess();
}

::testing::AssertionResult
PrintedTypeCXX11MatchesWrapper(StringRef Code, StringRef DeclName,
                               StringRef ExpectedPrinted,
                               ScopePrintingKind::ScopePrintingKind Scope) {
  std::vector<std::string> Args(1, "-std=c++11");
  return PrintedTypeMatches(Code,
                            Args,
                            /*SuppressUnwrittenScope*/ true,
                            valueDecl(hasName(DeclName)).bind("id"),
                            ExpectedPrinted,
                            "input.cc",
                            Scope);
}

::testing::AssertionResult
PrintedTypeCXX11Matches(StringRef Code, StringRef DeclName,
                        StringRef ExpectedPrintedAllScopes,
                        StringRef ExpectedPrintedScopesAsWritten,
                        StringRef ExpectedPrintedNoScopes) {
  std::vector<std::string> Args(1, "-std=c++11");

  // Scope == FullScope
  ::testing::AssertionResult result
      = PrintedTypeCXX11MatchesWrapper(Code, DeclName,
                                       ExpectedPrintedAllScopes,
                                       ScopePrintingKind::FullScope);
  if(!result.operator bool()) {
    return result << ", with: FullScope";
  }

  // TODO: Activate this as soon as something similar to ScopeAsWritten is
  // implemented.
//  // Scope == ScopeAsWritten
//  result =
//      PrintedTypeCXX11MatchesWrapper(Code, DeclName,
//                                     ExpectedPrintedScopesAsWritten,
//                                     ScopePrintingKind::ScopeAsWritten);
//  if(!result.operator bool()) {
//    return result << ", with: ScopeAsWritten";
//  }

  // Scope == SuppressScope
  result =
      PrintedTypeCXX11MatchesWrapper(Code, DeclName,
                                     ExpectedPrintedNoScopes,
                                     ScopePrintingKind::SuppressScope);
  if(!result.operator bool()) {
    return result << ", with: SuppressScope";
  }

  return ::testing::AssertionSuccess();
}

} // unnamed namespace

TEST(TypePrinter, UnwrittenScope) {
  ASSERT_TRUE(PrintedTypeCXX11Matches(
    "class W { };"
    "namespace Z {"
      "class W { };"
      "namespace Y {"
        "class W { };"
        "namespace X {"
          "class W { };"
        "}"
        "W A;"
      "}"
    "}",
    "A",
    "::Z::Y::W",
    "W",
    "W"));
}

TEST(TypePrinter, UnwrittenScopeWithTag) {
  ASSERT_TRUE(PrintedTypeCXX11Matches(
      "class W { };"
      "namespace Z {"
        "class W { };"
        "namespace Y {"
          "class W { };"
          "namespace X {"
            "class W { };"
          "}"
          "class W A;"
        "}"
      "}",
      "A",
      "class ::Z::Y::W",
      "class W",
      "class W"));
}

TEST(TypePrinter, WrittenScopeAbsolute1) {
  ASSERT_TRUE(PrintedTypeCXX11Matches(
    "class W { };"
    "namespace Z {"
      "class W { };"
      "namespace Y {"
        "class W { };"
        "namespace X {"
          "class W { };"
        "}"
        "::Z::Y::W A;"
      "}"
    "}",
    "A",
    "::Z::Y::W",
    "::Z::Y::W",
    "W"));
}

TEST(TypePrinter, WrittenScopeAbsolute2) {
  ASSERT_TRUE(PrintedTypeCXX11Matches(
    "class W { };"
    "namespace Z {"
      "class W { };"
      "namespace Y {"
        "class W { };"
        "namespace X {"
          "class W { };"
        "}"
        "::W A;"
      "}"
    "}",
    "A",
    "::W",
    "::W",
    "W"));
}

TEST(TypePrinter, WrittenScopeNonAbsolute1) {
  ASSERT_TRUE(PrintedTypeCXX11Matches(
    "class W { };"
    "namespace Z {"
      "class W { };"
      "namespace Y {"
        "class W { };"
        "namespace X {"
          "class W { };"
        "}"
        "X::W A;"
      "}"
    "}",
    "A",
    "::Z::Y::X::W",
    "X::W",
    "W"));
}

TEST(TypePrinter, WrittenScopeNonAbsolute2) {
  ASSERT_TRUE(PrintedTypeCXX11Matches(
    "class W { };"
    "namespace Z {"
      "class W { };"
      "namespace Y {"
        "class W { };"
        "namespace X {"
          "class W { };"
        "}"
        "Z::Y::X::W A;"
      "}"
    "}",
    "A",
    "::Z::Y::X::W",
    "Z::Y::X::W",
    "W"));
}

TEST(TypePrinter, WrittenScopeNonAbsolute3) {
  ASSERT_TRUE(PrintedTypeCXX11Matches(
    "class W { };"
    "namespace Z {"
      "class W { };"
      "namespace Y {"
        "class W { };"
        "namespace X {"
          "class W {"
            "Z::W (*A)(W);"
          "};"
        "}"
      "}"
    "}",
    "A",
    "::Z::W (*)(::Z::Y::X::W)",
    "Z::W (*)(W)",
    "W (*)(W)"));
}

TEST(TypePrinter, WrittenScopeNonAbsoluteNested1) {
  ASSERT_TRUE(PrintedTypeCXX11Matches(
    "class W { };"
    "namespace Z {"
      "class W { };"
      "namespace Y {"
        "class W { };"
        "namespace X {"
          "class W { };"
        "}"
        "::W (*(Z::W::* A)(void (W::*)(X::W, W, ::Z::W, int, void (Y::W::*)(int))))();"
      "}"
    "}",
    "A",
    "::W (*(::Z::W::*)(void (::Z::Y::W::*)(::Z::Y::X::W, ::Z::Y::W, ::Z::W, int, void (::Z::Y::W::*)(int))))()",
    //FIXME: The written scope of a member pointer seems not to be in the AST:
    "::W (*("/*Z::*/"W::*)(void (W::*)(X::W, W, ::Z::W, int, void ("/*Y::*/"W::*)(int))))()",
    "W (*(W::*)(void (W::*)(W, W, W, int, void (W::*)(int))))()"));
}


TEST(TypePrinter, TemplClassWithSimpleTemplArg) {
  ASSERT_TRUE(PrintedTypeCXX11Matches(
    "namespace Z { template <typename T> class X { }; class Y { };"
      "X<Y> A;"
    "}",
    "A",
    "::Z::X< ::Z::Y>",
    "X<Y>",
    "X<Y>"));
}

TEST(TypePrinter, TemplClassWithTemplClassTemplArg1) {
  ASSERT_TRUE(PrintedTypeCXX11Matches(
    "namespace Z { template <typename T> class X { public: template <typename T2, typename T3> class Y { }; };"
      "namespace W { namespace V { class U { }; } }"
      "X< X<bool>::Y<int, W::V::U> > A;"
    "}",
    "A",
    "::Z::X< ::Z::X<bool>::Y<int, ::Z::W::V::U> >",
    "X<X<bool>::Y<int, W::V::U> >",
    "X<Y<int, U> >"));
}

TEST(TypePrinter, TypeDef1) {
  ASSERT_TRUE(PrintedTypeCXX11Matches(
      "class W { };"
      "namespace Z {"
        "namespace Y {"
          "class W { };"
          "namespace X {"
            "class W {"
              "public:"
              "class V { };"
            "};"
          "}"
          "typedef X::W WT1;"
        "}"
        "class W { };"
        "typedef Y::WT1 WT2;"
        "WT2::V A;"
      "}",
      "A",
      "::Z::WT2::V",
      "WT2::V",
      "V"));
}

TEST(TypePrinter, TypeDef2) {
  ASSERT_TRUE(PrintedTypeCXX11Matches(
      "class W { };"
      "namespace Z {"
        "namespace Y {"
          "class W { };"
          "namespace X {"
            "class W {"
              "public:"
              "class V { };"
            "};"
          "}"
          "typedef X::W WT1;"
        "}"
        "class W { };"
        "typedef Y::WT1 WT2;"
        "WT2 A;"
      "}",
      "A",
      "::Z::WT2",
      "WT2",
      "WT2"));
}


TEST(TypePrinter, TypeDef3) {
  ASSERT_TRUE(PrintedTypeCXX11Matches(
      "class W { };"
      "namespace Z {"
        "namespace Y {"
          "class W { };"
            "namespace X {"
            "class W {"
              "public:"
              "class V { };"
              "typedef V VT1;"
            "};"
          "}"
          "typedef X::W WT1;"
        "}"
        "class W { };"
        "Y::WT1::VT1 A;"
      "}",
      "A",
      "::Z::Y::WT1::VT1",
      "Y::WT1::VT1",
      "VT1"));
}

TEST(TypePrinter, AnonymousNamespace1) {
  ASSERT_TRUE(PrintedTypeCXX11Matches(
      "namespace Z {"
        "namespace {"
          "namespace {"
            "namespace Y {"
              "namespace {"
                "class X { };"
              "}"
              "X A;"
            "}"
          "}"
        "}"
      "}",
      "A",
      "::Z::Y::X",
      "X",
      "X"));
}

// Template dependent type: Printing of absolute scope not possible.
TEST(TypePrinter, TemplateDependent1) {
  ASSERT_TRUE(PrintedTypeCXX11Matches(
      "template <typename U, typename Y>"
      "struct Z : public Y {"
        "template<typename X> struct V {"
          "class W { };"
          "typename Y::template V<X>::W A;"
        "};"
      "};",
      "A",
      "typename Y::template V<X>::W",
      "typename Y::template V<X>::W",
      "typename W"));
}

TEST(TypePrinter, TemplateDependent2) {
  ASSERT_TRUE(PrintedTypeCXX11Matches(
      "template <typename U, typename Y>"
      "struct Z : public Y {"
        "template<typename X> struct V {"
          "class W { };"
          "V<X>::W A;"
        "};"
      "};",
      "A",
      "::Z::V<X>::W",
      "V<X>::W",
      "W"));
}


// Local class: Printing of absolute scope not possible.
TEST(TypePrinter, LocalClasses1) {
  ASSERT_TRUE(PrintedTypeCXX11Matches(
      "void f() {"
        "class Z {"
          "public: class Y { };"
        "};"
        "Z::Y A;"
      "}",
      "A",
      "Z::Y", // TODO: or "::f()::Z::Y"?
      "Z::Y",
      "Y"));
}
