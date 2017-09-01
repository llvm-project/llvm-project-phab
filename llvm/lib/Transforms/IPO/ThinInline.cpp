#include "llvm/Transforms/IPO/ThinInline.h"
#include "llvm/ADT/SCCIterator.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

//===----------------------------------------------------------------------===//
// Implementations of the ThinInline class methods.
//

void ThinInline::ComputeThinInlineDecision(ThinInlineDecision &Decision) {
  auto TCG = ThinCallGraph(TheIndex);
  // TCG.printInSCC(errs());
  GenerateThinInlineDecisionInPriorityOrder(Decision, TCG);
}

void ThinInline::GenerateThinInlineDecisionInPriorityOrder(
    ThinInlineDecision &Decision, ThinCallGraph &TCG) {
  if (!Decision.empty())
    Decision.clear();
  GeneratePriorityWorkList(TCG);
  while (!PriorityWorkList.empty()) {
    auto MaxEdgeIter = getMaxBenefitsEdgeIter();
    if (MaxEdgeIter == PriorityWorkList.end())
      break;
    auto MaxEdge = *MaxEdgeIter;
    PriorityWorkList.erase(MaxEdgeIter);
    if (MaxEdge->getBenefits() <= 0)
      break;
    Decision.push_back(std::make_tuple(
        MaxEdge->getCaller()->getGUID(), MaxEdge->getCallee()->getGUID(),
        MaxEdge->getCSID(), MaxEdge->getCaller()->getMaxCSID() + 1));
    ApplyInlineOnEdge(MaxEdge);
  }
}

void ThinInline::GeneratePriorityWorkList(ThinCallGraph &TCG) {
  if (!PriorityWorkList.empty())
    PriorityWorkList.clear();
  TCG.ComputeWholeGraphBenefits();
  for (auto &I : TCG) {
    auto TCGN = I.second.get();
    for (auto &CE : TCGN->getCallEdges()) {
      auto &TCE = CE.second;
      // If Benefits <= 0, it should never be inlined, don't have to insert it
      // to PriorityWorkList.
      if (TCE.getBenefits() > 0)
        PriorityWorkList.push_back(&TCE);
    }
  }
}

std::vector<ThinCallEdge *>::iterator ThinInline::getMaxBenefitsEdgeIter() {
  if (PriorityWorkList.empty())
    return PriorityWorkList.end();
  auto Max = PriorityWorkList.begin();
  for (auto I = Max + 1, End = PriorityWorkList.end(); I != End; ++I)
    if ((*Max)->getBenefits() < (*I)->getBenefits())
      Max = I;
  if ((*Max)->getBenefits() <= 0)
    return PriorityWorkList.end();
  return Max;
}

// Get new hotness from inlined edge and callee's call edge.
static CalleeInfo::HotnessType getNewHotness(CalleeInfo::HotnessType H1,
                                             CalleeInfo::HotnessType H2) {
  if (H1 == CalleeInfo::HotnessType::Hot ||
      H1 == CalleeInfo::HotnessType::Critical)
    return CalleeInfo::HotnessType::Hot;
  if (H2 == CalleeInfo::HotnessType::Critical)
    return CalleeInfo::HotnessType::Critical;
  if (H1 == CalleeInfo::HotnessType::Cold &&
      H2 == CalleeInfo::HotnessType::Cold)
    return CalleeInfo::HotnessType::Cold;
  return CalleeInfo::HotnessType::Unknown;
}

void ThinInline::ApplyInlineOnEdge(ThinCallEdge *Edge) {
  auto Caller = Edge->getCaller();
  auto Callee = Edge->getCallee();
  auto &CEs = Caller->getCallEdges();
  auto CSID = Edge->getCSID();
  // Remove this inlined Call Edge from Caller's Call Edges.
  for (auto I = CEs.begin(), End = CEs.end(); I != End; ++I) {
    if (I->first == CSID) {
      CEs.erase(I);
      break;
    }
  }

  // Add Callee's Call Edge into Caller.
  for (auto &I : Callee->getCallEdges()) {
    auto NewCSID = Caller->getIncreasedMaxCSID();
    auto NewHotness = getNewHotness(Edge->getHotness(), I.second.getHotness());
    // Benefits will be computed later, together with all the edges contains
    // this inlined call edge's caller and callee.
    PriorityWorkList.push_back(new ThinCallEdge{Caller, I.second.getCallee(),
                                                NewHotness, NewCSID,
                                                I.second.getInlineFlag()});
  }

  // Reset caller's size as its original size + inlined callee's size.
  Caller->setInstCount(Caller->getInstCount() + Callee->getInstCount());
  // Update all the edges related to caller in priority work list since its size
  // changed after inline.
  for (auto I : PriorityWorkList) {
    if (I->getCaller() == Caller || I->getCallee() == Caller)
      I->UpdateBenefits();
  }
  // Remove the edges whose benefits <= 0 in priority work list.
  // PriorityWorkList.erase(
  //      std::remove_if(PriorityWorkList.begin(), PriorityWorkList.end(),
  //                     [](ThinCallEdge *E) { return E->getBenefits() <= 0;
  //                     }));
}

