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

#include <queue>
#include <unordered_set>

using namespace llvm;
using namespace lld;
using namespace lld::elf;

namespace {
class CallGraphSort {
  using NodeIndex = std::ptrdiff_t;
  using EdgeIndex = std::ptrdiff_t;

  struct Node {
    Node() = default;
    Node(const InputSectionBase *IS);
    std::vector<const InputSectionBase *> Sections;
    std::vector<EdgeIndex> IncidentEdges;
    int64_t Size = 0;
    uint64_t Weight = 0;
  };

  struct Edge {
    NodeIndex From;
    NodeIndex To;
    mutable uint64_t Weight;
    bool operator==(const Edge Other) const;
    bool operator<(const Edge Other) const;
    void kill();
    bool isDead() const;
  };

  std::vector<Node> Nodes;
  std::vector<Edge> Edges;
  struct EdgePriorityCmp {
    std::vector<Edge> &Edges;
    bool operator()(EdgeIndex A, EdgeIndex B) const {
      return Edges[A].Weight < Edges[B].Weight;
    }
  };
  std::priority_queue<EdgeIndex, std::vector<EdgeIndex>, EdgePriorityCmp>
      WorkQueue{EdgePriorityCmp{Edges}};

  void contractEdge(EdgeIndex CEI);
  void generateClusters();

public:
  CallGraphSort(DenseMap<std::pair<StringRef, StringRef>, uint64_t> &Profile);

  DenseMap<const InputSectionBase *, int> run();
};
} // end anonymous namespace

CallGraphSort::Node::Node(const InputSectionBase *IS) {
  Sections.push_back(IS);
  Size = IS->getSize();
}

bool CallGraphSort::Edge::operator==(const Edge Other) const {
  return From == Other.From && To == Other.To;
}

bool CallGraphSort::Edge::operator<(const Edge Other) const {
  if (From != Other.From)
    return From < Other.From;
  if (To != Other.To)
    return To < Other.To;
  return false;
}

void CallGraphSort::Edge::kill() {
  From = 0;
  To = 0;
}

bool CallGraphSort::Edge::isDead() const { return From == 0 && To == 0; }

// Take the edge list in Config->CallGraphProfile, resolve symbol names to
// SymbolBodys, and generate a graph between InputSections with the provided
// weights.
CallGraphSort::CallGraphSort(
    DenseMap<std::pair<StringRef, StringRef>, uint64_t> &Profile) {
  DenseMap<const InputSectionBase *, NodeIndex> SecToNode;
  std::map<Edge, EdgeIndex> EdgeMap;

  auto GetOrCreateNode = [&](const InputSectionBase *IS) -> NodeIndex {
    auto Res = SecToNode.insert(std::make_pair(IS, Nodes.size()));
    if (Res.second)
      Nodes.emplace_back(IS);
    return Res.first->second;
  };

  // Create the graph.
  for (const auto &C : Profile) {
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
    Edge E{From, To, C.second};
    auto Res = EdgeMap.insert(std::make_pair(E, Edges.size()));
    EdgeIndex EI = Res.first->second;
    if (Res.second) {
      Edges.push_back(E);
      Nodes[From].IncidentEdges.push_back(EI);
      Nodes[To].IncidentEdges.push_back(EI);
    } else
      Edges[EI].Weight = SaturatingAdd(Edges[EI].Weight, C.second);
    Nodes[To].Weight = SaturatingAdd(Nodes[To].Weight, C.second);
  }
}

