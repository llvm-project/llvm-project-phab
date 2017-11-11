//===--- PropertyDeclarationCheck.cpp - clang-tidy-------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "PropertyDeclarationCheck.h"
#include "../utils/OptionsUtils.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace objc {

namespace {
constexpr char DefaultSpecialAcronyms[] =
    "ASCII;"
    "PDF;"
    "XML;"
    "HTML;"
    "URL;"
    "RTF;"
    "HTTP;"
    "TIFF;"
    "JPG;"
    "PNG;"
    "GIF;"
    "LZW;"
    "ROM;"
    "RGB;"
    "CMYK;"
    "MIDI;"
    "FTP";

/// For now we will only fix 'CamelCase' property to
/// 'camelCase'. For other cases the users need to
/// come up with a proper name by their own.
/// FIXME: provide fix for snake_case to snakeCase
FixItHint generateFixItHint(const ObjCPropertyDecl *Decl) {
  if (isupper(Decl->getName()[0])) {
    auto NewName = Decl->getName().str();
    NewName[0] = tolower(NewName[0]);
    return FixItHint::CreateReplacement(
        CharSourceRange::getTokenRange(SourceRange(Decl->getLocation())),
        llvm::StringRef(NewName));
  }
  return FixItHint();
}

bool startsWithSpecialAcronyms(StringRef Name,
                               const std::vector<std::string> &Prefixes) {
  for (auto const &Prefix : Prefixes) {
    if (Name.startswith(Prefix)) {
      return true;
    }
  }
  return false;
}
}  // namespace

PropertyDeclarationCheck::PropertyDeclarationCheck(StringRef Name,
                                                   ClangTidyContext *Context)
    : ClangTidyCheck(Name, Context),
      SpecialAcronyms(utils::options::parseStringList(
          Options.get("Acronyms", DefaultSpecialAcronyms))) {}

void PropertyDeclarationCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(
      objcPropertyDecl(
          // the property name should be in Lower Camel Case like
          // 'lowerCamelCase'
          unless(matchesName("::[a-z]+[a-z0-9]*([A-Z][a-z0-9]+)*$")))
          .bind("property"),
      this);
}

void PropertyDeclarationCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *MatchedDecl =
      Result.Nodes.getNodeAs<ObjCPropertyDecl>("property");
  assert(MatchedDecl->getName().size() > 0);
  // Skip the check of lowerCamelCase if the name has prefix of special acronyms
  if (startsWithSpecialAcronyms(MatchedDecl->getName(), SpecialAcronyms)) {
    return;
  }
  diag(MatchedDecl->getLocation(),
       "property '%0' is not in proper format according to property naming "
       "convention. It should be in the format of lowerCamelCase or has "
       "special acronyms")
      << MatchedDecl->getName() << generateFixItHint(MatchedDecl);
}

void PropertyDeclarationCheck::storeOptions(ClangTidyOptions::OptionMap &Opts) {
  Options.store(Opts, "Acronyms",
                utils::options::serializeStringList(SpecialAcronyms));
}

}  // namespace objc
}  // namespace tidy
}  // namespace clang
