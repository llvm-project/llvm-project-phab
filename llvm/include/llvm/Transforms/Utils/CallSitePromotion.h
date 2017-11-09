//===- CallSitePromotion.h - Utilities for call promotion -------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file declares utilities useful for promoting indirect call sites to
// direct call sites.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_TRANSFORMS_UTILS_CALLSITEPROMOTION_H
#define LLVM_TRANSFORMS_UTILS_CALLSITEPROMOTION_H

#include "llvm/IR/InstrTypes.h"

namespace llvm {

class CallSite;

/// Return true if the given indirect call site can be made to call the given
/// function.
///
/// This function ensures that the number and type of the call site's arguments
/// and return value match those of the given function. If the types do not
/// match exactly, they must at least be bitcast compatible.
bool canPromoteCallSite(CallSite CS, Function *Callee);

/// Promote the given indirect call site to directly call \p Callee.
///
/// If the function type of the call site doesn't match that of the callee,
/// bitcast instructions are inserted where appropriate. If \p Casts is
/// non-null, these bitcasts are collected in the provided container.
void makeCallSiteDirect(CallSite CS, Function *Callee,
                        SmallVectorImpl<CastInst *> *Casts = nullptr);

/// Demote the given direct call site to indirectly call \p CalledValue.
///
/// This function is meant to be used in conjunction with \p
/// makeCallSiteDirect. As such, any bitcast instructions contained in \p
/// Casts, which were created by \p promoteCallSite, will be erased.
void makeCallSiteIndirect(CallSite CS, Value *CalledValue,
                          ArrayRef<CastInst *> Casts);

/// Predicate and clone the given call site.
///
/// This function creates an if-then-else structure at the location of the call
/// site. The "if" condition is determined by the given predicate and values.
/// The original call site is moved into the "then" block, and a clone of the
/// call site is placed in the "else" block. The cloned instruction is returned.
/// If the call site happens to be an invoke instruction, further work is done
/// to fix phi nodes in the invoke's normal and unwind destinations.
///
/// This function is intended to facilitate transformations such as indirect
/// call promotion, where the callee can be changed based on some run-time
/// condition.
Instruction *versionCallSite(CallSite CS, CmpInst::Predicate Pred, Value *LHS,
                             Value *RHS);

} // end namespace llvm

#endif // LLVM_TRANSFORMS_UTILS_CALLSITEPROMOTION_H
