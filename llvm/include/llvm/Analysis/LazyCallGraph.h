//===- LazyCallGraph.h - Analysis of a Module's call graph ------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
/// \file
///
/// Implements a lazy call graph analysis for the new pass manager.
///
/// This analysis is used to decompose an LLVM Module into components useful
/// for a specific subset of interprocedural optimization (IPO) techniques:
///
/// 1) Call site based pairwise interprocedural transforms such as inlining,
///    argument promotion, interprocedural constant propagation, etc.
/// 2) Reachable set interprocedural analyses and inference such as function
///    attribute inference, exception handling pruning, etc.
///
/// Both of these benefit from the optimization of each function in the program
/// in post-order to ensure that callees are fully optimized when analyzing
/// their callers and the call edge. To support this, we need an analysis that
/// provides a post-order traversal of the strongly connected components (SCCs)
/// of program call graph.
///
/// However, during this post-order traversal, transformations in #1 as well
/// the basic function-local optimizations can mutate and refine the graph.
/// These graph mutations need to be immediately reflected to achieve both of
/// the above goals. Both transformations (#1 and #2) and analyses (#2) benefit
/// directly from a more precise and refined call graph structure, so we want
/// to incorporate graph updates immediately *during* the traversal. Put
/// differently, we want a post-order traversal of the graph *including* all
/// updates and refinements made to it. There are two primary forms of call
/// graph refinement we care about:
///
/// a) Removing edges from the graph which provides smaller, more precise
///    reachable sets and SCCs.
/// b) Transforming indirect calls (that we must treat conservatively) into
///    direct calls through constant propagation (typically this is thought of in
///    the context of devirtualization).
///
/// These kinds of on-line graph updates make maintaining the graph
/// significantly more complex in several ways. First, they require that the
/// representation be updatable at all. They also require the graph update
/// operations preserve enough information that any necessary changes to
/// optimization techniques in #1 and #2 can be performed in a way that
/// reflects the nature of the update. For example, if an update splits one SCC
/// into three SCCs, we also need to know the post-order traversal of those new
/// SCCs.
///
/// The refinement in (b) introduces an unusual requirement on the graph. For
/// optimizations such as those in category #1 above, the graph must provide an
/// ordering that ensures (to the extent possible) that *potential* call edge
/// target functions are traversed prior to any (b)-style refinement which
/// could introduce an actual call edge to that function. This is essentially
/// a secondary traversal order constraint in addition to the post-order over
/// SCCs. This is accomplished by providing a layer of function reference edges
/// in addition to the layer of explicit function call edges. Because
/// function-local and pairwise interprocedural transformations primarily turn
/// an existing (possibly indirect) reference to a function into a direct call
/// to a function, a post-ordering of this "reference graph" satisfies the
/// additional constraint.
///
/// Further, even for transformations which refine indirect calls to direct
/// calls without an intrinsic "reference" in the IR, we can force them to be
/// based upon some encoding of such references. For example, a profile-guided
/// indirect call promotion pass might insist on embedding the candidate
/// targets from the profile into the IR as references to functions in order to
/// have them be included in the above ordering constraint.
///
/// The extra ordering constraint also serves a secondary but closely related
/// purpose. It allows a pass manager to identify regions of the call graph
/// which can be optimized independently from each other, even in the face of
/// transformations which mutate the call graph. As a consequence, it can be
/// used to drive safe parallelism across even the a subset of the
/// interprocedural optimizations on a module of the IR.
///
/// This analysis also tries to form these graph structures in a cache-friendly
/// way. As a consequence, it works hard to form the graph lazily as the
/// traversal visits the functions in the module. It also needs the update
/// algorithms described above to work in a context of a lazily formed graph
/// rather than a complete graph.
///
/// To understand the specific rules and nature of this call graph analysis,
/// see the documentation of the \c LazyCallGraph below.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_ANALYSIS_LAZYCALLGRAPH_H
#define LLVM_ANALYSIS_LAZYCALLGRAPH_H

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/PointerUnion.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/iterator.h"
#include "llvm/ADT/iterator_range.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Support/Allocator.h"
#include "llvm/Support/raw_ostream.h"
#include <iterator>
#include <utility>

