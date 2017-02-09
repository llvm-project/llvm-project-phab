//===-- CanonicalizeAliases.h - Alias Canonicalization Pass -----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file canonicalizes aliases.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_TRANSFORMS_UTILS_CANONICALIZE_ALIASES_H
#define LLVM_TRANSFORMS_UTILS_CANONICALIZE_ALIASES_H

#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"

namespace llvm {

bool canonicalizeAliases(Module &M);

/// Simple pass that canonicalizes aliases.
class CanonicalizeAliases : public PassInfoMixin<CanonicalizeAliases> {
public:
  CanonicalizeAliases() = default;

  PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM);
};

} // end namespace llvm

#endif // LLVM_TRANSFORMS_UTILS_CANONICALIZE_ALIASESH
