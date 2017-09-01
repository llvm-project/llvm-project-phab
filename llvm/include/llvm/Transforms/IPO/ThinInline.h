#ifndef LLVM_THININLINE_H
#define LLVM_THININLINE_H

#include "llvm/IR/ModuleSummaryIndex.h"

#include <memory>
#include <vector>

namespace llvm {
class ThinCallGraph;
class ThinCallGraphNode;
class ThinCallEdge;

namespace ThinInlineConstants {
const int32_t DefaultThreshold = 600;

const int32_t HotThreshold = 2000;

const int32_t ColdThreshold = 200;

const int32_t CriticalThreshold = 5000;

const int32_t AlwaysInline = 5000;

const int32_t NeverInline = -1;
} // namespace ThinInlineConstants

/// \brief ThinInline decision.
class ThinInlineDecision {
public:
  // <Caller GUID, Callee GUID, CSID, NewCSID>
  using ThinInlineDecisionTy =
      std::tuple<GlobalValue::GUID, GlobalValue::GUID, uint32_t, uint32_t>;
  using ThinInlineDecisionVecTy = std::vector<ThinInlineDecisionTy>;

private:
  ThinInlineDecisionVecTy Decision;

public:
  ThinInlineDecisionVecTy &getDecision() { return Decision; }

  typedef ThinInlineDecisionVecTy::iterator iterator;
  typedef ThinInlineDecisionVecTy::const_iterator const_iterator;

  inline iterator begin() { return Decision.begin(); }
  inline iterator end() { return Decision.end(); }
  inline const_iterator begin() const { return Decision.begin(); }
  inline const_iterator end() const { return Decision.end(); }

  ThinInlineDecision &push_back(const ThinInlineDecisionTy InlinedEdge) {
    Decision.push_back(InlinedEdge);
    return *this;
  }

  bool empty() { return Decision.empty(); }

  void clear() { Decision.clear(); }

  // Static methods to get the specific info in a inlined edge.
  // Should be updated whenever the ThinInlineDecisionTy changed.
  static GlobalValue::GUID
  getCallerGUID(const ThinInlineDecisionTy &InlinedEdge) {
    return std::get<0>(InlinedEdge);
  }
  static GlobalValue::GUID
  getCalleeGUID(const ThinInlineDecisionTy &InlinedEdge) {
    return std::get<1>(InlinedEdge);
  }
  static uint32_t getCSID(const ThinInlineDecisionTy &InlinedEdge) {
    return std::get<2>(InlinedEdge);
  }
  static uint32_t getNewCSID(const ThinInlineDecisionTy &InlinedEdge) {
    return std::get<3>(InlinedEdge);
  }
};

/// \brief ThinInline basic class.
class ThinInline {
private:
  ModuleSummaryIndex &TheIndex;

  std::vector<ThinCallEdge *> PriorityWorkList;

public:
  ThinInline(ModuleSummaryIndex &Index) : TheIndex(Index) {}

  void ComputeThinInlineDecision(ThinInlineDecision &Decision);

private:
  void GenerateThinInlineDecisionInPriorityOrder(ThinInlineDecision &Decision,
                                                 ThinCallGraph &TCG);

  void GeneratePriorityWorkList(ThinCallGraph &TCG);

  std::vector<ThinCallEdge *>::iterator getMaxBenefitsEdgeIter();

  void ApplyInlineOnEdge(ThinCallEdge *Edge);
};

/// \brief The basic data container for the thin call graph of a module summary.
class ThinCallGraph {
private:
  ModuleSummaryIndex &TheIndex;

  using GUIDNodeMapTy =
      std::map<const GlobalValue::GUID, std::unique_ptr<ThinCallGraphNode>>;

  /// \brief A map from \c GUID to \c ThinCallGraphNode*.
  GUIDNodeMapTy GUIDNodeMap;

  // The entry of the thin call graph.
  // Since we thin call graph is for whole program, the entry should be main
  // function.
  ThinCallGraphNode *EntryNode;

public:
  ThinCallGraph(ModuleSummaryIndex &Index) : TheIndex(Index) {
    buildThinCallGraphFromModuleSummary();
  }

  ModuleSummaryIndex &getModuleSummaryIndex() { return TheIndex; }

  void setEntryNode(ThinCallGraphNode *EN) { EntryNode = EN; }

  ThinCallGraphNode *getEntryNode() const { return EntryNode; }