namespace llvm {
class PreservedAnalyses;
class raw_ostream;

/// A lazily constructed view of the call graph of a module.
///
/// NB: This is *not* a traditional call graph! It is a graph which models both
/// the current calls and potential calls. As a consequence there are many
/// edges in this call graph that do not correspond to a 'call' or 'invoke'
/// instruction.
///
/// This analysis is designed to serve a complex set of constraints faced by
/// the optimizer when decomposing an LLVM Module for call-graph aware
/// optimizations and analyses. See the file comment for the detailed
/// motivations and context that leads to the particular design.
///
/// The analysis builds a pruned call graph with an edge between two function
/// if there exists at least one direct call from one to the other. On top of
/// that is layered a pruned reference graph with an edge between two functions
/// if there exists at least one (potentially transitive, through multiple
/// layers of global constants) reference from one function to the other.
/// A call edge is trivially also a reference edge, and so we nest the call
/// graph's SCCs (\c SCC below) within the referenc graph's SCCs (\c RefSCC
/// below).
///
/// Both of these graphs are directed graphs, and this analysis specifically
/// works to establish the SCCs of these directed graphs and support the
/// post-order traversal of the resulting SCC DAG. The SCC formation is done
/// lazily and on-demand during the traversal, including any necessary
/// inspection of the IR to build the graph itself, in order to maximize
/// locality when walking a module of the IR. The formation uses
/// straight-forward and lazy implementations of Tarjan's SCC detection
/// algorithm, and so is linear in the number of edges plus nodes.
///
/// The roots of the call graph are the external functions and functions
/// escaped into global variables. Those functions can be called from outside
/// of the module or via unknowable means in the IR -- we may not be able to
/// form even a potential call edge from a function body which may dynamically
/// load the function and call it.
///
/// The graph supports online updates, including within the region of the graph
/// traversed to form SCCs (both reference and call edge SCCs). These updates,
/// when within traversed regions of the graph, and thus potentially mutating
/// the SCC structure in addition to mutating the underlying graph, are also
/// written in a way that updates the SCC structure and surfaces detailed
/// information about the nature of those updates to the caller so it can
/// observe the new state and incorporate any changes. See the mutation API on
/// the \c RefSCC class below for details.
///
/// Updates are also implemented using straightforward versions of Tarjan's SCC
/// finding algorithm and are expected to be linear in the number of edges
/// leaving the set of potentially impacted nodes plus the number of impacted
/// nodes. While in the worst case this is the entire traversed region of the
/// graph, hitting that worst case requires mutating a leaf of the graph after
/// having traversed the root in order to form a large cycle. If doing repeated
/// mutations, the only reasonable access pattern is to mutate the just
/// traversed SCCs. In practice, this is exactly the set of mutations done by
/// the optimizer, leaving the complexity linear in the number of edges
/// departing nodes in the current SCC. The entire analysis should be
/// re-computed if full interprocedural optimizations run at any point and
/// could trigger large-scale incremental updates. Batching these mutations and
/// a single update is expected to be significantly more efficient.
///
/// FIXME: There is a well understood superlinear space problem with the
/// function reference graph as implemented: it de-normalizes indirect
/// references through tables of function pointers. There are many solutions
/// available to this, all of which have the effect of preserving some amount
/// of normalization and indirection to reduce the space requirements. We
/// expect to introduce one of these techniques as soon as this becomes
/// a problem in practice.
///
/// FIXME: This class is named LazyCallGraph in a lame attempt to distinguish
/// it from the existing CallGraph. At some point, it is expected that this
/// will be the only call graph and it will be renamed accordingly.
class LazyCallGraph {
public:
  class Node;
  class SCC;
  class RefSCC;
  class edge_iterator;
  class call_edge_iterator;

  /// A class used to represent edges in the call graph.
  ///
  /// The lazy call graph models both *call* edges and *reference* edges. Call
  /// edges are much what you would expect, and exist when there is a 'call' or
  /// 'invoke' instruction of some function. Reference edges are also tracked
  /// along side these, and exist whenever any instruction (transitively
  /// through its operands) references a function. All call edges are
  /// inherently reference edges, and so the reference graph forms a superset
  /// of the formal call graph.
  ///
  /// Furthermore, edges also may point to raw \c Function objects when those
  /// functions have not been scanned and incorporated into the graph (yet).
  /// This is one of the primary ways in which the graph can be lazy. When
  /// functions are scanned and fully incorporated into the graph, all of the
  /// edges referencing them are updated to point to the graph \c Node objects
  /// instead of to the raw \c Function objects. This class even provides
  /// methods to trigger this scan on-demand by attempting to get the target
  /// node of the graph and providing a reference back to the graph in order to
  /// lazily build it if necessary.
  ///
  /// All of these forms of edges are fundamentally represented as outgoing
  /// edges. The edges are stored in the source node and point at the target
  /// node. This allows the edge structure itself to be a very compact data
  /// structure: essentially a tagged pointer.
  class Edge {
  public:
    /// The kind of edge in the graph.
    enum Kind : bool { Ref = false, Call = true };

    Edge();
    explicit Edge(Function &F, Kind K);
    explicit Edge(Node &N, Kind K);

    /// Test whether the edge is null.
    ///
    /// This happens when an edge has been deleted. We leave the edge objects
    /// around but clear them.
    operator bool() const;

    /// Test whether the edge represents a direct call to a function.
    ///
    /// This requires that the edge is not null.
    bool isCall() const;

    /// Get the function referenced by this edge.
    ///
    /// This requires that the edge is not null, but will succeed whether we
    /// have built a graph node for the function yet or not.
    Function &getFunction() const;

    /// Get the call graph node referenced by this edge if one exists.
    ///
    /// This requires that the edge is not null. If we have built a graph node
    /// for the function this edge points to, this will return that node,
    /// otherwise it will return null.
    Node *getNode() const;

    /// Get the call graph node for this edge, building it if necessary.
    ///
    /// This requires that the edge is not null. If we have not yet built
    /// a graph node for the function this edge points to, this will first ask
    /// the graph to build that node, inserting it into all the relevant
    /// structures.
    Node &getNode(LazyCallGraph &G);

