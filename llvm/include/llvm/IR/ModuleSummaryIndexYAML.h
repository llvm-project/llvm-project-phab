//===-- llvm/ModuleSummaryIndexYAML.h - YAML I/O for summary ----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_IR_MODULESUMMARYINDEXYAML_H
#define LLVM_IR_MODULESUMMARYINDEXYAML_H

#include "llvm/IR/ModuleSummaryIndex.h"
#include "llvm/Support/YAMLTraits.h"

namespace llvm {
namespace yaml {

using NameMapTy = std::map<GlobalValue::GUID, StringRef>;

template <typename T> struct NameMapWrapper {
  T *Val;
  NameMapTy *NameMap;

  StringRef getNameOrEmpty(GlobalValue::GUID G) {
    if (NameMap)
      return (*NameMap)[G];
    return StringRef("");
  }
};

template <> struct ScalarEnumerationTraits<TypeTestResolution::Kind> {
  static void enumeration(IO &io, TypeTestResolution::Kind &value) {
    io.enumCase(value, "Unsat", TypeTestResolution::Unsat);
    io.enumCase(value, "ByteArray", TypeTestResolution::ByteArray);
    io.enumCase(value, "Inline", TypeTestResolution::Inline);
    io.enumCase(value, "Single", TypeTestResolution::Single);
    io.enumCase(value, "AllOnes", TypeTestResolution::AllOnes);
  }
};

template <> struct ScalarEnumerationTraits<CalleeInfo::HotnessType> {
  static void enumeration(IO &io, CalleeInfo::HotnessType &value) {
    io.enumCase(value, "Unknown", CalleeInfo::HotnessType::Unknown);
    io.enumCase(value, "Cold", CalleeInfo::HotnessType::Cold);
    io.enumCase(value, "None", CalleeInfo::HotnessType::None);
    io.enumCase(value, "Hot", CalleeInfo::HotnessType::Hot);
  }
};

template <> struct ScalarEnumerationTraits<GlobalValueSummary::SummaryKind> {
  static void enumeration(IO &io, GlobalValueSummary::SummaryKind &value) {
    io.enumCase(value, "Alias", GlobalValueSummary::SummaryKind::AliasKind);
    io.enumCase(value, "Function",
                GlobalValueSummary::SummaryKind::FunctionKind);
    io.enumCase(value, "GlobalVar",
                GlobalValueSummary::SummaryKind::GlobalVarKind);
  }
};

template <> struct MappingTraits<TypeTestResolution> {
  static void mapping(IO &io, TypeTestResolution &res) {
    io.mapOptional("Kind", res.TheKind);
    io.mapOptional("SizeM1BitWidth", res.SizeM1BitWidth);
  }
};

template <>
struct ScalarEnumerationTraits<WholeProgramDevirtResolution::ByArg::Kind> {
  static void enumeration(IO &io,
                          WholeProgramDevirtResolution::ByArg::Kind &value) {
    io.enumCase(value, "Indir", WholeProgramDevirtResolution::ByArg::Indir);
    io.enumCase(value, "UniformRetVal",
                WholeProgramDevirtResolution::ByArg::UniformRetVal);
    io.enumCase(value, "UniqueRetVal",
                WholeProgramDevirtResolution::ByArg::UniqueRetVal);
    io.enumCase(value, "VirtualConstProp",
                WholeProgramDevirtResolution::ByArg::VirtualConstProp);
  }
};

template <> struct MappingTraits<WholeProgramDevirtResolution::ByArg> {
  static void mapping(IO &io, WholeProgramDevirtResolution::ByArg &res) {
    io.mapOptional("Kind", res.TheKind);
    io.mapOptional("Info", res.Info);
  }
};

template <>
struct CustomMappingTraits<
    std::map<std::vector<uint64_t>, WholeProgramDevirtResolution::ByArg>> {
  static void inputOne(
      IO &io, StringRef Key,
      std::map<std::vector<uint64_t>, WholeProgramDevirtResolution::ByArg> &V) {
    std::vector<uint64_t> Args;
    std::pair<StringRef, StringRef> P = {"", Key};
    while (!P.second.empty()) {
      P = P.second.split(',');
      uint64_t Arg;
      if (P.first.getAsInteger(0, Arg)) {
        io.setError("key not an integer");
        return;
      }
      Args.push_back(Arg);
    }
    io.mapRequired(Key.str().c_str(), V[Args]);
  }
  static void output(
      IO &io,
      std::map<std::vector<uint64_t>, WholeProgramDevirtResolution::ByArg> &V) {
    for (auto &P : V) {
      std::string Key;
      for (uint64_t Arg : P.first) {
        if (!Key.empty())
          Key += ',';
        Key += llvm::utostr(Arg);
      }
      io.mapRequired(Key.c_str(), P.second);
    }
  }
};

template <> struct ScalarEnumerationTraits<WholeProgramDevirtResolution::Kind> {
  static void enumeration(IO &io, WholeProgramDevirtResolution::Kind &value) {
    io.enumCase(value, "Indir", WholeProgramDevirtResolution::Indir);
    io.enumCase(value, "SingleImpl", WholeProgramDevirtResolution::SingleImpl);
  }
};

template <> struct MappingTraits<WholeProgramDevirtResolution> {
  static void mapping(IO &io, WholeProgramDevirtResolution &res) {
    io.mapOptional("Kind", res.TheKind);
    io.mapOptional("SingleImplName", res.SingleImplName);
    io.mapOptional("ResByArg", res.ResByArg);
  }
};

template <>
struct CustomMappingTraits<std::map<uint64_t, WholeProgramDevirtResolution>> {
  static void inputOne(IO &io, StringRef Key,
                       std::map<uint64_t, WholeProgramDevirtResolution> &V) {
    uint64_t KeyInt;
    if (Key.getAsInteger(0, KeyInt)) {
      io.setError("key not an integer");
      return;
    }
    io.mapRequired(Key.str().c_str(), V[KeyInt]);
  }
  static void output(IO &io, std::map<uint64_t, WholeProgramDevirtResolution> &V) {
    for (auto &P : V)
      io.mapRequired(llvm::utostr(P.first).c_str(), P.second);
  }
};

template <> struct MappingTraits<TypeIdSummary> {
  static void mapping(IO &io, TypeIdSummary& summary) {
    io.mapOptional("TTRes", summary.TTRes);
    io.mapOptional("WPDRes", summary.WPDRes);
  }
};

struct RefYaml {
  StringRef Name;
  GlobalValue::GUID GUID;
};

struct CalleeInfoYaml {
  StringRef Name;
  GlobalValue::GUID GUID;
  CalleeInfo::HotnessType Hotness;
};

struct GlobalValueSummaryYaml {
  StringRef Name;
  GlobalValueSummary::SummaryKind Kind;
  GlobalValue::LinkageTypes Linkage;
  bool NotEligibleToImport, Live;
  std::vector<RefYaml> Refs;
};

struct FunctionSummaryYaml {
  GlobalValueSummaryYaml GVSum;
  unsigned InstCount;
  std::vector<CalleeInfoYaml> Calls;

