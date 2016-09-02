//===--- tools/extra/clang-refactor/USRFindingAction.cpp - USREngine ------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// \brief Provides an action to find USR for the symbol at <offset>, as well as
/// all additional USRs.
///
//===----------------------------------------------------------------------===//

#include "USRFindingAction.h"
#include "USRFinder.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTTypeTraits.h"
#include "clang/AST/Decl.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/FileManager.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Lex/Lexer.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Refactoring.h"
#include "clang/Tooling/Tooling.h"
#include <algorithm>
#include <set>

using namespace llvm;

namespace clang {
namespace clang_refactor {

namespace {
// \brief NamedDeclFindingConsumer should delegate finding USRs of given Decl to
// AdditionalUSRFinder. AdditionalUSRFinder adds USRs of ctor and dtor if given
// Decl refers to class and adds USRs of all overridden methods if Decl refers
// to virtual method.
class AdditionalUSRFinder : public RecursiveASTVisitor<AdditionalUSRFinder> {
public:
  explicit AdditionalUSRFinder(const Decl *FoundDecl, ASTContext &Context)
      : FoundDecl(FoundDecl), Context(Context) {}

  std::vector<std::string> Find() {
    // Fill OverriddenMethods and PartialSpecs storages.
    TraverseDecl(Context.getTranslationUnitDecl());
    if (const auto *MethodDecl = dyn_cast<CXXMethodDecl>(FoundDecl)) {
      addUSRsOfOverridenFunctions(MethodDecl);
      for (const auto &OverriddenMethod : OverriddenMethods) {
        if (checkIfOverriddenFunctionAscends(OverriddenMethod)) {
          USRSet.insert(getUSRForDecl(OverriddenMethod));
        }
      }
    } else if (const auto *RecordDecl = dyn_cast<CXXRecordDecl>(FoundDecl)) {
      handleCXXRecordDecl(RecordDecl);
    } else if (const auto *TemplateDecl =
                   dyn_cast<ClassTemplateDecl>(FoundDecl)) {
      handleClassTemplateDecl(TemplateDecl);
    } else {
      USRSet.insert(getUSRForDecl(FoundDecl));
    }
    return std::vector<std::string>(USRSet.begin(), USRSet.end());
  }

  bool VisitCXXMethodDecl(const CXXMethodDecl *MethodDecl) {
    if (MethodDecl->isVirtual())
      OverriddenMethods.push_back(MethodDecl);
    return true;
  }

  bool VisitClassTemplatePartialSpecializationDecl(
      const ClassTemplatePartialSpecializationDecl *PartialSpec) {
    PartialSpecs.push_back(PartialSpec);
    return true;
  }

private:
  void handleCXXRecordDecl(const CXXRecordDecl *RecordDecl) {
    RecordDecl = RecordDecl->getDefinition();
    if (const auto *ClassTemplateSpecDecl =
            dyn_cast<ClassTemplateSpecializationDecl>(RecordDecl))
      handleClassTemplateDecl(ClassTemplateSpecDecl->getSpecializedTemplate());

    addUSRsOfCtorDtors(RecordDecl);
  }

  void handleClassTemplateDecl(const ClassTemplateDecl *TemplateDecl) {
    for (const auto *Specialization : TemplateDecl->specializations())
      addUSRsOfCtorDtors(Specialization);

    for (const auto *PartialSpec : PartialSpecs)
      if (PartialSpec->getSpecializedTemplate() == TemplateDecl)
        addUSRsOfCtorDtors(PartialSpec);
    addUSRsOfCtorDtors(TemplateDecl->getTemplatedDecl());
  }

  void addUSRsOfCtorDtors(const CXXRecordDecl *RecordDecl) {
    RecordDecl = RecordDecl->getDefinition();
    for (const auto *CtorDecl : RecordDecl->ctors())
      USRSet.insert(getUSRForDecl(CtorDecl));
    USRSet.insert(getUSRForDecl(RecordDecl->getDestructor()));
    USRSet.insert(getUSRForDecl(RecordDecl));
  }

