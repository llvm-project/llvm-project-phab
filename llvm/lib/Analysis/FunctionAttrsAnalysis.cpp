//===- FunctionAttrs.cpp - Pass which marks functions attributes ----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file implements interprocedural passes which walk the
/// call-graph deducing and/or propagating function attributes.
///
//===----------------------------------------------------------------------===//

#include "llvm/Analysis/FunctionAttrsAnalysis.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/BasicAliasAnalysis.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/InstIterator.h"
using namespace llvm;

/// Returns the memory access attribute for function F using AAR for AA results,
/// where SCCNodes is the current SCC.
///
/// If ThisBody is true, this function may examine the function body and will
/// return a result pertaining to this copy of the function. If it is false, the
/// result will be based only on AA results for the function declaration; it
/// will be assumed that some other (perhaps less optimized) version of the
/// function may be selected at link time.
MemoryAccessKind llvm::checkFunctionMemoryAccess(
    const Function &F, bool ThisBody, AAResults &AAR,
    const SCCNodeSet &SCCNodes,
    DenseMap<const Function *, ModRefInfo> *CSModRefMap) {
  FunctionModRefBehavior MRB = AAR.getModRefBehavior(&F);
  if (MRB == FMRB_DoesNotAccessMemory)
    // Already perfect!
    return MAK_ReadNone;

  if (!ThisBody) {
    if (AliasAnalysis::onlyReadsMemory(MRB))
      return MAK_ReadOnly;

    // Conservatively assume it writes to memory.
    return MAK_MayWrite;
  }

  // Scan the function body for instructions that may read or write memory.
  bool ReadsMemory = false;
  for (auto II = inst_begin(F), E = inst_end(F); II != E; ++II) {
    const Instruction *I = &*II;

    // Some instructions can be ignored even if they read or write memory.
    // Detect these now, skipping to the next instruction if one is found.
    ImmutableCallSite CS(cast<const Value>(I));
    if (CS) {
      // Ignore calls to functions in the same SCC, as long as the call sites
      // don't have operand bundles.  Calls with operand bundles are allowed to
      // have memory effects not described by the memory effects of the call
      // target.
      if (!CS.hasOperandBundles() && CS.getCalledFunction() &&
          SCCNodes.count(const_cast<Function *>(CS.getCalledFunction())))
        continue;

      ModRefInfo *SaveModRef = nullptr;
      if (!CS.hasOperandBundles() && CS.getCalledFunction() && CSModRefMap) {
        auto Result =
            CSModRefMap->try_emplace(CS.getCalledFunction(), MRI_NoModRef);
        SaveModRef = &Result.first->second;
      }

      FunctionModRefBehavior MRB = AAR.getModRefBehavior(CS);

      // If the call doesn't access memory, we're done.
      if (!(MRB & MRI_ModRef))
        continue;

      if (!AliasAnalysis::onlyAccessesArgPointees(MRB)) {
        // The call could access any memory. If that includes writes, give up.
        if (MRB & MRI_Mod) {
          if (SaveModRef)
            (*SaveModRef) = MRI_Mod;
          else
            return MAK_MayWrite;
        }
        // If it reads, note it.
        else if (MRB & MRI_Ref) {
          if (SaveModRef && *SaveModRef == MRI_NoModRef)
            (*SaveModRef) = MRI_Ref;
          else
            ReadsMemory = true;
        }
        continue;
      }

      // Check whether all pointer arguments point to local memory, and
      // ignore calls that only access local memory.
      for (auto CI = CS.arg_begin(), CE = CS.arg_end(); CI != CE; ++CI) {
        Value *Arg = *CI;
        if (!Arg->getType()->isPtrOrPtrVectorTy())
          continue;

        AAMDNodes AAInfo;
        I->getAAMetadata(AAInfo);
        MemoryLocation Loc(Arg, MemoryLocation::UnknownSize, AAInfo);

        // Skip accesses to local or constant memory as they don't impact the
        // externally visible mod/ref behavior.
        if (AAR.pointsToConstantMemory(Loc, /*OrLocal=*/true))
          continue;

        if (MRB & MRI_Mod) {
          if (SaveModRef)
            (*SaveModRef) = MRI_Mod;
          else
            // Writes non-local memory.  Give up.
            return MAK_MayWrite;
        } else if (MRB & MRI_Ref) {
          if (SaveModRef && *SaveModRef == MRI_NoModRef)
            (*SaveModRef) = MRI_Ref;
          else
            // Ok, it reads non-local memory.
            ReadsMemory = true;
        }
      }
      continue;
    } else if (const LoadInst *LI = dyn_cast<const LoadInst>(I)) {
      // Ignore non-volatile loads from local memory. (Atomic is okay here.)
      if (!LI->isVolatile()) {
        MemoryLocation Loc = MemoryLocation::get(LI);
        if (AAR.pointsToConstantMemory(Loc, /*OrLocal=*/true))
          continue;
      }
    } else if (const StoreInst *SI = dyn_cast<const StoreInst>(I)) {
      // Ignore non-volatile stores to local memory. (Atomic is okay here.)
      if (!SI->isVolatile()) {
        MemoryLocation Loc = MemoryLocation::get(SI);
        if (AAR.pointsToConstantMemory(Loc, /*OrLocal=*/true))
          continue;
      }
    } else if (const VAArgInst *VI = dyn_cast<const VAArgInst>(I)) {
      // Ignore vaargs on local memory.
      MemoryLocation Loc = MemoryLocation::get(VI);
      if (AAR.pointsToConstantMemory(Loc, /*OrLocal=*/true))
        continue;
    }

    // Any remaining instructions need to be taken seriously!  Check if they
    // read or write memory.
    if (I->mayWriteToMemory())
      // Writes memory.  Just give up.
      return MAK_MayWrite;

    // If this instruction may read memory, remember that.
    ReadsMemory |= I->mayReadFromMemory();
  }

  return ReadsMemory ? MAK_ReadOnly : MAK_ReadNone;
}

MemoryAccessKind llvm::computeFunctionBodyMemoryAccess(
    const Function &F, AAResults &AAR,
    DenseMap<const Function *, ModRefInfo> *CSModRefMap) {
  return checkFunctionMemoryAccess(F, /*ThisBody=*/true, AAR, {}, CSModRefMap);
}