  std::vector<uint64_t> TypeTests;
  std::vector<FunctionSummary::VFuncId> TypeTestAssumeVCalls,
      TypeCheckedLoadVCalls;
  std::vector<FunctionSummary::ConstVCall> TypeTestAssumeConstVCalls,
      TypeCheckedLoadConstVCalls;
};

struct GlobalVarSummaryYaml {
  GlobalValueSummaryYaml GVSum;
};

struct AliasSummaryYaml {
  GlobalValueSummaryYaml GVSum;
  GlobalValue::GUID Aliasee;
  StringRef AliaseeName;
};

} // End yaml namespace
} // End llvm namespace

LLVM_YAML_IS_FLOW_SEQUENCE_VECTOR(uint64_t)

namespace llvm {
namespace yaml {

template <> struct MappingTraits<FunctionSummary::VFuncId> {
  static void mapping(IO &io, FunctionSummary::VFuncId& id) {
    io.mapOptional("GUID", id.GUID);
    io.mapOptional("Offset", id.Offset);
  }
};

template <> struct MappingTraits<FunctionSummary::ConstVCall> {
  static void mapping(IO &io, FunctionSummary::ConstVCall& id) {
    io.mapOptional("VFunc", id.VFunc);
    io.mapOptional("Args", id.Args);
  }
};

} // End yaml namespace
} // End llvm namespace

