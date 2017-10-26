//===- InlineDeclMembers.h - Decl.h Members that must be inlined -*- C++ -*-==//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file defines inline members of classes defined in Decl.h that have
//  (circular) dependencies on other files and should only be included
//  in .cpp files (as opposed to .h).
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_AST_INLINEDECLMEMBERS_H
#define LLVM_CLANG_AST_INLINEDECLMEMBERS_H

#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
namespace clang {

inline bool FunctionDecl::willHaveBody() const {
  assert(!isa<CXXDeductionGuideDecl>(this) &&
         "must not be called on a deduction guide since we share this data "
         "member while giving it different semantics");
  return WillHaveBody;
}

inline void FunctionDecl::setWillHaveBody(bool V) {
  assert(!isa<CXXDeductionGuideDecl>(this) &&
         "must not be called on a deduction guide since we share this data "
         "member while giving it different semantics");
  WillHaveBody = V;
}

} // namespace clang

#endif // LLVM_CLANG_AST_INLINEDECLMEMBERS_H
