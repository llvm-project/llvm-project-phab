//===--- SuspiciousCallArgumentCheck.h - clang-tidy--------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MISC_SUSPICIOUS_CALL_ARGUMENT_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MISC_SUSPICIOUS_CALL_ARGUMENT_H

#include "../ClangTidy.h"

namespace clang {
namespace tidy {

/// This checker finds those function calls where the function arguments are 
/// provided in an incorrect order. It compares the name of the given variable 
/// to the argument name in the function definition.
/// It issues a message if the given variable name is similar to an another 
/// function argument in a function call. It uses case insensitive comparison.
///
/// For the user-facing documentation see:
/// http://clang.llvm.org/extra/clang-tidy/checks/misc-suspicious-call-argument.html
class SuspiciousCallArgumentCheck : public ClangTidyCheck {
public:
  SuspiciousCallArgumentCheck(StringRef Name, ClangTidyContext *Context)
      : ClangTidyCheck(Name, Context) {}
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;
};

} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MISC_SUSPICIOUS_CALL_ARGUMENT_H
