//===--- AssertionCountCheck.h - clang-tidy----------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MISC_ASSERTION_COUNT_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MISC_ASSERTION_COUNT_H

#include "../ClangTidy.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include <string>
#include <vector>

namespace clang {
namespace tidy {
namespace misc {

/// Allows to impose limits on the lines-per-assertions ratio for functions.
///
/// For the user-facing documentation see:
/// http://clang.llvm.org/extra/clang-tidy/checks/misc-assertion-count.html
class AssertionCountCheck : public ClangTidyCheck {
public:
  AssertionCountCheck(StringRef Name, ClangTidyContext *Context);

  void storeOptions(ClangTidyOptions::OptionMap &Opts) override;

  // with certain config parameters, this chech will never output a warning.
  // this function allows to skip this check entirely in such a case
  bool isNOP() const;

  void registerPPCallbacks(CompilerInstance &Compiler) override;
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;

  // given this much lines, what is the minimal number of assertions to have?
  unsigned expectedAssertionsMin(unsigned Lines) const;

  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;

private:
  const unsigned LineThreshold;
  const unsigned AssertsThreshold;
  const unsigned LinesStep;
  const unsigned AssertsStep;
  const std::string RawAssertList;
  SmallVector<StringRef, 5> AssertNames;

  std::vector<SourceRange> FoundAssertions;
};

} // namespace misc
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MISC_ASSERTION_COUNT_H