  void printInSCC(raw_ostream &OS);

  void ComputeWholeGraphBenefits();

  typedef GUIDNodeMapTy::iterator iterator;
  typedef GUIDNodeMapTy::const_iterator const_iterator;

  inline iterator begin() { return GUIDNodeMap.begin(); }
  inline iterator end() { return GUIDNodeMap.end(); }
  inline const_iterator begin() const { return GUIDNodeMap.begin(); }
  inline const_iterator end() const { return GUIDNodeMap.end(); }

  /// \brief Returns the call graph node for the provided function.
  inline const ThinCallGraphNode *
  operator[](const GlobalValue::GUID GUID) const {
    const_iterator I = GUIDNodeMap.find(GUID);
    assert(I != GUIDNodeMap.end() && "Function not in callgraph!");
    return I->second.get();
  }

  /// \brief Returns the call graph node for the provided function.
  inline ThinCallGraphNode *operator[](const GlobalValue::GUID GUID) {
    const_iterator I = GUIDNodeMap.find(GUID);
    assert(I != GUIDNodeMap.end() && "Function not in callgraph!");
    return I->second.get();
  }

private:
  void buildThinCallGraphFromModuleSummary();

  void addToThinCallGraph(const GlobalValue::GUID GUID);

  ThinCallGraphNode *getOrInsertNode(const GlobalValue::GUID GUID);
};

/// \brief A node in the thin call graph for a module summary.
///
/// Typically represents a function summary in the thin call graph.
class ThinCallGraphNode {
public:
  using CallRecord = std::pair<uint32_t, ThinCallEdge>;

private:
  GlobalValue::GUID TheGUID;

  ThinCallGraph &TheGraph;

  std::vector<CallRecord> CallEdges;

  // This node's max CallSite ID.
  // After inlining, the Callee's CallSite will be inserted into Caller,
  // and their CallSite ID will start from MaxCSID+1.
  uint32_t MaxCSID;

  uint32_t InstCount;

  bool Declare = false;

public:
  ThinCallGraphNode(GlobalValue::GUID GUID, ThinCallGraph &TCG)
      : TheGUID(GUID), TheGraph(TCG), MaxCSID(0), InstCount(0) {}

  void addCalledFunction(FunctionSummary::EdgeTy CE, ThinCallGraphNode *TCGN);

  FunctionSummary *getFunctionSummary();

  GlobalValue::GUID getGUID() { return TheGUID; }

  std::vector<CallRecord> &getCallEdges() { return CallEdges; }

  uint32_t getMaxCSID() { return MaxCSID; }

  uint32_t getIncreasedMaxCSID() { return ++MaxCSID; }

  uint32_t getInstCount() { return InstCount; }

  void setInstCount(uint32_t Count) { InstCount = Count; }

  void setDeclare(bool D) { Declare = D; }

  bool isDeclare() { return Declare; }

  typedef std::vector<CallRecord>::iterator iterator;
  typedef std::vector<CallRecord>::const_iterator const_iterator;

  inline iterator begin() { return CallEdges.begin(); }
  inline iterator end() { return CallEdges.end(); }
  inline const_iterator begin() const { return CallEdges.begin(); }
  inline const_iterator end() const { return CallEdges.end(); }
  inline bool empty() const { return CallEdges.empty(); }
  inline unsigned size() const { return (unsigned)CallEdges.size(); }
};

/// \brief An edge in the thin call graph for a module summary.
///
/// Typically represens a call site in the thin call graph.
class ThinCallEdge {
private:
  ThinCallGraphNode *Caller;

  ThinCallGraphNode *Callee;

  CalleeInfo::HotnessType Hotness;

  uint32_t CSID;

  CalleeInfo::InlineFlagType InlineFlag;

  int64_t Benefits;

public:
  ThinCallEdge(ThinCallGraphNode *Caller, ThinCallGraphNode *Callee,
               CalleeInfo CI)
      : Caller(Caller), Callee(Callee), Hotness(CI.Hotness), CSID(CI.CSID),
        InlineFlag(CI.InlineFlag) {}

  ThinCallEdge(ThinCallGraphNode *Caller, ThinCallGraphNode *Callee,
               CalleeInfo::HotnessType Hotness, uint32_t CSID,
               CalleeInfo::InlineFlagType InlineFlag)
      : Caller(Caller), Callee(Callee), Hotness(Hotness), CSID(CSID),
        InlineFlag(InlineFlag) {}

