//===--- AvoidReservedNamesCheck.h - clang-tidy------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_CERT_AVOID_RESERVED_NAMES_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_CERT_AVOID_RESERVED_NAMES_H

#include "../ClangTidy.h"

namespace clang {
namespace tidy {
namespace cert {

/// This check will catch names declared or defined as a reserved identifier.
///
/// For the user-facing documentation see:
/// http://clang.llvm.org/extra/clang-tidy/checks/cert-dcl51-cpp.html
class AvoidReservedNamesCheck : public ClangTidyCheck {
public:
  AvoidReservedNamesCheck(StringRef Name, ClangTidyContext *Context)
      : ClangTidyCheck(Name, Context) {}
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;
  void declNameIsReservedCheck(StringRef Name, SourceLocation Location, bool IsGlobal);
  void macroNameIsKeywordCheck(const Token &MacroNameTok);
  void registerPPCallbacks(CompilerInstance &Compiler) override;
private:
  bool isInGlobalNamespace(const Decl *D);
};

} // namespace cert
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_CERT_AVOID_RESERVED_NAMES_H
