//===- SpeculateAroundPHIs.h - Speculate around PHIs ------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_TRANSFORMS_SCALAR_SPECULATEAROUNDPHIS_H
#define LLVM_TRANSFORMS_SCALAR_SPECULATEAROUNDPHIS_H

#include "llvm/ADT/SetVector.h"
#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Support/Compiler.h"
#include <vector>

namespace llvm {

/// This pass handles speculating instructions around PHIs when doing so is
/// profitable.
///
/// The motivating example are PHIs of constants which will require
/// materializing the constants along each edge. If the PHI is used by an
/// instruction where the target can materialize the constant as parte of the
/// instruction, it is profitable to speculate those instructions around the
/// PHI node. This can reduce totaly dynamic instruction count as well as
/// decrease register pressure.
struct SpeculateAroundPHIsPass : PassInfoMixin<SpeculateAroundPHIsPass> {
  /// \brief Run the pass over the function.
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
};

} // end namespace llvm

#endif // LLVM_TRANSFORMS_SCALAR_SPECULATEAROUNDPHIS_H
