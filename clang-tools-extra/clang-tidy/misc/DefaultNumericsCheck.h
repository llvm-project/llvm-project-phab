//===--- DefaultNumericsCheck.h - clang-tidy---------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MISC_DEFAULT_NUMERICS_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MISC_DEFAULT_NUMERICS_H

#include "../ClangTidy.h"

namespace clang {
namespace tidy {
namespace misc {

/// This check flags usages of ``std::numeric_limits<T>::{min,max}()`` for
/// unspecialized types. It is dangerous because it returns T(), which rarely
/// might be the minimum or the maximum for this type.
///
/// For the user-facing documentation see:
/// http://clang.llvm.org/extra/clang-tidy/checks/misc-default-numerics.html
class DefaultNumericsCheck : public ClangTidyCheck {
public:
  DefaultNumericsCheck(StringRef Name, ClangTidyContext *Context)
      : ClangTidyCheck(Name, Context) {}
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;
};

} // namespace misc
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MISC_DEFAULT_NUMERICS_H