LLVM_YAML_IS_SEQUENCE_VECTOR(FunctionSummary::VFuncId)
LLVM_YAML_IS_SEQUENCE_VECTOR(FunctionSummary::ConstVCall)

namespace llvm {
namespace yaml {

template <> struct MappingTraits<RefYaml> {
  static void mapping(IO &io, RefYaml &summary) {
    io.mapOptional("GUID", summary.GUID);
    io.addComment(Comment(summary.Name, false));
  }
};

template <> struct MappingTraits<CalleeInfoYaml> {
  static void mapping(IO &io, CalleeInfoYaml &summary) {
    io.mapOptional("GUID", summary.GUID);
    io.addComment(Comment(summary.Name, false));
    io.mapOptional("Hotness", summary.Hotness);
  }
};

static void mapGlobalValueSummary(IO &io, GlobalValueSummaryYaml &summary) {
  io.addComment(Comment(summary.Name));
  io.mapOptional("Kind", summary.Kind);
  io.mapOptional("Linkage", summary.Linkage);
  io.mapOptional("NotEligibleToImport", summary.NotEligibleToImport);
  io.mapOptional("Live", summary.Live);
  io.mapOptional("Refs", summary.Refs);
}

template <> struct MappingTraits<FunctionSummaryYaml> {
  static void mapping(IO &io, FunctionSummaryYaml& summary) {
    mapGlobalValueSummary(io, summary.GVSum);
    io.mapOptional("InstCount", summary.InstCount);
    io.mapOptional("Calls", summary.Calls);
    io.mapOptional("TypeTests", summary.TypeTests);
    io.mapOptional("TypeTestAssumeVCalls", summary.TypeTestAssumeVCalls);
    io.mapOptional("TypeCheckedLoadVCalls", summary.TypeCheckedLoadVCalls);
    io.mapOptional("TypeTestAssumeConstVCalls",
                   summary.TypeTestAssumeConstVCalls);
    io.mapOptional("TypeCheckedLoadConstVCalls",
                   summary.TypeCheckedLoadConstVCalls);
  }
};

template <> struct MappingTraits<GlobalVarSummaryYaml> {
  static void mapping(IO &io, GlobalVarSummaryYaml &summary) {
    mapGlobalValueSummary(io, summary.GVSum);
  }
};

template <> struct MappingTraits<AliasSummaryYaml> {
  static void mapping(IO &io, AliasSummaryYaml &summary) {
    mapGlobalValueSummary(io, summary.GVSum);
    io.mapOptional("Aliasee", summary.Aliasee);
    io.addComment(Comment{summary.AliaseeName, false});
  }
};

} // End yaml namespace
} // End llvm namespace

LLVM_YAML_IS_STRING_MAP(TypeIdSummary)
LLVM_YAML_IS_SEQUENCE_VECTOR(FunctionSummaryYaml)
LLVM_YAML_IS_SEQUENCE_VECTOR(CalleeInfoYaml)
LLVM_YAML_IS_SEQUENCE_VECTOR(RefYaml)
LLVM_YAML_IS_SEQUENCE_VECTOR(GlobalVarSummaryYaml)
LLVM_YAML_IS_SEQUENCE_VECTOR(AliasSummaryYaml)
LLVM_YAML_IS_FLOW_SEQUENCE_VECTOR(std::string)

