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
#include <array>

using namespace clang::ast_matchers;

namespace {
std::array<StringRef, 93> Keywords = {"override",
                                      "final",
                                      "noreturn",
                                      "carries_dependency",
                                      "deprecated",
                                      "fallthrough",
                                      "nodiscard",
                                      "maybe_unused",
                                      "optimize_for_synchronized",
                                      "alignas",
                                      "alignof",
                                      "and",
                                      "and_eq",
                                      "asm",
                                      "auto",
                                      "bitand",
                                      "bitor",
                                      "bool",
                                      "break",
                                      "case",
                                      "catch",
                                      "char",
                                      "char16_t",
                                      "char32_t",
                                      "class",
                                      "compl",
                                      "const",
                                      "constexpr",
                                      "const_cast",
                                      "continue",
                                      "decltype",
                                      "default",
                                      "delete",
                                      "do",
                                      "double",
                                      "dynamic_cast",
                                      "else",
                                      "enum",
                                      "explicit",
                                      "export",
                                      "extern",
                                      "false",
                                      "float",
                                      "for",
                                      "friend",
                                      "goto",
                                      "if",
                                      "inline",
                                      "int",
                                      "long",
                                      "mutable",
                                      "namespace",
                                      "new",
                                      "noexcept",
                                      "not",
                                      "not_eq",
                                      "nullptr",
                                      "operator",
                                      "or",
                                      "or_eq",
                                      "private",
                                      "protected",
                                      "public",
                                      "register",
                                      "reinterpret_cast",
                                      "return",
                                      "short",
                                      "signed",
                                      "sizeof",
                                      "static",
                                      "static_assert",
                                      "static_cast",
                                      "struct",
                                      "switch",
                                      "template",
                                      "this",
                                      "thread_local",
                                      "throw",
                                      "true",
                                      "try",
                                      "typedef",
                                      "typeid",
                                      "typename",
                                      "union",
                                      "unsigned",
                                      "using",
                                      "virtual",
                                      "void",
                                      "volatile",
                                      "wchar_t",
                                      "while",
                                      "xor",
                                      "xor_eq"};
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
    macroNameCheck(MacroNameTok);
  }
  void MacroUndefined(const Token &MacroNameTok,
                      const MacroDefinition &MD) override {
    macroNameCheck(MacroNameTok);
  }

private:
  Preprocessor *PP;
  AvoidReservedNamesCheck *Check;
  void macroNameCheck(const Token &MacroNameTok) {
    if (!MacroNameTok.getLocation().isValid() ||
        PP->getSourceManager().isInSystemHeader(MacroNameTok.getLocation()))
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

  if (!ND->getLocation().isValid() ||
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
    diag(Location, "do not use names that contain a double underscore");
  if (IsGlobal && Name.startswith("_"))
    diag(Location, "do not use global names that start with an underscore");
  if (Name.startswith("_") && Name.size() > 1 && std::isupper(Name[1]))
    diag(Location, "do not use names that begin with an "
                   "underscore followed by an uppercase "
                   "letter");
}

bool AvoidReservedNamesCheck::isInGlobalNamespace(const Decl *D) {
  if (D->getDeclContext()->isTranslationUnit())
    return true;
  if (auto NS = dyn_cast<NamespaceDecl>(D->getDeclContext())) {
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
    diag(MacroNameTok.getLocation(), "do not use macro names which are "
                                     "identical to keywords or attribute "
                                     "tokens");
}

void AvoidReservedNamesCheck::registerPPCallbacks(CompilerInstance &Compiler) {
  Compiler.getPreprocessor().addPPCallbacks(
      llvm::make_unique<AvoidReservedNamesPPCallbacks>(
          &Compiler.getPreprocessor(), this));
}

} // namespace cert
} // namespace tidy
} // namespace clang
