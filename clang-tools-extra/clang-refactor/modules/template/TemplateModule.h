//===--- RefactoringModuleTemplate.h - clang-refactor -----------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_REFACTOR_REFACTORING_MODULE_TEMPLATE_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_REFACTOR_REFACTORING_MODULE_TEMPLATE_H

#include "../core/RefactoringModule.h"

#include <string>

namespace clang {
namespace clang_refactor {
namespace template_module {

class TemplateModule : public RefactoringModule {
public:
  TemplateModule(const std::string &ModuleName,
                 const std::string &ModuleMetaDesciption)
      : RefactoringModule(ModuleName, ModuleMetaDesciption) {}

  int checkArguments() override;

  int extractRefactoringInformation() override;

  int handleTranslationUnit() override;

  int applyReplacements() override;

  ~TemplateModule() override {}
};

} // end namespace template_module
} // end namespace clang_refactor
} // end namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_REFACTOR_REFACTORING_MODULE_TEMPLATE_H
