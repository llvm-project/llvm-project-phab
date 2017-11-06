//===--- CopyConstructorInitCheck.cpp - clang-tidy-------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "CopyConstructorInitCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Lex/Lexer.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace misc {

void CopyConstructorInitCheck::registerMatchers(MatchFinder *Finder) {
  if (!getLangOpts().CPlusPlus)
    return;

  Finder->addMatcher(
      cxxConstructorDecl(
          isCopyConstructor(),
          hasAnyConstructorInitializer(cxxCtorInitializer(
              isBaseInitializer(),
              withInitializer(cxxConstructExpr(hasDeclaration(
                  cxxConstructorDecl(isDefaultConstructor())))))),
          unless(isInstantiated()))
          .bind("ctor"),
      this);
}

static bool isEmptyClass(const CXXRecordDecl *Class) {
  if (!Class->field_empty())
    return false;
  return true;
}

void CopyConstructorInitCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *Ctor = Result.Nodes.getNodeAs<CXXConstructorDecl>("ctor");
  std::string ParamName = Ctor->getParamDecl(0)->getNameAsString();

  // We want only one warning (and FixIt) for each ctor.
  std::string FixItInitList;
  bool HasRelevantBaseInit = false;
  bool ShouldNotDoFixit = false;
  bool HasWrittenInitializer = false;
  SmallVector<FixItHint, 2> SafeFixIts;
  for (const auto *Init : Ctor->inits()) {
    if (Init->getSourceRange().isValid())
      HasWrittenInitializer = true;
    if (!Init->isBaseInitializer())
      continue;
    const Type *BaseType = Init->getBaseClass();
    if (const auto *TempSpecTy = dyn_cast<TemplateSpecializationType>(BaseType))
      ShouldNotDoFixit = ShouldNotDoFixit || TempSpecTy->isTypeAlias();
    ShouldNotDoFixit = ShouldNotDoFixit || isa<TypedefType>(BaseType);
    const auto *BaseClass = BaseType->getAsCXXRecordDecl()->getDefinition();
    if (isEmptyClass(BaseClass) && BaseClass->forallBases(isEmptyClass))
      continue;
    bool NonCopyableBase = false;
    for (const auto *Ctor : BaseClass->ctors()) {
      if (Ctor->isCopyConstructor() &&
          (Ctor->getAccess() == AS_private || Ctor->isDeleted())) {
        NonCopyableBase = true;
        break;
      }
    }
    if (NonCopyableBase)
      continue;
    const auto *CExpr = dyn_cast<CXXConstructExpr>(Init->getInit());
    if (!CExpr || !CExpr->getConstructor()->isDefaultConstructor())
      continue;
    HasRelevantBaseInit = true;
    // Valid when the initializer is written manually (not generated).
    if (Init->getSourceRange().isValid()) {
      if (!ParamName.empty())
        SafeFixIts.push_back(
            FixItHint::CreateInsertion(CExpr->getLocEnd(), ParamName));
    } else {
      if (Init->getSourceLocation().isMacroID() ||
          Ctor->getLocation().isMacroID() || ShouldNotDoFixit)
        break;
      FixItInitList += BaseClass->getNameAsString();
      FixItInitList += "(" + ParamName + "), ";
    }
  }
  if (!HasRelevantBaseInit)
    return;

  auto Diag = diag(Ctor->getLocation(),
                   "calling a base constructor other than the copy constructor")
              << SafeFixIts;

  if (FixItInitList.empty() || ParamName.empty() || ShouldNotDoFixit)
    return;

  std::string FixItMsg{FixItInitList.substr(0, FixItInitList.size() - 2)};
  SourceLocation FixItLoc;
  // There is no initialization list in this constructor.
  if (!HasWrittenInitializer) {
    FixItLoc = Ctor->getBody()->getLocStart();
    FixItMsg = " : " + FixItMsg;
  } else {
    // We apply the missing ctros at the beginning of the initialization list.
    FixItLoc = (*Ctor->init_begin())->getSourceLocation();
    FixItMsg += ',';
  }
  FixItMsg += ' ';

  Diag << FixItHint::CreateInsertion(FixItLoc, FixItMsg);
} // namespace misc

} // namespace misc
} // namespace tidy
} // namespace clang
