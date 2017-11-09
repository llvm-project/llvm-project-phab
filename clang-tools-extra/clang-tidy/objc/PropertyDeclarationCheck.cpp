//===--- PropertyDeclarationCheck.cpp - clang-tidy-------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "PropertyDeclarationCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace objc {

namespace {

/// we will do best effort to generate a fix, however, if the
/// case can not be solved with a simple fix (e.g. CamelCase
/// is a simple fix), we will leave the fix to the user.
/// TODO: provide fix for snake_case to snakeCase
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
}  // namespace

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
  diag(MatchedDecl->getLocation(),
       "property '%0' is not in proper format according to property naming "
       "convention")
      << MatchedDecl->getName() << generateFixItHint(MatchedDecl);
}

}  // namespace objc
}  // namespace tidy
}  // namespace clang
