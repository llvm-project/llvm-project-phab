//===--- MultiwayPathsCoveredCheck.h - clang-tidy----------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_HICPP_MULTIWAY_PATHS_COVERED_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_HICPP_MULTIWAY_PATHS_COVERED_H

#include "../ClangTidy.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include <iostream>

namespace clang {
namespace tidy {
namespace hicpp {

/// Find occasions, where not all codepaths are explicitly covered in code.
/// This includes 'switch' without a 'default'-branch and 'if'-'else if'-chains
/// without a final 'else'-branch.
///
/// For the user-facing documentation see:
/// http://clang.llvm.org/extra/clang-tidy/checks/hicpp-multiway-paths-covered.html
class MultiwayPathsCoveredCheck : public ClangTidyCheck {
public:
  MultiwayPathsCoveredCheck(StringRef Name, ClangTidyContext *Context)
      : ClangTidyCheck(Name, Context),
        WarnOnMissingElse(Options.get("WarnOnMissingElse", 0)) {}
  void storeOptions(ClangTidyOptions::OptionMap &Opts) override;
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;

private:
  void handleSwitchWithDefault(const SwitchStmt *Switch);
  void handleSwitchWithoutDefault(
      const SwitchStmt *Switch,
      const ast_matchers::MatchFinder::MatchResult &Result);
  /// This option configures if there is a warning on missing 'else'-branches
  /// in 'if-else if'-chains. The default is false, since this option might be
  /// very noisy on particular codebases.
  const bool WarnOnMissingElse;
};

} // namespace hicpp
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_HICPP_MULTIWAY_PATHS_COVERED_H
