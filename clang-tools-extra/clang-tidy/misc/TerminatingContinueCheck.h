//===--- TerminatingContinueCheck.h - clang-tidy-----------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MISC_TERMINATING_CONTINUE_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MISC_TERMINATING_CONTINUE_H

#include "../ClangTidy.h"

namespace clang {
namespace tidy {
namespace misc {

/// Checks if a 'continue' statement terminates the loop. It does if the loop
/// has false condition.
///
/// For the user-facing documentation see:
/// http://clang.llvm.org/extra/clang-tidy/checks/misc-terminating-continue.html
class TerminatingContinueCheck : public ClangTidyCheck {
public:
  TerminatingContinueCheck(StringRef Name, ClangTidyContext *Context)
      : ClangTidyCheck(Name, Context) {}
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;
};

} // namespace misc
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MISC_TERMINATING_CONTINUE_H