namespace llvm {
namespace yaml {

template <> struct ScalarEnumerationTraits<GlobalValue::LinkageTypes> {
  static void enumeration(IO &io, GlobalValue::LinkageTypes &value) {
    io.enumCase(value, "ExternalLinkage",
                GlobalValue::LinkageTypes::ExternalLinkage);
    io.enumCase(value, "AvailableExternallyLinkage",
                GlobalValue::LinkageTypes::AvailableExternallyLinkage);
    io.enumCase(value, "LinkOnceAnyLinkage",
                GlobalValue::LinkageTypes::LinkOnceAnyLinkage);
    io.enumCase(value, "LinkOnceODRLinkage",
                GlobalValue::LinkageTypes::LinkOnceODRLinkage);
    io.enumCase(value, "WeakAnyLinkage",
                GlobalValue::LinkageTypes::WeakAnyLinkage);
    io.enumCase(value, "WeakODRLinkage",
                GlobalValue::LinkageTypes::WeakODRLinkage);
    io.enumCase(value, "AppendingLinkage",
                GlobalValue::LinkageTypes::AppendingLinkage);
    io.enumCase(value, "InternalLinkage",
                GlobalValue::LinkageTypes::InternalLinkage);
    io.enumCase(value, "PrivateLinkage",
                GlobalValue::LinkageTypes::PrivateLinkage);
    io.enumCase(value, "ExternalWeakLinkage",
                GlobalValue::LinkageTypes::ExternalWeakLinkage);
    io.enumCase(value, "CommonLinkage",
                GlobalValue::LinkageTypes::CommonLinkage);
  }
};

// FIXME: Add YAML mappings for the rest of the module summary.
template <>
struct CustomMappingTraits<NameMapWrapper<GlobalValueSummaryMapTy>> {
  static void inputOne(IO &io, StringRef Key,
                       NameMapWrapper<GlobalValueSummaryMapTy> &V) {
    std::vector<FunctionSummaryYaml> FSums;
    std::vector<GlobalVarSummaryYaml> GVSums;
    std::vector<AliasSummaryYaml> ASums;
    io.mapRequired(Key.str().c_str(), FSums);
    io.mapRequired(Key.str().c_str(), GVSums);
    io.mapRequired(Key.str().c_str(), ASums);
    auto &Elem = (*V.Val)[std::stoi(Key)];
    for (auto &FSum : FSums) {
      Elem.SummaryList.push_back(llvm::make_unique<FunctionSummary>(
          GlobalValueSummary::GVFlags(
              static_cast<GlobalValue::LinkageTypes>(FSum.GVSum.Linkage),
              FSum.GVSum.NotEligibleToImport, FSum.GVSum.Live),
          0, ArrayRef<ValueInfo>{}, ArrayRef<FunctionSummary::EdgeTy>{},
          std::move(FSum.TypeTests), std::move(FSum.TypeTestAssumeVCalls),
          std::move(FSum.TypeCheckedLoadVCalls),
          std::move(FSum.TypeTestAssumeConstVCalls),
          std::move(FSum.TypeCheckedLoadConstVCalls)));
    }

    for (auto &GVSum : GVSums) {
      Elem.SummaryList.push_back(llvm::make_unique<GlobalVarSummary>(
          GlobalValueSummary::GVFlags(
              static_cast<GlobalValue::LinkageTypes>(GVSum.GVSum.Linkage),
              GVSum.GVSum.NotEligibleToImport, GVSum.GVSum.Live),
          ArrayRef<ValueInfo>{}));
    }

    for (auto &ASum : ASums) {
      Elem.SummaryList.push_back(llvm::make_unique<AliasSummary>(
          GlobalValueSummary::GVFlags(
              static_cast<GlobalValue::LinkageTypes>(ASum.GVSum.Linkage),
              ASum.GVSum.NotEligibleToImport, ASum.GVSum.Live),
          ArrayRef<ValueInfo>{}));
    }
  }

