//===--- ThrowWithNoexceptCheck.h - clang-tidy-------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MISC_THROW_WITH_NOEXCEPT_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MISC_THROW_WITH_NOEXCEPT_H

#include "../ClangTidy.h"

namespace clang {
namespace tidy {
namespace misc {

///\brief Warns about using throw in function declared as noexcept.
/// It complains about every throw, even if it is caught later.
class ThrowWithNoexceptCheck : public ClangTidyCheck {
public:
  ThrowWithNoexceptCheck(StringRef Name, ClangTidyContext *Context)
      : ClangTidyCheck(Name, Context) {}
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;
};

} // namespace misc
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MISC_THROW_WITH_NOEXCEPT_H