  ThinCallGraphNode *getCaller() { return Caller; }

  ThinCallGraphNode *getCallee() { return Callee; }

  CalleeInfo::HotnessType getHotness() { return Hotness; }

  uint32_t getCSID() { return CSID; }

  CalleeInfo::InlineFlagType getInlineFlag() { return InlineFlag; }

  int64_t getBenefits() { return Benefits; }

  void UpdateBenefits();
};

//===----------------------------------------------------------------------===//
// GraphTraits specializations for thin call graphs so that they can be treated
// as graphs by the generic graph algorithms.
//

// Provide graph traits for tranversing thin call graphs using standard graph
// traversals.
template <> struct GraphTraits<ThinCallGraphNode *> {
  typedef ThinCallGraphNode *NodeRef;

  typedef ThinCallGraphNode::CallRecord CGNPairTy;

  static NodeRef getEntryNode(ThinCallGraphNode *CGN) { return CGN; }

  static ThinCallGraphNode *CGNGetValue(CGNPairTy P) {
    return P.second.getCallee();
  }

  typedef mapped_iterator<ThinCallGraphNode::iterator, decltype(&CGNGetValue)>
      ChildIteratorType;

  static ChildIteratorType child_begin(NodeRef N) {
    return ChildIteratorType(N->begin(), &CGNGetValue);
  }
  static ChildIteratorType child_end(NodeRef N) {
    return ChildIteratorType(N->end(), &CGNGetValue);
  }
};

template <> struct GraphTraits<const ThinCallGraphNode *> {
  typedef const ThinCallGraphNode *NodeRef;

  typedef ThinCallGraphNode::CallRecord CGNPairTy;

  static NodeRef getEntryNode(const ThinCallGraphNode *CGN) { return CGN; }

  static const ThinCallGraphNode *CGNGetValue(CGNPairTy P) {
    return P.second.getCallee();
  }

  typedef mapped_iterator<ThinCallGraphNode::const_iterator,
                          decltype(&CGNGetValue)>
      ChildIteratorType;

  static ChildIteratorType child_begin(NodeRef N) {
    return ChildIteratorType(N->begin(), &CGNGetValue);
  }
  static ChildIteratorType child_end(NodeRef N) {
    return ChildIteratorType(N->end(), &CGNGetValue);
  }
};

template <>
struct GraphTraits<ThinCallGraph *> : public GraphTraits<ThinCallGraphNode *> {
  static NodeRef getEntryNode(ThinCallGraph *CGN) {
    return CGN->getEntryNode(); // Start at main!
  }
  typedef std::pair<const GlobalValue::GUID, std::unique_ptr<ThinCallGraphNode>>
      PairTy;
  static ThinCallGraphNode *CGGetValuePtr(PairTy &P) { return P.second.get(); }

  // nodes_iterator/begin/end - Allow iteration over all nodes in the graph
  typedef mapped_iterator<ThinCallGraph::iterator, decltype(&CGGetValuePtr)>
      nodes_iterator;
  static nodes_iterator nodes_begin(ThinCallGraph *CG) {
    return nodes_iterator(CG->begin(), &CGGetValuePtr);
  }
  static nodes_iterator nodes_end(ThinCallGraph *CG) {
    return nodes_iterator(CG->end(), &CGGetValuePtr);
  }
};

template <>
struct GraphTraits<const ThinCallGraph *>
    : public GraphTraits<const ThinCallGraphNode *> {
  static NodeRef getEntryNode(const ThinCallGraph *CGN) {
    return CGN->getEntryNode(); // Start at main!
  }
  typedef std::pair<const GlobalValue::GUID, std::unique_ptr<ThinCallGraphNode>>
      PairTy;
  static const ThinCallGraphNode *CGGetValuePtr(const PairTy &P) {
    return P.second.get();
  }

  // nodes_iterator/begin/end - Allow iteration over all nodes in the graph
  typedef mapped_iterator<ThinCallGraph::const_iterator,
                          decltype(&CGGetValuePtr)>
      nodes_iterator;
  static nodes_iterator nodes_begin(const ThinCallGraph *CG) {
    return nodes_iterator(CG->begin(), &CGGetValuePtr);
  }
  static nodes_iterator nodes_end(const ThinCallGraph *CG) {
    return nodes_iterator(CG->end(), &CGGetValuePtr);
  }
};

} // namespace llvm

#endif
