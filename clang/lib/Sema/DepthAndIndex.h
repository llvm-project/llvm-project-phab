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
#include "clang/Sema/Template.h"

/// Retrieve the depth and index of a template parameter.
std::pair<unsigned, unsigned>
getDepthAndIndex(NamedDecl *ND) {
  if (TemplateTypeParmDecl *TTP = dyn_cast<TemplateTypeParmDecl>(ND))
    return std::make_pair(TTP->getDepth(), TTP->getIndex());

  if (NonTypeTemplateParmDecl *NTTP = dyn_cast<NonTypeTemplateParmDecl>(ND))
    return std::make_pair(NTTP->getDepth(), NTTP->getIndex());

  TemplateTemplateParmDecl *TTP = cast<TemplateTemplateParmDecl>(ND);
  return std::make_pair(TTP->getDepth(), TTP->getIndex());
}


/// Retrieve the depth and index of an unexpanded parameter pack.
std::pair<unsigned, unsigned>
getDepthAndIndex(UnexpandedParameterPack UPP) {
  if (const TemplateTypeParmType *TTP
                          = UPP.first.dyn_cast<const TemplateTypeParmType *>())
    return std::make_pair(TTP->getDepth(), TTP->getIndex());

  return getDepthAndIndex(UPP.first.get<NamedDecl *>());
}

#endif // LLVM_CLANG_LIB_SEMA_DEPTHANDINDEX_H