//===--- CapturedVariables.cpp - Clang refactoring library ----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "CaptureVariables.h"
#include "clang/AST/Decl.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Tooling/Refactoring/ASTSelection.h"
#include "llvm/ADT/DenseMap.h"

using namespace clang;

namespace {

// FIXME: Determine 'const' qualifier.
// FIXME: Track variables defined in the extracted code.
/// Scans the extracted AST to determine which variables have to be captured
/// and passed to the extracted function.
class ExtractedVariableCaptureVisitor
    : public RecursiveASTVisitor<ExtractedVariableCaptureVisitor> {
public:
  struct ExtractedEntityInfo {
    /// True if this entity is used in extracted code.
    bool IsUsed = false;
    /// True if this entity is defined in the extracted code.
    bool IsDefined = false;
  };

  bool VisitDeclRefExpr(const DeclRefExpr *E) {
    const VarDecl *VD = dyn_cast<VarDecl>(E->getDecl());
    if (!VD)
      return true;
    // FIXME: Capture 'self'.
    if (!VD->isLocalVarDeclOrParm())
      return true;
    captureVariable(VD);
    return true;
  }

  bool VisitVarDecl(const VarDecl *VD) {
    captureVariable(VD).IsDefined = true;
    // FIXME: Track more information about variables defined in extracted code
    // to support "use after defined in extracted" situation reasonably well.
    return true;
  }

  const llvm::DenseMap<const VarDecl *, ExtractedEntityInfo> &
  getVisitedVariables() const {
    return Variables;
  }

private:
  ExtractedEntityInfo &captureVariable(const VarDecl *VD) {
    ExtractedEntityInfo &Result = Variables[VD];
    Result.IsUsed = true;
    return Result;
  }

  llvm::DenseMap<const VarDecl *, ExtractedEntityInfo> Variables;
  // TODO: Track fields/this/self.
};

} // end anonymous namespace

namespace clang {
namespace tooling {

void CapturedEntity::printParamDecl(llvm::raw_ostream &OS,
                                    const PrintingPolicy &PP) const {
  switch (Kind) {
  case CapturedVarDecl:
    VD->getType().print(OS, PP, /*PlaceHolder=*/VD->getName());
    break;
  }
}

void CapturedEntity::printFunctionCallArg(llvm::raw_ostream &OS,
                                          const PrintingPolicy &PP) const {
  // FIXME: Take address if needed.
  switch (Kind) {
  case CapturedVarDecl:
    OS << VD->getName();
    break;
  }
}

StringRef CapturedEntity::getName() const {
  switch (Kind) {
  case CapturedVarDecl:
    return VD->getName();
  }
  llvm_unreachable("invalid captured entity kind");
}

/// Scans the extracted AST to determine which variables have to be captured
/// and passed to the extracted function.
std::vector<CapturedEntity>
findCapturedExtractedEntities(const CodeRangeASTSelection &Code) {
  ExtractedVariableCaptureVisitor Visitor;
  for (size_t I = 0, E = Code.size(); I != E; ++I)
    Visitor.TraverseStmt(const_cast<Stmt *>(Code[I]));
  std::vector<CapturedEntity> Entities;
  for (const auto &I : Visitor.getVisitedVariables()) {
    if (!I.getSecond().IsDefined)
      Entities.push_back(CapturedEntity(I.getFirst()));
    // FIXME: Handle variables used after definition in extracted code.
  }
  // Sort the entities by name.
  std::sort(Entities.begin(), Entities.end(),
            [](const CapturedEntity &X, const CapturedEntity &Y) {
              return X.getName() < Y.getName();
            });
  // FIXME: Capture any field if necessary (method -> function extraction).
  // FIXME: Capture 'this' / 'self' if necessary.
  // FIXME: Compute the actual parameter types.
  return Entities;
}

} // end namespace tooling
} // end namespace clang
