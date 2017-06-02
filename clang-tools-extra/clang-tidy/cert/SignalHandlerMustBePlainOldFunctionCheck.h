//===--- SignalHandlerMustBePlainOldFunctionCheck.h - clang-tidy-*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_CERT_SIGNAL_HANDLER_MUST_BE_PLAIN_OLD_FUNCTION_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_CERT_SIGNAL_HANDLER_MUST_BE_PLAIN_OLD_FUNCTION_H

#include "../ClangTidy.h"

namespace clang {
namespace tidy {
namespace cert {

/// A signal handler must be a plain old function.
///
/// For the user-facing documentation see:
/// http://clang.llvm.org/extra/clang-tidy/checks/cert-msc54-cpp.html
class SignalHandlerMustBePlainOldFunctionCheck : public ClangTidyCheck {
public:
  SignalHandlerMustBePlainOldFunctionCheck(StringRef Name, ClangTidyContext *Context)
      : ClangTidyCheck(Name, Context) {}
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;
};

} // namespace cert
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_CERT_SIGNAL_HANDLER_MUST_BE_PLAIN_OLD_FUNCTION_H
