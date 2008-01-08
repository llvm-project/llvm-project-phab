//=-- ExplodedNode.h - Local, Path-Sensitive Supergraph Vertices -*- C++ -*--=//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file defines the template class ExplodedNode which is used to
//  represent a node in the location*state "exploded graph" of an
//  intra-procedural, path-sensitive dataflow analysis.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_ANALYSIS_EXPLODEDNODE
#define LLVM_CLANG_ANALYSIS_EXPLODEDNODE

#include "clang/Analysis/ProgramEdge.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/FoldingSet.h"
#include "llvm/ADT/GraphTraits.h"
#include "llvm/ADT/DepthFirstIterator.h"

namespace clang {
  
class ReachabilityEngineImpl;

class ExplodedNodeImpl : public llvm::FoldingSetNode {
protected:
  friend class ReachabilityEngineImpl;
  
  /// nodeID - A unique ID for the node.  This number indicates the
  ///  creation order of vertices, with lower numbers being created first.
  ///  The first created node has nodeID == 0.
  const unsigned nodeID;
  
  /// Location - The program location (within a function body) associated
  ///  with this node.  The location is a 'ProgramEdge' in the CFG.
  const ProgramEdge Location;
  
  /// State - The state associated with this node. Normally this value
  ///  is immutable, but we anticipate there will be times when algorithms
  ///  that directly manipulate the analysis graph will need to change it.
  void* State;

  /// Preds - The predecessors of this node.
  llvm::SmallVector<ExplodedNodeImpl*,2> Preds;
  
  /// Succs - The successors of this node.
  llvm::SmallVector<ExplodedNodeImpl*,2> Succs;
    
  // FIXME: Preds and Succs only grow, not shrink.  It would be nice
  //  if these were allocated from the same BumpPtrAllocator as
  //  the nodes themselves.

  /// Construct a ExplodedNodeImpl with the given node ID, program edge,
  ///  and state.
  explicit ExplodedNodeImpl(unsigned ID, const ProgramEdge& loc, void* state)
    : nodeID(ID), Location(loc), State(state) {}

  /// addUntypedPredeccessor - Adds a predecessor to the current node, and 
  ///  in tandem add this node as a successor of the other node.  This
  ///  "untyped" version is intended to be used only by ReachabilityEngineImpl;
  ///  normal clients should use 'addPredecessor' in ExplodedNode<>.
  void addUntypedPredecessor(ExplodedNodeImpl* V) {
    Preds.push_back(V);
    V->Succs.push_back(V);
  }

public:
  /// getLocation - Returns the edge associated with the given node.
  const ProgramEdge& getLocation() const { return Location; }
  
  /// getnodeID - Returns the unique ID of the node.  These IDs reflect
  ///  the order in which vertices were generated by ReachabilityEngineImpl.
  unsigned getnodeID() const { return nodeID; }
};

  
template <typename StateTy>
struct ReachabilityTrait {
  static inline void* toPtr(StateTy S) {
    return reinterpret_cast<void*>(S);
  }
  
  static inline StateTy toState(void* P) {
    return reinterpret_cast<StateTy>(P);
  }
};

  
template <typename StateTy>
class ExplodedNode : public ExplodedNodeImpl {
public:
  /// Construct a ExplodedNodeImpl with the given node ID, program edge,
  ///  and state.
  explicit ExplodedNode(unsigned ID, const ProgramEdge& loc, StateTy state)
  : ExplodedNodeImpl(ID,loc,ReachabilityTrait<StateTy>::toPtr(state)) {}
  
  /// getState - Returns the state associated with the node.  
  inline StateTy getState() const {
    return ReachabilityTrait<StateTy>::toState(State);
  }
  
  // Profiling (for FoldingSet).
  inline void Profile(llvm::FoldingSetNodeID& ID) const {
    StateTy::Profile(ID,getState());
  }

  // Iterators over successor and predecessor vertices.
  typedef ExplodedNode*        succ_iterator;
  typedef const ExplodedNode*  const_succ_iterator;
  typedef ExplodedNode*        pred_iterator;
  typedef const ExplodedNode*  const_pred_iterator;
  
  pred_iterator pred_begin() {
    return static_cast<pred_iterator>(Preds.begin());
  }
  
  pred_iterator pred_end() {
    return static_cast<pred_iterator>(Preds.end());
  }
  
  const_pred_iterator pred_begin() const {
    return static_cast<const_pred_iterator>(Preds.begin());
  }
  
  const_pred_iterator pred_end() const {
    return static_cast<const_pred_iterator>(Preds.end());
  }
  
  succ_iterator succ_begin() {
    return static_cast<succ_iterator>(Succs.begin());
  }
  
  succ_iterator succ_end() {
    return static_cast<succ_iterator>(Succs.end());
  }
  
  const_succ_iterator succ_begin() const {
    return static_cast<const_succ_iterator>(Succs.begin());
  }
  
  const_succ_iterator succ_end() const {
    return static_cast<const_succ_iterator>(Succs.end());
  }
  
  unsigned succ_size() const { return Succs.size(); }
  bool succ_empty() const { return Succs.empty(); }
  
  unsigned pred_size() const { return Preds.size(); }
  unsigned pred_empty() const { return Preds.empty(); }
};
  
} // end namespace clang

// GraphTraits for ExplodedNodes.

namespace llvm {
template<typename StateTy>
struct GraphTraits<clang::ExplodedNode<StateTy>*> {
  typedef clang::ExplodedNode<StateTy>      NodeType;
  typedef typename NodeType::succ_iterator  ChildIteratorType;
  typedef llvm::df_iterator<NodeType*>      nodes_iterator;
  
  static inline NodeType* getEntryNode(NodeType* N) {
    return N;
  }
  
  static inline ChildIteratorType child_begin(NodeType* N) {
    return N->succ_begin();
  }
  
  static inline ChildIteratorType child_end(NodeType* N) {
    return N->succ_end();
  }
  
  static inline nodes_iterator nodes_begin(NodeType* N) {
    return df_begin(N);
  }
  
  static inline nodes_iterator nodes_end(NodeType* N) {
    return df_end(N);
  }
};

template<typename StateTy>
struct GraphTraits<const clang::ExplodedNode<StateTy>*> {
  typedef const clang::ExplodedNode<StateTy> NodeType;
  typedef typename NodeType::succ_iterator   ChildIteratorType;
  typedef llvm::df_iterator<NodeType*>       nodes_iterator;
  
  static inline NodeType* getEntryNode(NodeType* N) {
    return N;
  }
  
  static inline ChildIteratorType child_begin(NodeType* N) {
    return N->succ_begin();
  }
  
  static inline ChildIteratorType child_end(NodeType* N) {
    return N->succ_end();
  }
  
  static inline nodes_iterator nodes_begin(NodeType* N) {
    return df_begin(N);
  }
  
  static inline nodes_iterator nodes_end(NodeType* N) {
    return df_end(N);
  }
};
}
#endif
