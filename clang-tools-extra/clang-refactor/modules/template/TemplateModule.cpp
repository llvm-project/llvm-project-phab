//===--- RefactoringModuleTemplate.cpp - clang-refactor ---------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "TemplateModule.h"

#include <string>

namespace clang {
namespace clang_refactor {
namespace template_module {

int TemplateModule::checkArguments() {
  return 0;
}

int TemplateModule::extractRefactoringInformation() {
  return 0;
}

int TemplateModule::handleTranslationUnit() {
  return 0;
}

int TemplateModule::applyReplacements() {
  return 0;
}

} // end namespace template_module
} // end namespace clang_refactor
} // end namespace clang