  void addUSRsOfOverridenFunctions(const CXXMethodDecl *MethodDecl) {
    USRSet.insert(getUSRForDecl(MethodDecl));
    for (const auto &OverriddenMethod : MethodDecl->overridden_methods())
      // Recursively visit each OverridenMethod.
      addUSRsOfOverridenFunctions(OverriddenMethod);
  }

  bool checkIfOverriddenFunctionAscends(const CXXMethodDecl *MethodDecl) {
    for (const auto &OverriddenMethod : MethodDecl->overridden_methods()) {
      if (USRSet.find(getUSRForDecl(OverriddenMethod)) != USRSet.end())
        return true;
      return checkIfOverriddenFunctionAscends(OverriddenMethod);
    }
    return false;
  }

  const Decl *FoundDecl;
  ASTContext &Context;
  std::set<std::string> USRSet;
  std::vector<const CXXMethodDecl *> OverriddenMethods;
  std::vector<const ClassTemplatePartialSpecializationDecl *> PartialSpecs;
};
} // namespace

class NamedDeclFindingConsumer : public ASTConsumer {
public:
  explicit NamedDeclFindingConsumer(
      ArrayRef<unsigned> SymbolOffsets, ArrayRef<std::string> QualifiedNames,
      std::vector<std::string> &SpellingNames,
      std::vector<std::vector<std::string>> &USRList)
      : SymbolOffsets(SymbolOffsets), QualifiedNames(QualifiedNames),
        SpellingNames(SpellingNames), USRList(USRList) {
  }

private:
  void FindSymbol(ASTContext &Context, const SourceManager &SourceMgr,
                  unsigned SymbolOffset, const std::string &QualifiedName) {
    const SourceLocation Point =
        SourceMgr.getLocForStartOfFile(SourceMgr.getMainFileID())
            .getLocWithOffset(SymbolOffset);

    if (!Point.isValid())
      return;
    const NamedDecl *FoundDecl = nullptr;

    if (QualifiedName.empty())
      FoundDecl = getNamedDeclAt(Context, Point);
    else
      FoundDecl = getNamedDeclFor(Context, QualifiedName);

    if (FoundDecl == nullptr) {
      if (QualifiedName.empty()) {
        FullSourceLoc FullLoc(Point, SourceMgr);
        errs() << "clang-refactor rename: could not find symbol at "
               << SourceMgr.getFilename(Point) << ":"
               << FullLoc.getSpellingLineNumber() << ":"
               << FullLoc.getSpellingColumnNumber() << " (offset "
               << SymbolOffset << ").\n";
        exit(1);
      } else {
        errs() << "USREngine: could not find symbol " << QualifiedName << ".\n";
        exit(1);
      }
      return;
    }

    // If FoundDecl is a constructor or destructor, we want to instead take
    // the Decl of the corresponding class.
    if (const auto *CtorDecl = dyn_cast<CXXConstructorDecl>(FoundDecl))
      FoundDecl = CtorDecl->getParent();
    else if (const auto *DtorDecl = dyn_cast<CXXDestructorDecl>(FoundDecl))
      FoundDecl = DtorDecl->getParent();

    SpellingNames.push_back(FoundDecl->getNameAsString());
    AdditionalUSRFinder Finder(FoundDecl, Context);
    USRList.push_back(Finder.Find());
  }

  void HandleTranslationUnit(ASTContext &Context) override {
    const SourceManager &SourceMgr = Context.getSourceManager();
    for (unsigned Offset : SymbolOffsets)
      FindSymbol(Context, SourceMgr, Offset, "");
    for (const std::string &QualifiedName : QualifiedNames)
      FindSymbol(Context, SourceMgr, 0, QualifiedName);
  }

  ArrayRef<unsigned> SymbolOffsets;
  ArrayRef<std::string> QualifiedNames;
  std::vector<std::string> &SpellingNames;
  std::vector<std::vector<std::string>> &USRList;
};

std::unique_ptr<ASTConsumer> USRFindingAction::newASTConsumer() {
  return llvm::make_unique<NamedDeclFindingConsumer>(
      SymbolOffsets, QualifiedNames, SpellingNames, USRList);
}

} // namespace clang_refactor
} // namespace clang
