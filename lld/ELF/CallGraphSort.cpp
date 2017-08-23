//===- CallGraphSort.cpp --------------------------------------------------===//
//
//                             The LLVM Linker
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file This file implements Call-Chain Clustering from:
/// Optimizing Function Placement for Large-Scale Data-Center Applications
/// https://research.fb.com/wp-content/uploads/2017/01/cgo2017-hfsort-final1.pdf
///
/// The goal of this algorithm is to improve runtime performance of the final
/// executable by arranging code sections such that page table and i-cache
/// misses are minimized.
///
/// It does so given a call graph profile by the following:
/// * Build a call graph from the profile
/// * While there are unresolved edges
///   * Find the edge with the highest weight
///   * Check if merging the two clusters would create a cluster larger than the
///     target page size
///   * If not, contract that edge putting the callee after the caller
/// * Sort remaining clusters by density
///
//===----------------------------------------------------------------------===//

#include "CallGraphSort.h"
#include "SymbolTable.h"
#include "Target.h"

#include "llvm/Support/MathExtras.h"

#include <unordered_set>

using namespace llvm;
using namespace lld;
using namespace lld::elf;

namespace {
using NodeIndex = std::ptrdiff_t;

struct Node {
  Node() = default;
  Node(const InputSectionBase *IS) {
    Sections.push_back(IS);
    Size = IS->getSize();
  }
  std::vector<const InputSectionBase *> Sections;
  int64_t Size = 0;
  uint64_t Weight = 0;
};

struct Edge {
  NodeIndex From;
  NodeIndex To;
  mutable uint64_t Weight;
  bool operator==(const Edge Other) const {
    return From == Other.From && To == Other.To;
  }
};

struct EdgeHash {
  std::size_t operator()(const Edge E) const {
    return llvm::hash_combine(E.From, E.To);
  };
};
} // end anonymous namespace

static void insertOrIncrementEdge(std::unordered_set<Edge, EdgeHash> &Edges,
                                  const Edge E) {
  if (E.From == E.To)
    return;
  auto Res = Edges.insert(E);
  if (!Res.second)
    Res.first->Weight = SaturatingAdd(Res.first->Weight, E.Weight);
}

// Take the edge list in Config->CallGraphProfile, resolve symbol names to
// SymbolBodys, and generate a graph between InputSections with the provided
// weights.
static void buildCallGraph(std::vector<Node> &Nodes,
                           std::unordered_set<Edge, EdgeHash> &Edges) {
  DenseMap<const InputSectionBase *, NodeIndex> SecToNode;

  auto GetOrCreateNode = [&](const InputSectionBase *IS) -> NodeIndex {
    auto Res = SecToNode.insert(std::make_pair(IS, Nodes.size()));
    if (Res.second)
      Nodes.emplace_back(IS);
    return Res.first->second;
  };

  // Create the graph.
  for (const auto &C : Config->CallGraphProfile) {
    if (C.second == 0)
      continue;
    auto FromDR = dyn_cast_or_null<DefinedRegular>(Symtab->find(C.first.first));
    auto ToDR = dyn_cast_or_null<DefinedRegular>(Symtab->find(C.first.second));
    if (!FromDR || !ToDR)
      continue;
    auto FromSB = dyn_cast_or_null<const InputSectionBase>(FromDR->Section);
    auto ToSB = dyn_cast_or_null<const InputSectionBase>(ToDR->Section);
    if (!FromSB || !ToSB || FromSB->getSize() == 0 || ToSB->getSize() == 0)
      continue;
    NodeIndex From = GetOrCreateNode(FromSB);
    NodeIndex To = GetOrCreateNode(ToSB);
    insertOrIncrementEdge(Edges, {From, To, C.second});
    Nodes[To].Weight = SaturatingAdd(Nodes[To].Weight, C.second);
  }
}

// Group InputSections into clusters using the Call-Chain Clustering heuristic
// then sort the clusters by density.
static void generateClusters(std::vector<Node> &Nodes,
                             std::unordered_set<Edge, EdgeHash> &Edges) {
  // Collapse the graph.
  while (!Edges.empty()) {
    // Find the largest edge
    // FIXME: non deterministic order for equal edges.
    // FIXME: n^2 on Edges.
    auto Max = std::max_element(
        Edges.begin(), Edges.end(),
        [](const Edge A, const Edge B) { return A.Weight < B.Weight; });
    const Edge MaxE = *Max;
    Edges.erase(Max);
    // Merge the Nodes.
    Node &From = Nodes[MaxE.From];
    Node &To = Nodes[MaxE.To];
    if (From.Size + To.Size > Target->PageSize)
      continue;
    From.Sections.insert(From.Sections.end(), To.Sections.begin(),
                         To.Sections.end());
    To.Sections.clear();
    From.Size += To.Size;
    From.Weight = SaturatingAdd(From.Weight, To.Weight);
    // Collect all edges from or to the removed node and update them for the new
    // node.
    std::vector<Edge> OldEdges;
    // FIXME: n^2 on Edges.
    for (auto EI = Edges.begin(), EE = Edges.end(); EI != EE;) {
      if (EI->From == MaxE.To || EI->To == MaxE.To) {
        OldEdges.push_back(*EI);
        EI = Edges.erase(EI);
      } else
        ++EI;
    }
    for (const Edge E : OldEdges) {
      insertOrIncrementEdge(Edges,
                            {E.From == MaxE.To ? MaxE.From : E.From,
                             E.To == MaxE.To ? MaxE.From : E.To, E.Weight});
    }
  }

  // Sort by density.
  std::sort(Nodes.begin(), Nodes.end(), [](const Node &A, const Node &B) {
    return (APFloat(APFloat::IEEEdouble(), A.Weight) /
            APFloat(APFloat::IEEEdouble(), A.Size))
               .compare(APFloat(APFloat::IEEEdouble(), B.Weight) /
                        APFloat(APFloat::IEEEdouble(), B.Size)) ==
           APFloat::cmpLessThan;
  });
}

// Sort sections by the profile data provided by -callgraph-ordering-file
//
// This first builds a call graph based on the profile data then iteratively
// merges the hottest call edges as long as it would not create a cluster larger
// than the page size. All clusters are then sorted by a density metric to
// further improve locality.
llvm::DenseMap<const InputSectionBase *, int>
elf::computeCallGraphProfileOrder() {
  std::vector<Node> Nodes;
  std::unordered_set<Edge, EdgeHash> Edges;

  buildCallGraph(Nodes, Edges);
  generateClusters(Nodes, Edges);

  // Generate order.
  llvm::DenseMap<const InputSectionBase *, int> OrderMap;
  ssize_t CurOrder = 1;

  for (const Node &N : Nodes)
    for (const InputSectionBase *IS : N.Sections)
      OrderMap[IS] = CurOrder++;

  return OrderMap;
}