  private:
    friend class LazyCallGraph::Node;

    PointerIntPair<PointerUnion<Function *, Node *>, 1, Kind> Value;

    void setKind(Kind K) { Value.setInt(K); }
  };

  typedef SmallVector<Edge, 4> EdgeVectorT;
  typedef SmallVectorImpl<Edge> EdgeVectorImplT;

  /// A node in the call graph.
  ///
  /// This represents a single node. It's primary roles are to cache the list of
  /// callees, de-duplicate and provide fast testing of whether a function is
  /// a callee, and facilitate iteration of child nodes in the graph.
  class Node {
    friend class LazyCallGraph;
    friend class LazyCallGraph::SCC;

    LazyCallGraph *G;
    Function &F;

    // We provide for the DFS numbering and Tarjan walk lowlink numbers to be
    // stored directly within the node. These are both '-1' when nodes are part
    // of an SCC (or RefSCC), or '0' when not yet reached in a DFS walk.
    int DFSNumber;
    int LowLink;

    mutable EdgeVectorT Edges;
    DenseMap<Function *, int> EdgeIndexMap;

    /// Basic constructor implements the scanning of F into Edges and
    /// EdgeIndexMap.
    Node(LazyCallGraph &G, Function &F);

    /// Internal helper to insert an edge to a function.
    void insertEdgeInternal(Function &ChildF, Edge::Kind EK);

    /// Internal helper to insert an edge to a node.
    void insertEdgeInternal(Node &ChildN, Edge::Kind EK);

    /// Internal helper to change an edge kind.
    void setEdgeKind(Function &ChildF, Edge::Kind EK);

    /// Internal helper to remove the edge to the given function.
    void removeEdgeInternal(Function &ChildF);

    /// Print the name of this node's function.
    friend raw_ostream &operator<<(raw_ostream &OS, const Node &N) {
      return OS << N.F.getName();
    }

    /// Dump the name of this node's function to stderr.
    void dump() const;

  public:
    LazyCallGraph &getGraph() const { return *G; }

    Function &getFunction() const { return F; }

    edge_iterator begin() const {
      return edge_iterator(Edges.begin(), Edges.end());
    }
    edge_iterator end() const { return edge_iterator(Edges.end(), Edges.end()); }

    const Edge &operator[](int i) const { return Edges[i]; }
    const Edge &operator[](Function &F) const {
      assert(EdgeIndexMap.find(&F) != EdgeIndexMap.end() && "No such edge!");
      return Edges[EdgeIndexMap.find(&F)->second];
    }
    const Edge &operator[](Node &N) const { return (*this)[N.getFunction()]; }

    call_edge_iterator call_begin() const {
      return call_edge_iterator(Edges.begin(), Edges.end());
    }
    call_edge_iterator call_end() const {
      return call_edge_iterator(Edges.end(), Edges.end());
    }

    iterator_range<call_edge_iterator> calls() const {
      return make_range(call_begin(), call_end());
    }

    /// Equality is defined as address equality.
    bool operator==(const Node &N) const { return this == &N; }
    bool operator!=(const Node &N) const { return !operator==(N); }
  };

  /// A lazy iterator used for both the entry nodes and child nodes.
  ///
  /// When this iterator is dereferenced, if not yet available, a function will
  /// be scanned for "calls" or uses of functions and its child information
  /// will be constructed. All of these results are accumulated and cached in
  /// the graph.
  class edge_iterator
      : public iterator_adaptor_base<edge_iterator, EdgeVectorImplT::iterator,
                                     std::forward_iterator_tag> {
    friend class LazyCallGraph;
    friend class LazyCallGraph::Node;

    EdgeVectorImplT::iterator E;

    // Build the iterator for a specific position in the edge list.
    edge_iterator(EdgeVectorImplT::iterator BaseI,
                  EdgeVectorImplT::iterator E)
        : iterator_adaptor_base(BaseI), E(E) {
      while (I != E && !*I)
        ++I;
    }

  public:
    edge_iterator() {}

    using iterator_adaptor_base::operator++;
    edge_iterator &operator++() {
      do {
        ++I;
      } while (I != E && !*I);
      return *this;
    }
  };

  /// A lazy iterator over specifically call edges.
  ///
  /// This has the same iteration properties as the \c edge_iterator, but
  /// restricts itself to edges which represent actual calls.
  class call_edge_iterator
      : public iterator_adaptor_base<call_edge_iterator,
                                     EdgeVectorImplT::iterator,
                                     std::forward_iterator_tag> {
    friend class LazyCallGraph;
    friend class LazyCallGraph::Node;

    EdgeVectorImplT::iterator E;

    /// Advance the iterator to the next valid, call edge.
    void advanceToNextEdge() {
      while (I != E && (!*I || !I->isCall()))
        ++I;
    }

    // Build the iterator for a specific position in the edge list.
    call_edge_iterator(EdgeVectorImplT::iterator BaseI,
                       EdgeVectorImplT::iterator E)
        : iterator_adaptor_base(BaseI), E(E) {
      advanceToNextEdge();
    }

  public:
    call_edge_iterator() {}

    using iterator_adaptor_base::operator++;
    call_edge_iterator &operator++() {
      ++I;
      advanceToNextEdge();
      return *this;
    }
  };

