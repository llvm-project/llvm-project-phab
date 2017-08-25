//===-- ModuleSummaryIndex.cpp - Module Summary Index ---------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the module index and summary classes for the
// IR library.
//
//===----------------------------------------------------------------------===//

#include "llvm/IR/ModuleSummaryIndex.h"
#include "llvm/ADT/SCCIterator.h"
#include "llvm/ADT/StringMap.h"
using namespace llvm;

FunctionSummary FunctionSummary::ExternalNode =
    FunctionSummary::makeDummyFunctionSummary({});

// Collect for the given module the list of function it defines
// (GUID -> Summary).
void ModuleSummaryIndex::collectDefinedFunctionsForModule(
    StringRef ModulePath, GVSummaryMapTy &GVSummaryMap) const {
  for (auto &GlobalList : *this) {
    auto GUID = GlobalList.first;
    for (auto &GlobSummary : GlobalList.second.SummaryList) {
      auto *Summary = dyn_cast_or_null<FunctionSummary>(GlobSummary.get());
      if (!Summary)
        // Ignore global variable, focus on functions
        continue;
      // Ignore summaries from other modules.
      if (Summary->modulePath() != ModulePath)
        continue;
      GVSummaryMap[GUID] = Summary;
    }
  }
}

// Collect for each module the list of function it defines (GUID -> Summary).
void ModuleSummaryIndex::collectDefinedGVSummariesPerModule(
    StringMap<GVSummaryMapTy> &ModuleToDefinedGVSummaries) const {
  for (auto &GlobalList : *this) {
    auto GUID = GlobalList.first;
    for (auto &Summary : GlobalList.second.SummaryList) {
      ModuleToDefinedGVSummaries[Summary->modulePath()][GUID] = Summary.get();
    }
  }
}

GlobalValueSummary *
ModuleSummaryIndex::getGlobalValueSummary(uint64_t ValueGUID,
                                          bool PerModuleIndex) const {
  auto VI = getValueInfo(ValueGUID);
  assert(VI && "GlobalValue not found in index");
  assert((!PerModuleIndex || VI.getSummaryList().size() == 1) &&
         "Expected a single entry per global value in per-module index");
  auto &Summary = VI.getSummaryList()[0];
  return Summary.get();
}

bool ModuleSummaryIndex::isGUIDLive(GlobalValue::GUID GUID) const {
  auto VI = getValueInfo(GUID);
  if (!VI)
    return true;
  const auto &SummaryList = VI.getSummaryList();
  if (SummaryList.empty())
    return true;
  for (auto &I : SummaryList)
    if (isGlobalValueLive(I.get()))
      return true;
  return false;
}

LLVM_DUMP_METHOD
void ModuleSummaryIndex::dumpSCCs(raw_ostream &O) {
  for (scc_iterator<ModuleSummaryIndex *> I =
           scc_begin<ModuleSummaryIndex *>(this);
       !I.isAtEnd(); ++I) {
    O << "SCC (" << utostr(I->size()) << " node" << (I->size() == 1 ? "" : "s")
      << ") {\n";
    for (const ValueInfo V : *I) {
      FunctionSummary *F = nullptr;
      if (V.getSummaryList().size())
        F = cast<FunctionSummary>(V.getSummaryList().front().get());
      O << " " << (F ? F->modulePath() : "External") << " "
        << utostr(V.getGUID()) << (I.hasLoop() ? " (has loop)" : "") << "\n";
    }
    O << "}\n";
  }
}
