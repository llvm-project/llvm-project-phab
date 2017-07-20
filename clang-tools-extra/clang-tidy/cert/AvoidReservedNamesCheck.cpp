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
#include <vector>

using namespace clang::ast_matchers;

namespace {
const std::vector<StringRef> Keywords = {
#define KEYWORD(X, Y) #X,
#define CXX_KEYWORD_OPERATOR(X, Y) #X,
#define CXX11_KEYWORD(X, Y) #X,
#define CONCEPTS_KEYWORD(X) #X,
#include "clang/Basic/TokenKinds.def"
};
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
    macroNameCheck(MacroNameTok, MD);
  }
  void MacroUndefined(const Token &MacroNameTok, const MacroDefinition &,
                      const MacroDirective *MD) override {
    macroNameCheck(MacroNameTok, MD);
  }

private:
  Preprocessor *PP;
  AvoidReservedNamesCheck *Check;
  bool semanticallyEquivalent(const Token &MNT, const MacroDirective *MD) {
    if (MD->getMacroInfo()->tokens_begin() == MD->getMacroInfo()->tokens_end())
      return false;
    for (auto TI = MD->getMacroInfo()->tokens_begin();
         TI != MD->getMacroInfo()->tokens_end(); ++TI) {
      if (MNT.getIdentifierInfo()->getName() !=
              TI->getIdentifierInfo()->getName() &&
          "__" + MNT.getIdentifierInfo()->getName().str() !=
              TI->getIdentifierInfo()->getName())
        return false;
    }
    return true;
  }
  void macroNameCheck(const Token &MacroNameTok, const MacroDirective *MD) {
    if (MacroNameTok.getLocation().isInvalid() ||
        PP->getSourceManager().isInSystemHeader(MacroNameTok.getLocation()) ||
        semanticallyEquivalent(MacroNameTok, MD))
      return;
    Check->macroNameIsKeywordCheck(MacroNameTok);
    Check->declNameIsReservedCheck(MacroNameTok.getIdentifierInfo()->getName(),
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

  declNameIsReservedCheck(ND->getName(), ND->getLocation(),
                          isInGlobalNamespace(ND));
}

void AvoidReservedNamesCheck::declNameIsReservedCheck(StringRef Name,
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
  if (D->getDeclContext()->isTranslationUnit())
    return true;
  if (const auto *NS = dyn_cast<NamespaceDecl>(D->getDeclContext())) {
    if (!NS->isAnonymousNamespace())
      return false;
    return isInGlobalNamespace(NS);
  }
  return false;
}

void AvoidReservedNamesCheck::macroNameIsKeywordCheck(
    const Token &MacroNameTok) {
  if (Keywords.end() != std::find(Keywords.begin(), Keywords.end(),
                                  MacroNameTok.getIdentifierInfo()->getName()))
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
