//===- polly/MaximalStaticExpander.h - Expand fully all memory accesses.-*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===------------------------------------------------------------------------------===//

#ifndef POLLY_MSE_H
#define POLLY_MSE_H

#include "polly/ScopPass.h"
#include "llvm/IR/PassManager.h"

namespace polly {
struct MaximalStaticExpandPass
    : public llvm::PassInfoMixin<MaximalStaticExpandPass> {
  llvm::PreservedAnalyses run(Scop &, ScopAnalysisManager &,
                              ScopStandardAnalysisResults &, SPMUpdater &);
};
} // namespace polly

#endif /* POLLY_MSE_H */
