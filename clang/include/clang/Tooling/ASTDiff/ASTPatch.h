//===- ASTPatch.h - AST patching ------------------------------*- C++ -*- -===//
//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLING_ASTDIFF_ASTPATCH_H
#define LLVM_CLANG_TOOLING_ASTDIFF_ASTPATCH_H

#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTTypeTraits.h"

using namespace clang;
using namespace ast_type_traits;

namespace clang {
namespace patch {

bool remove(Decl *D, ASTContext &Context) {
  auto *Ctx = D->getLexicalDeclContext();
  if (!Ctx->containsDecl(D))
    return false;
  Ctx->removeDecl(D);
  return true;
}

static bool removeFrom(Stmt *S, ASTContext &Context, DynTypedNode Parent) {
  auto *ConstParentS = Parent.template get<Stmt>();
  if (!ConstParentS)
    return false;
  auto *ParentS = const_cast<Stmt *>(ConstParentS);
  if (auto *CS = dyn_cast<CompoundStmt>(ParentS)) {
    std::vector<Stmt *> Stmts(CS->child_begin(), CS->child_end());
    auto End = Stmts.end();
    Stmts.erase(std::remove(Stmts.begin(), Stmts.end(), S), Stmts.end());
    CS->setStmts(Context, Stmts);
    return Stmts.end() != End;
  }
  return false;
}

bool remove(Stmt *Node, ASTContext &Context) {
  if (!Node)
    return false;
  auto DTN = DynTypedNode::create(*Node);
  const auto &Parents = Context.getParents(DTN);
  if (Parents.empty())
    return false;
  auto &Parent = Parents[0];
  return removeFrom(Node, Context, Parent);
}

template <class T> bool remove(const T *N, ASTContext &Context) {
  return remove(const_cast<T *>(N), Context);
}

bool remove(DynTypedNode DTN, ASTContext &Context) {
  if (auto *S = DTN.get<Stmt>())
    return remove(S, Context);
  if (auto *D = DTN.get<Decl>())
    return remove(D, Context);
  return false;
}
}
}
#endif