  /// An SCC of the call graph.
  ///
  /// This represents a Strongly Connected Component of the direct call graph
  /// -- ignoring indirect calls and function references. It stores this as
  /// a collection of call graph nodes. While the order of nodes in the SCC is
  /// stable, it is not any particular order.
  ///
  /// The SCCs are nested within a \c RefSCC, see below for details about that
  /// outer structure. SCCs do not support mutation of the call graph, that
  /// must be done through the containing \c RefSCC in order to fully reason
  /// about the ordering and connections of the graph.
  class SCC {
    friend class LazyCallGraph;
    friend class LazyCallGraph::Node;

    RefSCC *OuterRefSCC;
    SmallVector<Node *, 1> Nodes;

    template <typename NodeRangeT>
    SCC(RefSCC &OuterRefSCC, NodeRangeT &&Nodes)
        : OuterRefSCC(&OuterRefSCC), Nodes(std::forward<NodeRangeT>(Nodes)) {}

    void clear() {
      OuterRefSCC = nullptr;
      Nodes.clear();
    }

    /// Print a short descrtiption useful for debugging or logging.
    ///
    /// We print the function names in the SCC wrapped in '()'s and skipping
    /// the middle functions if there are a large number.
    //
    // Note: this is defined inline to dodge issues with GCC's interpretation
    // of enclosing namespaces for friend function declarations.
    friend raw_ostream &operator<<(raw_ostream &OS, const SCC &C) {
      OS << '(';
      int i = 0;
      for (LazyCallGraph::Node &N : C) {
        if (i > 0)
          OS << ", ";
        // Elide the inner elements if there are too many.
        if (i > 8) {
          OS << "..., " << *C.Nodes.back();
          break;
        }
        OS << N;
        ++i;
      }
      OS << ')';
      return OS;
    }

    /// Dump a short description of this SCC to stderr.
    void dump() const;

#ifndef NDEBUG
    /// Verify invariants about the SCC.
    ///
    /// This will attempt to validate all of the basic invariants within an
    /// SCC, but not that it is a strongly connected componet per-se. Primarily
    /// useful while building and updating the graph to check that basic
    /// properties are in place rather than having inexplicable crashes later.
    void verify();
#endif

  public:
    typedef pointee_iterator<SmallVectorImpl<Node *>::const_iterator> iterator;

    iterator begin() const { return Nodes.begin(); }
    iterator end() const { return Nodes.end(); }

    int size() const { return Nodes.size(); }

    RefSCC &getOuterRefSCC() const { return *OuterRefSCC; }

    /// Provide a short name by printing this SCC to a std::string.
    ///
    /// This copes with the fact that we don't have a name per-se for an SCC
    /// while still making the use of this in debugging and logging useful.
    std::string getName() const {
      std::string Name;
      raw_string_ostream OS(Name);
      OS << *this;
      OS.flush();
      return Name;
    }
  };

  /// A RefSCC of the call graph.
  ///
  /// This models a Strongly Connected Component of function reference edges in
  /// the call graph. As opposed to actual SCCs, these can be used to scope
  /// subgraphs of the module which are independent from other subgraphs of the
  /// module because they do not reference it in any way. This is also the unit
  /// where we do mutation of the graph in order to restrict mutations to those
  /// which don't violate this independence.
  ///
  /// A RefSCC contains a DAG of actual SCCs. All the nodes within the RefSCC
  /// are necessarily within some actual SCC that nests within it. Since
  /// a direct call *is* a reference, there will always be at least one RefSCC
  /// around any SCC.
  class RefSCC {
    friend class LazyCallGraph;
    friend class LazyCallGraph::Node;

    LazyCallGraph *G;
    SmallPtrSet<RefSCC *, 1> Parents;

    /// A postorder list of the inner SCCs.
    SmallVector<SCC *, 4> SCCs;

    /// A map from SCC to index in the postorder list.
    SmallDenseMap<SCC *, int, 4> SCCIndices;

    /// Fast-path constructor. RefSCCs should instead be constructed by calling
    /// formRefSCCFast on the graph itself.
    RefSCC(LazyCallGraph &G);

    /// Print a short description useful for debugging or logging.
    ///
    /// We print the SCCs wrapped in '[]'s and skipping the middle SCCs if
    /// there are a large number.
    //
    // Note: this is defined inline to dodge issues with GCC's interpretation
    // of enclosing namespaces for friend function declarations.
    friend raw_ostream &operator<<(raw_ostream &OS, const RefSCC &RC) {
      OS << '[';
      int i = 0;
      for (LazyCallGraph::SCC &C : RC) {
        if (i > 0)
          OS << ", ";
        // Elide the inner elements if there are too many.
        if (i > 4) {
          OS << "..., " << *RC.SCCs.back();
          break;
        }
        OS << C;
        ++i;
      }
      OS << ']';
      return OS;
    }

    /// Dump a short description of this RefSCC to stderr.
    void dump() const;

#ifndef NDEBUG
    /// Verify invariants about the RefSCC and all its SCCs.
    ///
    /// This will attempt to validate all of the invariants *within* the
    /// RefSCC, but not that it is a strongly connected component of the larger
    /// graph. This makes it useful even when partially through an update.
    ///
    /// Invariants checked:
    /// - SCCs and their indices match.
    /// - The SCCs list is in fact in post-order.
    void verify();
#endif

