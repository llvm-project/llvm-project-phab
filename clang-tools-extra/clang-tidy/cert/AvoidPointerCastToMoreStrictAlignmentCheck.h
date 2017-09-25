//===--- AvoidPointerCastToMoreStrictAlignmentCheck.h - clang-tidy*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_CERT_AVOID_POINTER_CAST_TO_MORE_STRICT_ALIGNMENT_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_CERT_AVOID_POINTER_CAST_TO_MORE_STRICT_ALIGNMENT_H

#include "../ClangTidy.h"

namespace clang {
namespace tidy {
namespace cert {

/// This check will give a warning if a pointer value is
/// converted to a pointer type that is more strictly
/// aligned than the referenced type.
///
/// For the user-facing documentation see:
/// http://clang.llvm.org/extra/clang-tidy/checks/cert-exp36-c.html;;
class AvoidPointerCastToMoreStrictAlignmentCheck : public ClangTidyCheck {
public:
  AvoidPointerCastToMoreStrictAlignmentCheck(StringRef Name, ClangTidyContext *Context)
      : ClangTidyCheck(Name, Context) {}
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;
};

} // namespace cert
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_CERT_AVOID_POINTER_CAST_TO_MORE_STRICT_ALIGNMENT_H