  static void output(IO &io, NameMapWrapper<GlobalValueSummaryMapTy> &V) {
    for (auto &P : *V.Val) {
      std::vector<FunctionSummaryYaml> FSums;
      std::vector<GlobalVarSummaryYaml> GVSums;
      std::vector<AliasSummaryYaml> ASums;
      for (auto &Sum : P.second.SummaryList) {
        std::vector<RefYaml> Refs;
        for (auto i : Sum->refs()) {
          Refs.push_back({V.getNameOrEmpty(i.getGUID()), i.getGUID()});
        }
        GlobalValueSummaryYaml GVSumY = {
            V.getNameOrEmpty(P.first),
            Sum->getSummaryKind(),
            static_cast<GlobalValue::LinkageTypes>(Sum->flags().Linkage),
            static_cast<bool>(Sum->flags().NotEligibleToImport),
            static_cast<bool>(Sum->flags().Live),
            Refs};

        if (auto *FSum = dyn_cast<FunctionSummary>(Sum.get())) {
          std::vector<CalleeInfoYaml> Calls;
          for (auto edge : FSum->calls()) {
            Calls.push_back(
                CalleeInfoYaml{V.getNameOrEmpty(edge.first.getGUID()),
                               edge.first.getGUID(), edge.second.Hotness});
          }
          FSums.push_back(FunctionSummaryYaml{
              GVSumY, FSum->instCount(), Calls, FSum->type_tests(),
              FSum->type_test_assume_vcalls(), FSum->type_checked_load_vcalls(),
              FSum->type_test_assume_const_vcalls(),
              FSum->type_checked_load_const_vcalls()});
        }
        if (dyn_cast<GlobalVarSummary>(Sum.get()))
          GVSums.push_back(GlobalVarSummaryYaml{GVSumY});
        if (auto *ASum = dyn_cast<AliasSummary>(Sum.get()))
          ASums.push_back(AliasSummaryYaml{
              GVSumY, ASum->getAliasee().getOriginalName(),
              V.getNameOrEmpty(ASum->getAliasee().getOriginalName())});
      }
      if (!FSums.empty())
        io.mapRequired(utostr(P.first).c_str(), FSums);
      if (!GVSums.empty())
        io.mapRequired(utostr(P.first).c_str(), GVSums);
      if (!ASums.empty())
        io.mapRequired(utostr(P.first).c_str(), ASums);
    }
  }
};

struct ModuleSummaryIndexWithModule {
  ModuleSummaryIndex *Index;
  Module *Mod;
};

template <> struct MappingTraits<ModuleSummaryIndexWithModule> {
  static void mapping(IO &io, ModuleSummaryIndexWithModule &M) {
    NameMapTy NameMap;
    if (M.Mod) {
      for (GlobalValue &V : M.Mod->global_values()) {
        NameMap.insert(std::make_pair(V.getGUID(), V.getName()));
      }
    }
    NameMapWrapper<GlobalValueSummaryMapTy> N = {&M.Index->GlobalValueMap,
                                                 &NameMap};
    io.mapRequired("ModuleSummaryIndex", N);
  }
};

template <> struct MappingTraits<ModuleSummaryIndex> {
  static void mapping(IO &io, ModuleSummaryIndex& index) {
    NameMapWrapper<GlobalValueSummaryMapTy> M = {&index.GlobalValueMap,
                                                 nullptr};
    io.addComment(Comment{"Use output for debugging/testing purposes only"});
    io.mapOptional("GlobalValueMap", M);
    io.mapOptional("TypeIdMap", index.TypeIdMap);
    io.mapOptional("WithGlobalValueDeadStripping",
                   index.WithGlobalValueDeadStripping);

    if (io.outputting()) {
      std::vector<std::string> CfiFunctionDefs(index.CfiFunctionDefs.begin(),
                                               index.CfiFunctionDefs.end());
      io.mapOptional("CfiFunctionDefs", CfiFunctionDefs);
      std::vector<std::string> CfiFunctionDecls(index.CfiFunctionDecls.begin(),
                                                index.CfiFunctionDecls.end());
      io.mapOptional("CfiFunctionDecls", CfiFunctionDecls);
    } else {
      std::vector<std::string> CfiFunctionDefs;
      io.mapOptional("CfiFunctionDefs", CfiFunctionDefs);
      index.CfiFunctionDefs = {CfiFunctionDefs.begin(), CfiFunctionDefs.end()};
      std::vector<std::string> CfiFunctionDecls;
      io.mapOptional("CfiFunctionDecls", CfiFunctionDecls);
      index.CfiFunctionDecls = {CfiFunctionDecls.begin(),
                                CfiFunctionDecls.end()};
    }
  }
};

} // End yaml namespace
} // End llvm namespace

#endif