  public:
    typedef pointee_iterator<SmallVectorImpl<SCC *>::const_iterator> iterator;
    typedef iterator_range<iterator> range;
    typedef pointee_iterator<SmallPtrSetImpl<RefSCC *>::const_iterator>
        parent_iterator;

    iterator begin() const { return SCCs.begin(); }
    iterator end() const { return SCCs.end(); }

    ssize_t size() const { return SCCs.size(); }

    SCC &operator[](int Idx) { return *SCCs[Idx]; }

    iterator find(SCC &C) const {
      return SCCs.begin() + SCCIndices.find(&C)->second;
    }

    parent_iterator parent_begin() const { return Parents.begin(); }
    parent_iterator parent_end() const { return Parents.end(); }

    iterator_range<parent_iterator> parents() const {
      return make_range(parent_begin(), parent_end());
    }

    /// Test if this SCC is a parent of \a C.
    bool isParentOf(const RefSCC &C) const { return C.isChildOf(*this); }

    /// Test if this RefSCC is an ancestor of \a C.
    bool isAncestorOf(const RefSCC &C) const { return C.isDescendantOf(*this); }

    /// Test if this RefSCC is a child of \a C.
    bool isChildOf(const RefSCC &C) const {
      return Parents.count(const_cast<RefSCC *>(&C));
    }

    /// Test if this RefSCC is a descendant of \a C.
    bool isDescendantOf(const RefSCC &C) const;

    /// Provide a short name by printing this SCC to a std::string.
    ///
    /// This copes with the fact that we don't have a name per-se for an SCC
    /// while still making the use of this in debugging and logging useful.
    std::string getName() const {
      std::string Name;
      raw_string_ostream OS(Name);
      OS << *this;
      OS.flush();
      return Name;
    }

    ///@{
    /// \name Mutation API
    ///
    /// These methods provide the core API for updating the call graph in the
    /// presence of a (potentially still in-flight) DFS-found SCCs.
    ///
    /// Note that these methods sometimes have complex runtimes, so be careful
    /// how you call them.

    /// Make an existing internal ref edge into a call edge.
    ///
    /// This may form a larger cycle and thus collapse SCCs into TargetN's SCC.
    /// If that happens, the deleted SCC pointers are returned. These SCCs are
    /// not in a valid state any longer but the pointers will remain valid
    /// until destruction of the parent graph instance for the purpose of
    /// clearing cached information.
    ///
    /// After this operation, both SourceN's SCC and TargetN's SCC may move
    /// position within this RefSCC's postorder list. Any SCCs merged are
    /// merged into the TargetN's SCC in order to preserve reachability analyses
    /// which took place on that SCC.
    SmallVector<SCC *, 1> switchInternalEdgeToCall(Node &SourceN,
                                                   Node &TargetN);

    /// Make an existing internal call edge into a ref edge.
    ///
    /// If SourceN and TargetN are part of a single SCC, it may be split up due
    /// to breaking a cycle in the call edges that formed it. If that happens,
    /// then this routine will insert new SCCs into the postorder list *before*
    /// the SCC of TargetN (previously the SCC of both). This preserves
    /// postorder as the TargetN can reach all of the other nodes by definition
    /// of previously being in a single SCC formed by the cycle from SourceN to
    /// TargetN. The newly added nodes are added *immediately* and contiguously
    /// prior to the TargetN SCC and so they may be iterated starting from
    /// there.
    void switchInternalEdgeToRef(Node &SourceN, Node &TargetN);

    /// Make an existing outgoing ref edge into a call edge.
    ///
    /// Note that this is trivial as there are no cyclic impacts and there
    /// remains a reference edge.
    void switchOutgoingEdgeToCall(Node &SourceN, Node &TargetN);

    /// Make an existing outgoing call edge into a ref edge.
    ///
    /// This is trivial as there are no cyclic impacts and there remains
    /// a reference edge.
    void switchOutgoingEdgeToRef(Node &SourceN, Node &TargetN);

    /// Insert a ref edge from one node in this RefSCC to another in this
    /// RefSCC.
    ///
    /// This is always a trivial operation as it doesn't change any part of the
    /// graph structure besides connecting the two nodes.
    ///
    /// Note that we don't support directly inserting internal *call* edges
    /// because that could change the graph structure and requires returning
    /// information about what became invalid. As a consequence, the pattern
    /// should be to first insert the necessary ref edge, and then to switch it
    /// to a call edge if needed and handle any invalidation that results. See
    /// the \c switchInternalEdgeToCall routine for details.
    void insertInternalRefEdge(Node &SourceN, Node &TargetN);

    /// Insert an edge whose parent is in this RefSCC and child is in some
    /// child RefSCC.
    ///
    /// There must be an existing path from the \p SourceN to the \p TargetN.
    /// This operation is inexpensive and does not change the set of SCCs and
    /// RefSCCs in the graph.
    void insertOutgoingEdge(Node &SourceN, Node &TargetN, Edge::Kind EK);