void CallGraphSort::contractEdge(EdgeIndex CEI) {
  // Make a copy of the edge as the original will be marked killed while being
  // used.
  Edge CE = Edges[CEI];
  std::vector<EdgeIndex> &FE = Nodes[CE.From].IncidentEdges;
  // Remove the self edge from From.
  FE.erase(std::remove(FE.begin(), FE.end(), CEI));
  std::vector<EdgeIndex> &TE = Nodes[CE.To].IncidentEdges;
  // Update all edges incident with To to reference From instead. Then if they
  // aren't self edges add them to From.
  for (EdgeIndex EI : TE) {
    Edge &E = Edges[EI];
    // E.From = E.From == CE.To ? CE.From : E.From;
    // E.To = E.To == CE.To ? CE.From : E.To;
    if (E.From == CE.To)
      E.From = CE.From;
    if (E.To == CE.To)
      E.To = CE.From;
    if (E.To == E.From) {
      E.kill();
      continue;
    }
    FE.push_back(EI);
  }

  // Free memory.
  std::vector<EdgeIndex>().swap(TE);

  if (FE.empty())
    return;

  // Sort edges so they can be merged. The stability of this sort doesn't matter
  // as equal edges will be merged in an order independent manner.
  std::sort(FE.begin(), FE.end(),
            [&](EdgeIndex AI, EdgeIndex BI) { return Edges[AI] < Edges[BI]; });

  // std::unique, but also merge equal values.
  auto First = FE.begin();
  auto Last = FE.end();
  auto Result = First;
  while (++First != Last) {
    if (Edges[*Result] == Edges[*First]) {
      Edges[*Result].Weight =
          SaturatingAdd(Edges[*Result].Weight, Edges[*First].Weight);
      Edges[*First].kill();
      // Add the updated edge to the work queue without removing the previous
      // entry. Edges will never be contracted twice as they are marked as dead.
      WorkQueue.push(*Result);
    } else if (++Result != First)
      *Result = *First;
  }
  FE.erase(++Result, FE.end());
}

// Group InputSections into clusters using the Call-Chain Clustering heuristic
// then sort the clusters by density.
void CallGraphSort::generateClusters() {
  for (size_t I = 0; I < Edges.size(); ++I)
    WorkQueue.push(I);
  // Collapse the graph.
  while (!WorkQueue.empty()) {
    EdgeIndex MaxI = WorkQueue.top();
    const Edge MaxE = Edges[MaxI];
    WorkQueue.pop();
    if (MaxE.isDead())
      continue;
    // Merge the Nodes.
    Node &From = Nodes[MaxE.From];
    Node &To = Nodes[MaxE.To];
    if (From.Size + To.Size > Target->PageSize)
      continue;
    contractEdge(MaxI);
    From.Sections.insert(From.Sections.end(), To.Sections.begin(),
                         To.Sections.end());
    From.Size += To.Size;
    From.Weight = SaturatingAdd(From.Weight, To.Weight);
    To.Sections.clear();
    To.Size = 0;
    To.Weight = 0;
  }

  // Remove empty or dead nodes.
  Nodes.erase(std::remove_if(Nodes.begin(), Nodes.end(),
                             [](const Node &N) {
                               return N.Size == 0 || N.Sections.empty();
                             }),
              Nodes.end());

  // Sort by density. Invalidates all NodeIndexs.
  std::sort(Nodes.begin(), Nodes.end(), [](const Node &A, const Node &B) {
    return (APFloat(APFloat::IEEEdouble(), A.Weight) /
            APFloat(APFloat::IEEEdouble(), A.Size))
               .compare(APFloat(APFloat::IEEEdouble(), B.Weight) /
                        APFloat(APFloat::IEEEdouble(), B.Size)) ==
           APFloat::cmpLessThan;
  });
}

DenseMap<const InputSectionBase *, int> CallGraphSort::run() {
  generateClusters();

  // Generate order.
  llvm::DenseMap<const InputSectionBase *, int> OrderMap;
  ssize_t CurOrder = 1;

  for (const Node &N : Nodes)
    for (const InputSectionBase *IS : N.Sections)
      OrderMap[IS] = CurOrder++;

  return OrderMap;
}

// Sort sections by the profile data provided by -callgraph-profile-file
//
// This first builds a call graph based on the profile data then iteratively
// merges the hottest call edges as long as it would not create a cluster larger
// than the page size. All clusters are then sorted by a density metric to
// further improve locality.
DenseMap<const InputSectionBase *, int> elf::computeCallGraphProfileOrder() {
  CallGraphSort CGS(Config->CallGraphProfile);
  return CGS.run();
}
