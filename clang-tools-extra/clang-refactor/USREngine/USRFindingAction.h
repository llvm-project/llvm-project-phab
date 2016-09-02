//===--- tools/extra/clang-refactor/USRFindingAction.h - USREngine --------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// \brief Provides an action to find all relevant USRs at a point.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_REFACTOR_USRENGINE_USRFINDINGACTION_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_REFACTOR_USRENGINE_USRFINDINGACTION_H

#include "clang/AST/ASTTypeTraits.h"
#include "clang/Frontend/FrontendAction.h"

#include <string>
#include <vector>

namespace clang {
class ASTConsumer;
class CompilerInstance;
class NamedDecl;

namespace clang_refactor {

struct USRFindingAction {
  USRFindingAction(ArrayRef<unsigned> SymbolOffsets,
                   ArrayRef<std::string> QualifiedNames)
      : SymbolOffsets(SymbolOffsets), QualifiedNames(QualifiedNames) {}
  std::unique_ptr<ASTConsumer> newASTConsumer();

  const std::vector<std::string> &getUSRSpellings() { return SpellingNames; }
  const std::vector<std::vector<std::string>> &getUSRList() { return USRList; }

private:
  std::vector<unsigned> SymbolOffsets;
  std::vector<std::string> QualifiedNames;
  std::vector<std::string> SpellingNames;
  std::vector<std::vector<std::string>> USRList;
};

} // namespace clang_refactor
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_REFACTOR_USRENGINE_USRFINDINGACTION_H
