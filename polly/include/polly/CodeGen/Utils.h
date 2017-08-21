//===- Utils.h - Utility functions for code generation ----------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains utility functions for the code generation.
//===----------------------------------------------------------------------===//

#ifndef POLLY_CODEGEN_UTILS_H
#define POLLY_CODEGEN_UTILS_H

#include <utility>

namespace llvm {
class Pass;
class Value;
class BasicBlock;
class DominatorTree;
class RegionInfo;
class LoopInfo;
class BranchInst;
} // namespace llvm

namespace polly {

class Scop;

using BBPair = std::pair<llvm::BasicBlock *, llvm::BasicBlock *>;
/// Execute a Scop conditionally wrt @p RTC.
///
/// In the CFG the optimized code of the Scop is generated next to the
/// original code. Both the new and the original version of the code remain
/// in the CFG. A branch statement decides which version is executed based on
/// the runtime value of @p RTC.
///
/// Before transformation:
///
///                        bb0
///                         |
///                     orig_scop
///                         |
///                        bb1
///
/// After transformation:
///                        bb0
///                         |
///                  polly.splitBlock
///                     /       \.
///                     |     startBlock
///                     |        |
///               orig_scop   new_scop
///                     \      /
///                      \    /
///                        bb1 (joinBlock)
///
/// @param S   The Scop to execute conditionally.
/// @param P   A reference to the pass calling this function.
/// @param RTC The runtime condition checked before executing the new SCoP.
///
/// @return  An std::pair:
///              - The first element is a BBPair of (StartBlock, EndBlock).
///              - The second element is the BranchInst which conditionally
///                branches to the SCoP based on the RTC.
///
std::pair<BBPair, llvm::BranchInst *>
executeScopConditionally(Scop &S, llvm::Value *RTC, llvm::DominatorTree &DT,
                         llvm::RegionInfo &RI, llvm::LoopInfo &LI);

/// Alternative to llvm::SplitCriticalEdge.
///
/// Creates a new block which branches to Succ. The edge to split is redirected
/// to the new block.
///
/// The issue with llvm::SplitCriticalEdge is that it does nothing if the edge
/// is not critical. The issue with llvm::SplitEdge is that it does not always
/// create the middle block, but reuses Prev/Succ if it can. We always want a
/// new middle block.
///
/// @param Prev   Edge Source BB
/// @param Succ   Edge Destination BB
/// @param Suffix Name to append
/// @param DT     DominatorTree
/// @param LI     LoopInfo
/// @param RI     RegionInfo
llvm::BasicBlock *splitEdge(llvm::BasicBlock *Prev, llvm::BasicBlock *Succ,
                            const char *Suffix, llvm::DominatorTree *DT,
                            llvm::LoopInfo *LI, llvm::RegionInfo *RI);

} // namespace polly
#endif