    /// Insert an edge whose source is in a descendant RefSCC and target is in
    /// this RefSCC.
    ///
    /// There must be an existing path from the target to the source in this
    /// case.
    ///
    /// NB! This is has the potential to be a very expensive function. It
    /// inherently forms a cycle in the prior RefSCC DAG and we have to merge
    /// RefSCCs to resolve that cycle. But finding all of the RefSCCs which
    /// participate in the cycle can in the worst case require traversing every
    /// RefSCC in the graph. Every attempt is made to avoid that, but passes
    /// must still exercise caution calling this routine repeatedly.
    ///
    /// Also note that this can only insert ref edges. In order to insert
    /// a call edge, first insert a ref edge and then switch it to a call edge.
    /// These are intentionally kept as separate interfaces because each step
    /// of the operation invalidates a different set of data structures.
    ///
    /// This returns all the RefSCCs which were merged into the this RefSCC
    /// (the target's). This allows callers to invalidate any cached
    /// information.
    ///
    /// FIXME: We could possibly optimize this quite a bit for cases where the
    /// caller and callee are very nearby in the graph. See comments in the
    /// implementation for details, but that use case might impact users.
    SmallVector<RefSCC *, 1> insertIncomingRefEdge(Node &SourceN,
                                                   Node &TargetN);

    /// Remove an edge whose source is in this RefSCC and target is *not*.
    ///
    /// This removes an inter-RefSCC edge. All inter-RefSCC edges originating
    /// from this SCC have been fully explored by any in-flight DFS graph
    /// formation, so this is always safe to call once you have the source
    /// RefSCC.
    ///
    /// This operation does not change the cyclic structure of the graph and so
    /// is very inexpensive. It may change the connectivity graph of the SCCs
    /// though, so be careful calling this while iterating over them.
    void removeOutgoingEdge(Node &SourceN, Node &TargetN);

    /// Remove a ref edge which is entirely within this RefSCC.
    ///
    /// Both the \a SourceN and the \a TargetN must be within this RefSCC.
    /// Removing such an edge may break cycles that form this RefSCC and thus
    /// this operation may change the RefSCC graph significantly. In
    /// particular, this operation will re-form new RefSCCs based on the
    /// remaining connectivity of the graph. The following invariants are
    /// guaranteed to hold after calling this method:
    ///
    /// 1) This RefSCC is still a RefSCC in the graph.
    /// 2) This RefSCC will be the parent of any new RefSCCs. Thus, this RefSCC
    ///    is preserved as the root of any new RefSCC DAG formed.
    /// 3) No RefSCC other than this RefSCC has its member set changed (this is
    ///    inherent in the definition of removing such an edge).
    /// 4) All of the parent links of the RefSCC graph will be updated to
    ///    reflect the new RefSCC structure.
    /// 5) All RefSCCs formed out of this RefSCC, excluding this RefSCC, will
    ///    be returned in post-order.
    /// 6) The order of the RefSCCs in the vector will be a valid postorder
    ///    traversal of the new RefSCCs.
    ///
    /// These invariants are very important to ensure that we can build
    /// optimization pipelines on top of the CGSCC pass manager which
    /// intelligently update the RefSCC graph without invalidating other parts
    /// of the RefSCC graph.
    ///
    /// Note that we provide no routine to remove a *call* edge. Instead, you
    /// must first switch it to a ref edge using \c switchInternalEdgeToRef.
    /// This split API is intentional as each of these two steps can invalidate
    /// a different aspect of the graph structure and needs to have the
    /// invalidation handled independently.
    ///
    /// The runtime complexity of this method is, in the worst case, O(V+E)
    /// where V is the number of nodes in this RefSCC and E is the number of
    /// edges leaving the nodes in this RefSCC. Note that E includes both edges
    /// within this RefSCC and edges from this RefSCC to child RefSCCs. Some
    /// effort has been made to minimize the overhead of common cases such as
    /// self-edges and edge removals which result in a spanning tree with no
    /// more cycles. There are also detailed comments within the implementation
    /// on techniques which could substantially improve this routine's
    /// efficiency.
    SmallVector<RefSCC *, 1> removeInternalRefEdge(Node &SourceN,
                                                   Node &TargetN);

    ///@}
  };

  /// A post-order depth-first SCC iterator over the call graph.
  ///
  /// This iterator triggers the Tarjan DFS-based formation of the SCC DAG for
  /// the call graph, walking it lazily in depth-first post-order. That is, it
  /// always visits SCCs for a callee prior to visiting the SCC for a caller
  /// (when they are in different SCCs).
  class postorder_ref_scc_iterator
      : public iterator_facade_base<postorder_ref_scc_iterator,
                                    std::forward_iterator_tag, RefSCC> {
    friend class LazyCallGraph;
    friend class LazyCallGraph::Node;

    /// Nonce type to select the constructor for the end iterator.
    struct IsAtEndT {};

    LazyCallGraph *G;
    RefSCC *C;

    // Build the begin iterator for a node.
    postorder_ref_scc_iterator(LazyCallGraph &G) : G(&G) {
      C = G.getNextRefSCCInPostOrder();
    }

    // Build the end iterator for a node. This is selected purely by overload.
    postorder_ref_scc_iterator(LazyCallGraph &G, IsAtEndT /*Nonce*/)
        : G(&G), C(nullptr) {}

  public:
    bool operator==(const postorder_ref_scc_iterator &Arg) const {
      return G == Arg.G && C == Arg.C;
    }

    reference operator*() const { return *C; }

    using iterator_facade_base::operator++;
    postorder_ref_scc_iterator &operator++() {
      C = G->getNextRefSCCInPostOrder();
      return *this;
    }
  };

