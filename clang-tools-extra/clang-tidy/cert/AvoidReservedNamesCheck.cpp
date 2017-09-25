//===--- AvoidReservedNamesCheck.cpp - clang-tidy--------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "AvoidReservedNamesCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/PPCallbacks.h"
#include "clang/Lex/Preprocessor.h"
#include <algorithm>

using namespace clang::ast_matchers;

namespace {
static bool IsKeyword(StringRef Name) {
  const static llvm::StringSet<> Keywords = {
#define KEYWORD(X, Y) #X,
#define CXX_KEYWORD_OPERATOR(X, Y) #X,
#define CXX11_KEYWORD(X, Y) #X,
#define CONCEPTS_KEYWORD(X) #X,
#include "clang/Basic/TokenKinds.def"
      "noreturn",
      "carries_dependency",
      "deprecated",
      "fallthrough",
      "nodiscard",
      "maybe_unused",
      "optimize_for_synchronized"};
  return Keywords.count(Name) != 0;
}
} // namespace

namespace clang {
namespace tidy {
namespace cert {

namespace {
class AvoidReservedNamesPPCallbacks : public PPCallbacks {
public:
  AvoidReservedNamesPPCallbacks(Preprocessor *PP,
                                AvoidReservedNamesCheck *Check)
      : PP(PP), Check(Check) {}

  void MacroDefined(const Token &MacroNameTok,
                    const MacroDirective *MD) override {
    checkMacroName(MacroNameTok, MD);
  }
  void MacroUndefined(const Token &MacroNameTok, const MacroDefinition &,
                      const MacroDirective *MD) override {
    checkMacroName(MacroNameTok, MD);
  }

private:
  Preprocessor *PP;
  AvoidReservedNamesCheck *Check;
  bool identical(const StringRef Name, const StringRef Alias) {
    if (Name == Alias ||
        (Alias.endswith(Name) && Alias.size() == (Name.size() + 2) &&
         Alias.startswith("__")))
      return true;
    return false;
  }
  bool semanticallyEquivalent(const Token &MNT, const MacroDirective *MD) {
    const auto &Tokens = MD->getMacroInfo()->tokens();
    return Tokens.size() == 1 &&
           identical(MNT.getIdentifierInfo()->getName(),
                     Tokens.front().getIdentifierInfo()->getName());
  }
  void checkMacroName(const Token &MacroNameTok, const MacroDirective *MD) {
    // Semantically equivalent:
    // #define const const
    // #definte const __const
    if (MacroNameTok.getLocation().isInvalid() ||
        PP->getSourceManager().isInSystemHeader(MacroNameTok.getLocation()) ||
        semanticallyEquivalent(MacroNameTok, MD))
      return;
    Check->checkMacroNameIsKeyword(MacroNameTok);
    Check->checkDeclNameIsReserved(MacroNameTok.getIdentifierInfo()->getName(),
                                   MacroNameTok.getLocation(), true);
  }
};
} // namespace

void AvoidReservedNamesCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(namedDecl().bind("var"), this);
  Finder->addMatcher(labelStmt().bind("label"), this);
}

void AvoidReservedNamesCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *ND = Result.Nodes.getNodeAs<NamedDecl>("var");

  if (const auto *L = Result.Nodes.getNodeAs<LabelStmt>("label"))
    ND = L->getDecl();

  if (ND->isImplicit())
    return;

  if (ND->getLocation().isInvalid() ||
      Result.SourceManager->isInSystemHeader(ND->getLocation()) ||
      !ND->getIdentifier())
    return;

  checkDeclNameIsReserved(ND->getName(), ND->getLocation(),
                          isInGlobalNamespace(ND));
}

void AvoidReservedNamesCheck::checkDeclNameIsReserved(StringRef Name,
                                                      SourceLocation Location,
                                                      bool IsGlobal) {

  if (Name.find("__") != StringRef::npos)
    diag(Location, "identifiers containing double underscores are reserved to "
                   "the implementation");
  if (IsGlobal && Name.startswith("_"))
    diag(Location, "do not use global names that start with an underscore");
  if (Name.startswith("_") && Name.size() > 1 && std::isupper(Name[1]))
    diag(Location, "identifiers that begin with an underscore followed by an "
                   "uppercase letter are reserved to the implementation");
}

bool AvoidReservedNamesCheck::isInGlobalNamespace(const Decl *D) {
  const DeclContext *DC = D->getDeclContext();
  while (const auto *NS = dyn_cast<NamespaceDecl>(DC)) {
    if (!NS->isAnonymousNamespace())
        return false;
    DC = NS->getDeclContext();
  }
  return DC->isTranslationUnit();
}

void AvoidReservedNamesCheck::checkMacroNameIsKeyword(
    const Token &MacroNameTok) {
  if (IsKeyword(MacroNameTok.getIdentifierInfo()->getName()))
    diag(MacroNameTok.getLocation(), "do not use a macro name that is "
                                     "identical to a keyword or an attribute");
}

void AvoidReservedNamesCheck::registerPPCallbacks(CompilerInstance &Compiler) {
  Compiler.getPreprocessor().addPPCallbacks(
      llvm::make_unique<AvoidReservedNamesPPCallbacks>(
          &Compiler.getPreprocessor(), this));
}

} // namespace cert
} // namespace tidy
} // namespace clang
