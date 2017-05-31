//===--- UndelegatedCopyOfBaseClassesCheck.h - clang-tidy--------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MISC_UNDELEGATED_COPY_OF_BASE_CLASSES_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MISC_UNDELEGATED_COPY_OF_BASE_CLASSES_H

#include "../ClangTidy.h"

namespace clang {
namespace tidy {
namespace misc {

/// Finds copy constructors where the ctor don't call the constructor of the
/// base class.
///
/// For the user-facing documentation see:
/// http://clang.llvm.org/extra/clang-tidy/checks/misc-undelegated-copy-of-base-classes.html
class UndelegatedCopyOfBaseClassesCheck : public ClangTidyCheck {
public:
  UndelegatedCopyOfBaseClassesCheck(StringRef Name, ClangTidyContext *Context)
      : ClangTidyCheck(Name, Context) {}
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;
};

} // namespace misc
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MISC_UNDELEGATED_COPY_OF_BASE_CLASSES_H