  /// Construct a graph for the given module.
  ///
  /// This sets up the graph and computes all of the entry points of the graph.
  /// No function definitions are scanned until their nodes in the graph are
  /// requested during traversal.
  LazyCallGraph(Module &M);

  LazyCallGraph(LazyCallGraph &&G);
  LazyCallGraph &operator=(LazyCallGraph &&RHS);

  edge_iterator begin() {
    return edge_iterator(EntryEdges.begin(), EntryEdges.end());
  }
  edge_iterator end() {
    return edge_iterator(EntryEdges.end(), EntryEdges.end());
  }

  postorder_ref_scc_iterator postorder_ref_scc_begin() {
    return postorder_ref_scc_iterator(*this);
  }
  postorder_ref_scc_iterator postorder_ref_scc_end() {
    return postorder_ref_scc_iterator(*this,
                                      postorder_ref_scc_iterator::IsAtEndT());
  }

  iterator_range<postorder_ref_scc_iterator> postorder_ref_sccs() {
    return make_range(postorder_ref_scc_begin(), postorder_ref_scc_end());
  }

  /// Lookup a function in the graph which has already been scanned and added.
  Node *lookup(const Function &F) const { return NodeMap.lookup(&F); }

  /// Lookup a function's SCC in the graph.
  ///
  /// \returns null if the function hasn't been assigned an SCC via the SCC
  /// iterator walk.
  SCC *lookupSCC(Node &N) const { return SCCMap.lookup(&N); }

  /// Lookup a function's RefSCC in the graph.
  ///
  /// \returns null if the function hasn't been assigned a RefSCC via the
  /// RefSCC iterator walk.
  RefSCC *lookupRefSCC(Node &N) const {
    if (SCC *C = lookupSCC(N))
      return &C->getOuterRefSCC();

    return nullptr;
  }

  /// Get a graph node for a given function, scanning it to populate the graph
  /// data as necessary.
  Node &get(Function &F) {
    Node *&N = NodeMap[&F];
    if (N)
      return *N;

    return insertInto(F, N);
  }

  ///@{
  /// \name Pre-SCC Mutation API
  ///
  /// These methods are only valid to call prior to forming any SCCs for this
  /// call graph. They can be used to update the core node-graph during
  /// a node-based inorder traversal that precedes any SCC-based traversal.
  ///
  /// Once you begin manipulating a call graph's SCCs, you must perform all
  /// mutation of the graph via the SCC methods.

  /// Update the call graph after inserting a new edge.
  void insertEdge(Node &Caller, Function &Callee, Edge::Kind EK);

  /// Update the call graph after inserting a new edge.
  void insertEdge(Function &Caller, Function &Callee, Edge::Kind EK) {
    return insertEdge(get(Caller), Callee, EK);
  }

  /// Update the call graph after deleting an edge.
  void removeEdge(Node &Caller, Function &Callee);

  /// Update the call graph after deleting an edge.
  void removeEdge(Function &Caller, Function &Callee) {
    return removeEdge(get(Caller), Callee);
  }

  ///@}

private:
  typedef SmallVectorImpl<Node *>::reverse_iterator node_stack_iterator;
  typedef iterator_range<node_stack_iterator> node_stack_range;

  /// Allocator that holds all the call graph nodes.
  SpecificBumpPtrAllocator<Node> BPA;

  /// Maps function->node for fast lookup.
  DenseMap<const Function *, Node *> NodeMap;

  /// The entry nodes to the graph.
  ///
  /// These nodes are reachable through "external" means. Put another way, they
  /// escape at the module scope.
  EdgeVectorT EntryEdges;

  /// Map of the entry nodes in the graph to their indices in \c EntryEdges.
  DenseMap<Function *, int> EntryIndexMap;

  /// Allocator that holds all the call graph SCCs.
  SpecificBumpPtrAllocator<SCC> SCCBPA;

  /// Maps Function -> SCC for fast lookup.
  DenseMap<Node *, SCC *> SCCMap;

  /// Allocator that holds all the call graph RefSCCs.
  SpecificBumpPtrAllocator<RefSCC> RefSCCBPA;

  /// The leaf RefSCCs of the graph.
  ///
  /// These are all of the RefSCCs which have no children.
  SmallVector<RefSCC *, 4> LeafRefSCCs;

  /// Stack of nodes in the DFS walk.
  SmallVector<std::pair<Node *, edge_iterator>, 4> DFSStack;

  /// Set of entry nodes not-yet-processed into RefSCCs.
  SmallVector<Function *, 4> RefSCCEntryNodes;

  /// Stack of nodes the DFS has walked but not yet put into a SCC.
  SmallVector<Node *, 4> PendingRefSCCStack;

