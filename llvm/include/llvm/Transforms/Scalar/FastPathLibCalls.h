//===- FastPathLibCalls.h - Insert fast-path code for lib calls -*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_TRANSFORMS_SCALAR_FASTPATHLIBCALLS_H
#define LLVM_TRANSFORMS_SCALAR_FASTPATHLIBCALLS_H

#include "llvm/IR/PassManager.h"

namespace llvm {

/// Pass that injects fast-path code for calls to known library functions.
///
/// Inject a fast-path code sequence for known library functions where
/// profitable. This pass specifically targets library functions with common
/// code paths that can be profitably "inlined", potentially behind a dynamic
/// test, rather than calling the library function.
///
/// For example, rather than call `memcpy` with a size of 16 bytes, if the size
/// is known to be a multiple of 8 and the pointers suitable aligned we can
/// emit a simple loop for short sizes that will run substantially faster than
/// calling out to a library function. With profile information, we can even
/// adjust thresholds and emit weights on the branches.
class FastPathLibCallsPass : public PassInfoMixin<FastPathLibCallsPass> {
public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM);
};

/// Create a legacy pass analogous to `FastPathLibCallsPass` above.
Pass *createFastPathLibCallsLegacyPass();

} // namespace llvm

#endif // LLVM_TRANSFORMS_SCALAR_FASTPATHLIBCALLS_H
