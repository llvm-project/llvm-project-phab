//===--- CaptureVariables.cpp - Clang refactoring library -----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_LIB_TOOLING_REFACTORING_EXTRACT_CAPTURE_VARIABLES_H
#define LLVM_CLANG_LIB_TOOLING_REFACTORING_EXTRACT_CAPTURE_VARIABLES_H

#include "clang/Basic/LLVM.h"
#include <vector>

namespace clang {

struct PrintingPolicy;
class VarDecl;

namespace tooling {

class CodeRangeASTSelection;

/// Represents a variable or a declaration that can be represented using a
/// a variable that was captured in the extracted code and that should be
/// passed to the extracted function as an argument.
class CapturedExtractedEntity {
  enum EntityKind {
    CapturedVarDecl,
    // FIXME: Field/This/Self.
  };

public:
  explicit CapturedExtractedEntity(const VarDecl *VD)
      : Kind(CapturedVarDecl), VD(VD) {}

  /// Print the parameter declaration for the captured entity.
  void printParamDecl(llvm::raw_ostream &OS, const PrintingPolicy &PP) const;

  /// Print the expression that should be used as an argument value when
  /// invoking the extracted function.
  void printUseExpr(llvm::raw_ostream &OS, const PrintingPolicy &PP) const;

  StringRef getName() const;

private:
  EntityKind Kind;
  union {
    const VarDecl *VD;
  };
  // FIXME: Track things like type & qualifiers.
};

/// Scans the extracted AST to determine which variables have to be captured
/// and passed to the extracted function.
std::vector<CapturedExtractedEntity>
findCapturedExtractedEntities(const CodeRangeASTSelection &Code);

} // end namespace tooling
} // end namespace clang

#endif // LLVM_CLANG_LIB_TOOLING_REFACTORING_EXTRACT_CAPTURE_VARIABLES_H