  /// Counter for the next DFS number to assign.
  int NextDFSNumber;

  /// Helper to insert a new function, with an already looked-up entry in
  /// the NodeMap.
  Node &insertInto(Function &F, Node *&MappedN);

  /// Helper to update pointers back to the graph object during moves.
  void updateGraphPtrs();

  /// Allocates an SCC and constructs it using the graph allocator.
  ///
  /// The arguments are forwarded to the constructor.
  template <typename... Ts> SCC *createSCC(Ts &&... Args) {
    return new (SCCBPA.Allocate()) SCC(std::forward<Ts>(Args)...);
  }

  /// Allocates a RefSCC and constructs it using the graph allocator.
  ///
  /// The arguments are forwarded to the constructor.
  template <typename... Ts> RefSCC *createRefSCC(Ts &&... Args) {
    return new (RefSCCBPA.Allocate()) RefSCC(std::forward<Ts>(Args)...);
  }

  /// Build the SCCs for a RefSCC out of a list of nodes.
  void buildSCCs(RefSCC &RC, node_stack_range Nodes);

  /// Connect a RefSCC into the larger graph.
  ///
  /// This walks the edges to connect the RefSCC to its children's parent set,
  /// and updates the root leaf list.
  void connectRefSCC(RefSCC &RC);

  /// Retrieve the next node in the post-order RefSCC walk of the call graph.
  RefSCC *getNextRefSCCInPostOrder();
};

inline LazyCallGraph::Edge::Edge() : Value() {}
inline LazyCallGraph::Edge::Edge(Function &F, Kind K) : Value(&F, K) {}
inline LazyCallGraph::Edge::Edge(Node &N, Kind K) : Value(&N, K) {}

inline LazyCallGraph::Edge::operator bool() const {
  return !Value.getPointer().isNull();
}

inline bool LazyCallGraph::Edge::isCall() const {
  assert(*this && "Queried a null edge!");
  return Value.getInt() == Call;
}

inline Function &LazyCallGraph::Edge::getFunction() const {
  assert(*this && "Queried a null edge!");
  auto P = Value.getPointer();
  if (auto *F = P.dyn_cast<Function *>())
    return *F;

  return P.get<Node *>()->getFunction();
}

inline LazyCallGraph::Node *LazyCallGraph::Edge::getNode() const {
  assert(*this && "Queried a null edge!");
  auto P = Value.getPointer();
  if (auto *N = P.dyn_cast<Node *>())
    return N;

  return nullptr;
}

inline LazyCallGraph::Node &LazyCallGraph::Edge::getNode(LazyCallGraph &G) {
  assert(*this && "Queried a null edge!");
  auto P = Value.getPointer();
  if (auto *N = P.dyn_cast<Node *>())
    return *N;

  Node &N = G.get(*P.get<Function *>());
  Value.setPointer(&N);
  return N;
}

// Provide GraphTraits specializations for call graphs.
template <> struct GraphTraits<LazyCallGraph::Node *> {
  typedef LazyCallGraph::Node NodeType;
  typedef LazyCallGraph::edge_iterator ChildIteratorType;

  static NodeType *getEntryNode(NodeType *N) { return N; }
  static ChildIteratorType child_begin(NodeType *N) { return N->begin(); }
  static ChildIteratorType child_end(NodeType *N) { return N->end(); }
};
template <> struct GraphTraits<LazyCallGraph *> {
  typedef LazyCallGraph::Node NodeType;
  typedef LazyCallGraph::edge_iterator ChildIteratorType;

  static NodeType *getEntryNode(NodeType *N) { return N; }
  static ChildIteratorType child_begin(NodeType *N) { return N->begin(); }
  static ChildIteratorType child_end(NodeType *N) { return N->end(); }
};

/// An analysis pass which computes the call graph for a module.
class LazyCallGraphAnalysis : public AnalysisInfoMixin<LazyCallGraphAnalysis> {
  friend AnalysisInfoMixin<LazyCallGraphAnalysis>;
  static char PassID;

public:
  /// Inform generic clients of the result type.
  typedef LazyCallGraph Result;

  /// Compute the \c LazyCallGraph for the module \c M.
  ///
  /// This just builds the set of entry points to the call graph. The rest is
  /// built lazily as it is walked.
  LazyCallGraph run(Module &M, ModuleAnalysisManager &) {
    return LazyCallGraph(M);
  }
};

/// A pass which prints the call graph to a \c raw_ostream.
///
/// This is primarily useful for testing the analysis.
class LazyCallGraphPrinterPass
    : public PassInfoMixin<LazyCallGraphPrinterPass> {
  raw_ostream &OS;

public:
  explicit LazyCallGraphPrinterPass(raw_ostream &OS);

  PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM);
};

/// A pass which prints the call graph as a DOT file to a \c raw_ostream.
///
/// This is primarily useful for visualization purposes.
class LazyCallGraphDOTPrinterPass
    : public PassInfoMixin<LazyCallGraphDOTPrinterPass> {
  raw_ostream &OS;

public:
  explicit LazyCallGraphDOTPrinterPass(raw_ostream &OS);

  PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM);
};
}

#endif