//===----------------------------------------------------------------------===//
// Implementations of the ThinCallGraph class methods.
//

void ThinCallGraph::buildThinCallGraphFromModuleSummary() {
  for (auto &I : TheIndex)
    addToThinCallGraph(I.first);
  assert(EntryNode && "No entry node set.\n");
}

void ThinCallGraph::addToThinCallGraph(const GlobalValue::GUID GUID) {
  ThinCallGraphNode *Node = getOrInsertNode(GUID);
  if (auto VI = TheIndex.getValueInfo(GUID)) {
    auto SL = VI.getSummaryList();
    if (!SL.empty())
      if (auto FS = dyn_cast<FunctionSummary>(SL[0].get()))
        for (auto &CE : FS->calls())
          Node->addCalledFunction(CE, getOrInsertNode(CE.first.getGUID()));
  }
}

ThinCallGraphNode *
ThinCallGraph::getOrInsertNode(const GlobalValue::GUID GUID) {
  auto &CGN = GUIDNodeMap[GUID];
  if (CGN)
    return CGN.get();
  CGN = make_unique<ThinCallGraphNode>(GUID, *this);
  auto FS = CGN.get()->getFunctionSummary();
  if (FS) {
    CGN.get()->setInstCount(FS->instCount());
    // Set main function as the entry of thin call graph.
    if (FS->getOriginalName() == GlobalValue::getGUID("main")) {
      assert(!EntryNode && "EntryNode is already set!\n");
      setEntryNode(CGN.get());
    }
  } else
    CGN.get()->setDeclare(true);
  return CGN.get();
}

void ThinCallGraph::printInSCC(raw_ostream &OS) {
  scc_iterator<ThinCallGraph *> CGI = scc_begin(this);
  while (!CGI.isAtEnd()) {
    const std::vector<ThinCallGraphNode *> &NodeVec = *CGI;
    for (auto TCGN : NodeVec) {
      OS << "Caller: " << TCGN->getGUID() << "\n";
      for (auto &CE : TCGN->getCallEdges())
        OS << "  Callee: " << CE.second.getCallee()->getGUID()
           << ", Call Site Order: " << CE.second.getCSID() << ", Inline Flag: "
           << static_cast<uint32_t>(CE.second.getInlineFlag()) << "\n";
    }
    OS << "\n";
    ++CGI;
  }
}

void ThinCallGraph::ComputeWholeGraphBenefits() {
  for (auto &GNM : GUIDNodeMap) {
    auto TCGN = GNM.second.get();
    for (auto &CE : TCGN->getCallEdges())
      CE.second.UpdateBenefits();
  }
}

//===----------------------------------------------------------------------===//
// Implementations of the ThinCallGraphNode class methods.
//

FunctionSummary *ThinCallGraphNode::getFunctionSummary() {
  auto &Summary = TheGraph.getModuleSummaryIndex();
  auto SL = Summary.getValueInfo(TheGUID).getSummaryList();
  if (!SL.empty())
    if (FunctionSummary *FS = dyn_cast<FunctionSummary>(SL[0].get()))
      return FS;
  return nullptr;
}

void ThinCallGraphNode::addCalledFunction(FunctionSummary::EdgeTy CE,
                                          ThinCallGraphNode *TCGN) {
  CallEdges.push_back({CE.second.CSID, ThinCallEdge{this, TCGN, CE.second}});
  MaxCSID = std::max(MaxCSID, CE.second.CSID);
}

//===----------------------------------------------------------------------===//
// Implementations of the ThinCallEdge class methods.
//

void ThinCallEdge::UpdateBenefits() {
  if (Caller->isDeclare() || Callee->isDeclare()) {
    Benefits = ThinInlineConstants::NeverInline;
    return;
  }
  int Threshold = ThinInlineConstants::DefaultThreshold;
  switch (InlineFlag) {
  default:
    Threshold = ThinInlineConstants::NeverInline;
    break;
  case CalleeInfo::InlineFlagType::Always:
    Threshold = ThinInlineConstants::AlwaysInline;
    break;
  case CalleeInfo::InlineFlagType::Never:
    Threshold = ThinInlineConstants::NeverInline;
    break;
  case CalleeInfo::InlineFlagType::Default:
    if (Hotness == CalleeInfo::HotnessType::Hot)
      Threshold = ThinInlineConstants::HotThreshold;
    else if (Hotness == CalleeInfo::HotnessType::Cold)
      Threshold = ThinInlineConstants::ColdThreshold;
    else if (Hotness == CalleeInfo::HotnessType::Critical)
      Threshold = ThinInlineConstants::CriticalThreshold;
    break;
  }
  Benefits =
      Threshold - (int64_t)(Caller->getInstCount() + Callee->getInstCount());
}
