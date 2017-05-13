//===- unittests/AST/AbsoluteScopeTest.cpp - absolute scope printing test -===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains tests for ScopePrintingKind::FullScope. In each test,
// code is rewritten by changing the type of a variable declaration to
// another class or function type. Without printing the full scopes the
// resulting code would be ill-formed.
//===----------------------------------------------------------------------===//

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/TargetOptions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Parse/ParseAST.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Rewrite/Frontend/Rewriters.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"
#include <string>

using namespace clang;
using namespace tooling;

namespace {

class DeclASTVisitor : public RecursiveASTVisitor<DeclASTVisitor>,
                       public ASTConsumer {
  StringRef VarDeclName;
  StringRef DeclNameOfNewType;
  unsigned int NumFoundVarDecls;
  unsigned int NumFoundDeclsOfNewtype;
  const VarDecl *FoundVarDecl;
  QualType FoundNewType;

public:
  DeclASTVisitor(StringRef declName, StringRef namedDeclNameOfNewType)
      : VarDeclName(declName), DeclNameOfNewType(namedDeclNameOfNewType),
        NumFoundVarDecls(0), NumFoundDeclsOfNewtype(0) {}

  // Look for the variable declaration described by the given name:
  bool VisitVarDecl(VarDecl *VD) {
    if (VD->getNameAsString() == VarDeclName) {
      NumFoundVarDecls++;
      FoundVarDecl = VD;
    }
    return true;
  }

  // Look for the declaration described by the given name and store the type.
  bool VisitTypeDecl(TypeDecl *TD) {
    if (TD->getNameAsString() == DeclNameOfNewType) {
      NumFoundDeclsOfNewtype++;
      FoundNewType = QualType(TD->getTypeForDecl(), 0);
    }
    return true;
  }
  // ... also accept value declarations (e.g. functions) because they have a
  // type too
  bool VisitValueDecl(ValueDecl *VD) {
    if (VD->getNameAsString() == DeclNameOfNewType) {
      NumFoundDeclsOfNewtype++;
      FoundNewType = VD->getType();
    }
    return true;
  }

  virtual bool HandleTopLevelDecl(DeclGroupRef DR) {
    for (DeclGroupRef::iterator i = DR.begin(), e = DR.end(); i != e; ++i) {
      TraverseDecl(*i);
    }
    return true;
  }

  unsigned int getNumFoundVarDecls() const { return NumFoundVarDecls; }

  unsigned int getNumFoundDeclsOfNewType() const {
    return NumFoundDeclsOfNewtype;
  }

  const VarDecl *getFoundVarDecl() const { return FoundVarDecl; }

  QualType getFoundNewType() const { return FoundNewType; }
};

::testing::AssertionResult ChangeTypeOfDeclaration(StringRef Code,
                                                   StringRef VarDeclName,
                                                   StringRef DeclNameOfNewType,
                                                   bool CPP11 = true) {
  CompilerInstance compilerInstance;
  compilerInstance.createDiagnostics();

  LangOptions &lo = compilerInstance.getLangOpts();
  lo.CPlusPlus = 1;
  if (CPP11) {
    lo.CPlusPlus11 = 1;
  }

  auto TO = std::make_shared<TargetOptions>();
  TO->Triple = llvm::sys::getDefaultTargetTriple();
  TargetInfo *TI =
      TargetInfo::CreateTargetInfo(compilerInstance.getDiagnostics(), TO);
  compilerInstance.setTarget(TI);

  compilerInstance.createFileManager();
  compilerInstance.createSourceManager(compilerInstance.getFileManager());
  SourceManager &sourceManager = compilerInstance.getSourceManager();
  compilerInstance.createPreprocessor(TU_Module);
  compilerInstance.createASTContext();

  // Set the main file handled by the source manager to the input code:
  std::unique_ptr<llvm::MemoryBuffer> mb(
      llvm::MemoryBuffer::getMemBufferCopy(Code, "input.cc"));
  sourceManager.setMainFileID(sourceManager.createFileID(std::move(mb)));
  compilerInstance.getDiagnosticClient().BeginSourceFile(
      compilerInstance.getLangOpts(), &compilerInstance.getPreprocessor());

  // Create the declaration visitor:
  DeclASTVisitor declVisitor(VarDeclName, DeclNameOfNewType);

  // Parse the code:
  ParseAST(compilerInstance.getPreprocessor(), &declVisitor,
           compilerInstance.getASTContext());

  // Evaluate the result of the AST traverse:
  if (declVisitor.getNumFoundVarDecls() != 1) {
    return testing::AssertionFailure()
           << "Expected exactly one variable declaration with the name \""
           << VarDeclName << "\" but " << declVisitor.getNumFoundVarDecls()
           << " were found";
  }
  if (declVisitor.getNumFoundDeclsOfNewType() != 1) {
    return testing::AssertionFailure()
           << "Expected exactly one declaration with the name \""
           << DeclNameOfNewType << "\" but "
           << declVisitor.getNumFoundDeclsOfNewType() << " were found";
  }

  // Found information on the basis of which the transformation will take place:
  const VarDecl *foundVarDecl = declVisitor.getFoundVarDecl();
  QualType foundNewType = declVisitor.getFoundNewType();

  Rewriter rewriter;
  rewriter.setSourceMgr(sourceManager, compilerInstance.getLangOpts());

  // Create declaration with the old name and the new type:
  std::string newDeclText;
  llvm::raw_string_ostream newDeclTextStream(newDeclText);
  PrintingPolicy policy = foundVarDecl->getASTContext().getPrintingPolicy();
  policy.PolishForDeclaration = true;
  // The important flag:
  policy.Scope = ScopePrintingKind::FullScope;
  foundNewType.print(newDeclTextStream, policy, VarDeclName);

  // Replace the old declaration placeholder with the new declaration:
  rewriter.ReplaceText(foundVarDecl->getSourceRange(), newDeclTextStream.str());

  const RewriteBuffer *rewriteBuffer =
      rewriter.getRewriteBufferFor(sourceManager.getMainFileID());
  if (!rewriteBuffer) {
    return testing::AssertionFailure() << "No changes have been made";
  } else {
    // Check whether the transformed/rewritten code is valid:
    std::unique_ptr<FrontendActionFactory> Factory =
        newFrontendActionFactory<SyntaxOnlyAction>();
    std::vector<std::string> args(1, "-std=c++11");
    std::string rewrittenCode =
        std::string(rewriteBuffer->begin(), rewriteBuffer->end());
    if (!runToolOnCodeWithArgs(Factory->create(), rewrittenCode, args,
                               "input.cc")) {
      return testing::AssertionFailure()
             << "Parsing error in rewritten code \"" << rewrittenCode << "\"";
    } else {
      return testing::AssertionSuccess();
    }
  }
}

} // unnamed namespace

TEST(AbsoluteScope, Test01) {
  ASSERT_TRUE(ChangeTypeOfDeclaration("namespace Z {"
                                      "  namespace Z { }"
                                      "  namespace Y {"
                                      "    class X3 { };"
                                      "  }"
                                      "  int A;"
                                      "}",
                                      "A", "X3"));
}

TEST(AbsoluteScope, Test02) {
  ASSERT_TRUE(ChangeTypeOfDeclaration("namespace Z {"
                                      "  namespace Y {"
                                      "    class X3 { };"
                                      "    void F(X3) { }"
                                      "  }"
                                      "  namespace Z {"
                                      "    namespace Z { }"
                                      "    int A;"
                                      "  }"
                                      "}",
                                      "A", "F"));
}
