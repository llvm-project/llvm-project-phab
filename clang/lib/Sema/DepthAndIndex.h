//===- DepthAndIndex.h - Static declaration of the getDepthAndIndex function -*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//===----------------------------------------------------------------------===//
//
//  This file defines getDepthAndIndex function.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_LIB_SEMA_DEPTHANDINDEX_H
#define LLVM_CLANG_LIB_SEMA_DEPTHANDINDEX_H

#include "clang/AST/DeclTemplate.h"
#include "clang/Sema/DeclSpec.h"

/// \brief Retrieve the depth and index of a template parameter.
static std::pair<unsigned, unsigned>
getDepthAndIndex(NamedDecl *ND) {
  if (TemplateTypeParmDecl *TTP = dyn_cast<TemplateTypeParmDecl>(ND))
    return std::make_pair(TTP->getDepth(), TTP->getIndex());

  if (NonTypeTemplateParmDecl *NTTP = dyn_cast<NonTypeTemplateParmDecl>(ND))
    return std::make_pair(NTTP->getDepth(), NTTP->getIndex());

  TemplateTemplateParmDecl *TTP = cast<TemplateTemplateParmDecl>(ND);
  return std::make_pair(TTP->getDepth(), TTP->getIndex());
}

#endif // LLVM_CLANG_LIB_SEMA_DEPTHANDINDEX_H