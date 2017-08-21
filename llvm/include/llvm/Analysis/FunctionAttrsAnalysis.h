//===-- FunctionAttrs.h - Compute function attrs --------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
/// \file
/// Provides analysis for computing function attributes
//===----------------------------------------------------------------------===//

#ifndef LLVM_ANALYSIS_FUNCTIONATTRSANALYSIS_H
#define LLVM_ANALYSIS_FUNCTIONATTRSANALYSIS_H

#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/Analysis/AliasAnalysis.h"

namespace llvm {

class AAResults;

/// The three kinds of memory access relevant to 'readonly' and
/// 'readnone' attributes.
enum MemoryAccessKind { MAK_ReadNone = 0, MAK_ReadOnly = 1, MAK_MayWrite = 2 };

typedef SmallSetVector<Function *, 8> SCCNodeSet;

/// Returns the memory access properties of this copy of the function.
MemoryAccessKind computeFunctionBodyMemoryAccess(
    const Function &F, AAResults &AAR,
    DenseMap<const Function *, ModRefInfo> *CSModRefMap = nullptr);

MemoryAccessKind
checkFunctionMemoryAccess(const Function &F, bool ThisBody, AAResults &AAR,
                          const SCCNodeSet &SCCNodes,
                          DenseMap<const Function *, ModRefInfo> *CSModRefMap);
} // namespace llvm

#endif // LLVM_ANALYSIS_FUNCTIONATTRSANALYSIS_H
