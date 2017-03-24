//===- Attribute.cpp ------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Example clang plugin which adds an an annotation to file-scope declarations
// with the 'example' attribute.
//
//===----------------------------------------------------------------------===//

#include "clang/AST/ASTContext.h"
#include "clang/AST/Attr.h"
#include "clang/Sema/AttributeList.h"
#include "clang/Sema/Sema.h"
#include "clang/Sema/SemaDiagnostic.h"
#include "llvm/IR/Attributes.h"
using namespace clang;

namespace {

struct ExampleAttrInfo : public ParsedAttrInfo {
  ExampleAttrInfo() {
    Syntax = AttributeList::AS_GNU;
    AttrKind = AttributeList::AT_Annotate;
    OptArgs = 1;
  }
  virtual bool handleDeclAttribute(Sema &S, Decl *D, const AttributeList &Attr) {
    /* Check if the decl is at file scope */
    if (!D->getDeclContext()->isFileContext()) {
      unsigned ID = S.getDiagnostics().getCustomDiagID(
        DiagnosticsEngine::Error,
        "'example' attribute only allowed at file scope");
      S.Diag(Attr.getLoc(), ID);
    }
    /* The attribute can take an optional string argument: check for this */
    StringRef Str = "";
    if (Attr.getNumArgs() > 0) {
      Expr *ArgExpr = Attr.getArgAsExpr(0);
      StringLiteral *Literal =
          dyn_cast<StringLiteral>(ArgExpr->IgnoreParenCasts());
      if (Literal) {
        Str = Literal->getString();
      } else {
        S.Diag(ArgExpr->getLocStart(), diag::err_attribute_argument_type)
          << Attr.getName() << AANT_ArgumentString;
      }
    }
    /* Attach an annotate attribute to the Decl */
    D->addAttr(new (S.Context)AnnotateAttr(Attr.getRange(), S.Context,
                                           "example("+Str.str()+")", 0));
    return true;
  }
};

}

static AttrInfoRegistry::Add<ExampleAttrInfo> X("example", "");
