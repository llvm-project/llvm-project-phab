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

struct CalleeInfoYaml {
  std::string Name;
  GlobalValue::GUID GUID;
  CalleeInfo::HotnessType Hotness;
};

struct FunctionSummaryYaml {
  GlobalValue::GUID GUID;
  GlobalValueSummary::SummaryKind Kind;
  GlobalValue::LinkageTypes Linkage;
  bool NotEligibleToImport, Live;
  unsigned InstCount;
  std::vector<CalleeInfoYaml> Calls;

  std::vector<uint64_t> TypeTests;
  std::vector<FunctionSummary::VFuncId> TypeTestAssumeVCalls,
      TypeCheckedLoadVCalls;
  std::vector<FunctionSummary::ConstVCall> TypeTestAssumeConstVCalls,
      TypeCheckedLoadConstVCalls;
};

struct GlobalVarSummaryYaml {
  GlobalValue::GUID GUID;
  GlobalValueSummary::SummaryKind Kind;
  GlobalValue::LinkageTypes Linkage;
  bool NotEligibleToImport, Live;
};

struct AliasSummaryYaml {
  GlobalValue::GUID GUID;
  GlobalValueSummary::SummaryKind Kind;
  GlobalValue::LinkageTypes Linkage;
  bool NotEligibleToImport, Live;
  std::string Aliasee;
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

template <> struct MappingTraits<CalleeInfoYaml> {
  static void mapping(IO &io, CalleeInfoYaml &summary) {
    io.mapOptional("Name", summary.Name);
    io.mapOptional("GUID", summary.GUID);
    io.mapOptional("Hotness", summary.Hotness);
  }
};

template <> struct MappingTraits<FunctionSummaryYaml> {
  static void mapping(IO &io, FunctionSummaryYaml& summary) {
    io.mapOptional("GUID", summary.GUID);
    io.mapOptional("Kind", summary.Kind);
    io.mapOptional("Linkage", summary.Linkage);
    io.mapOptional("NotEligibleToImport", summary.NotEligibleToImport);
    io.mapOptional("Live", summary.Live);
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
    io.mapOptional("GUID", summary.GUID);
    io.mapOptional("Kind", summary.Kind);
    io.mapOptional("Linkage", summary.Linkage);
    io.mapOptional("NotEligibleToImport", summary.NotEligibleToImport);
    io.mapOptional("Live", summary.Live);
  }
};

template <> struct MappingTraits<AliasSummaryYaml> {
  static void mapping(IO &io, AliasSummaryYaml &summary) {
    io.mapOptional("GUID", summary.GUID);
    io.mapOptional("Kind", summary.Kind);
    io.mapOptional("Linkage", summary.Linkage);
    io.mapOptional("NotEligibleToImport", summary.NotEligibleToImport);
    io.mapOptional("Live", summary.Live);
    io.mapOptional("Aliasee", summary.Aliasee);
  }
};

} // End yaml namespace
} // End llvm namespace

LLVM_YAML_IS_STRING_MAP(TypeIdSummary)
LLVM_YAML_IS_SEQUENCE_VECTOR(FunctionSummaryYaml)
LLVM_YAML_IS_SEQUENCE_VECTOR(CalleeInfoYaml)
LLVM_YAML_IS_SEQUENCE_VECTOR(GlobalVarSummaryYaml)
LLVM_YAML_IS_SEQUENCE_VECTOR(AliasSummaryYaml)
LLVM_YAML_IS_SEQUENCE_VECTOR(std::string)

namespace llvm {
namespace yaml {

struct NGVMValue {
  ModuleSummaryIndex *Index;
  GlobalValue::GUID GUID;
  GlobalValueSummaryInfo *Summary;
};
using NamedGlobalValueMap = std::map<std::string, struct NGVMValue>;

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
template <> struct CustomMappingTraits<NamedGlobalValueMap> {
  static void inputOne(IO &io, StringRef Key, NamedGlobalValueMap &V) {
    std::vector<FunctionSummaryYaml> FSums;
    std::vector<GlobalVarSummaryYaml> GVSums;
    std::vector<AliasSummaryYaml> ASums;
    io.mapRequired(Key.str().c_str(), FSums);
    io.mapRequired(Key.str().c_str(), GVSums);
    io.mapRequired(Key.str().c_str(), ASums);
    auto &Elem = V[Key].Summary;
    for (auto &FSum : FSums) {
      Elem->SummaryList.push_back(llvm::make_unique<FunctionSummary>(
          GlobalValueSummary::GVFlags(
              static_cast<GlobalValue::LinkageTypes>(FSum.Linkage),
              FSum.NotEligibleToImport, FSum.Live),
          0, ArrayRef<ValueInfo>{}, ArrayRef<FunctionSummary::EdgeTy>{},
          std::move(FSum.TypeTests), std::move(FSum.TypeTestAssumeVCalls),
          std::move(FSum.TypeCheckedLoadVCalls),
          std::move(FSum.TypeTestAssumeConstVCalls),
          std::move(FSum.TypeCheckedLoadConstVCalls)));
    }

    for (auto &GVSum : GVSums) {
      Elem->SummaryList.push_back(llvm::make_unique<GlobalVarSummary>(
          GlobalValueSummary::GVFlags(
              static_cast<GlobalValue::LinkageTypes>(GVSum.Linkage),
              GVSum.NotEligibleToImport, GVSum.Live),
          ArrayRef<ValueInfo>{}));
    }

    for (auto &ASum : ASums) {
      Elem->SummaryList.push_back(llvm::make_unique<AliasSummary>(
          GlobalValueSummary::GVFlags(
              static_cast<GlobalValue::LinkageTypes>(ASum.Linkage),
              ASum.NotEligibleToImport, ASum.Live),
          ArrayRef<ValueInfo>{}));
    }
  }

  static void output(IO &io, NamedGlobalValueMap &V) {
    for (auto &P : V) {
      std::vector<FunctionSummaryYaml> FSums;
      std::vector<GlobalVarSummaryYaml> GVSums;
      std::vector<AliasSummaryYaml> ASums;
      for (auto &Sum : P.second.Summary->SummaryList) {
        if (auto *FSum = dyn_cast<FunctionSummary>(Sum.get())) {
          std::vector<CalleeInfoYaml> Calls;
          for (auto edge : FSum->calls()) {
            Calls.push_back(
                {P.second.Index->findOidToValueName(edge.first.getGUID()),
                 edge.first.getGUID(), edge.second.Hotness});
          }
          FSums.push_back(FunctionSummaryYaml{
              P.second.GUID, FSum->getSummaryKind(),
              static_cast<GlobalValue::LinkageTypes>(FSum->flags().Linkage),
              static_cast<bool>(FSum->flags().NotEligibleToImport),
              static_cast<bool>(FSum->flags().Live),

              FSum->instCount(), Calls, FSum->type_tests(),
              FSum->type_test_assume_vcalls(), FSum->type_checked_load_vcalls(),
              FSum->type_test_assume_const_vcalls(),
              FSum->type_checked_load_const_vcalls()});
        }
      }
      for (auto &Sum : P.second.Summary->SummaryList) {
        if (auto *GVSum = dyn_cast<GlobalVarSummary>(Sum.get()))
          GVSums.push_back(GlobalVarSummaryYaml{
              P.second.GUID, GVSum->getSummaryKind(),
              static_cast<GlobalValue::LinkageTypes>(GVSum->flags().Linkage),
              static_cast<bool>(GVSum->flags().NotEligibleToImport),
              static_cast<bool>(GVSum->flags().Live)});
      }
      for (auto &Sum : P.second.Summary->SummaryList) {
        if (auto *ASum = dyn_cast<AliasSummary>(Sum.get()))
          ASums.push_back(AliasSummaryYaml{
              P.second.GUID, ASum->getSummaryKind(),
              static_cast<GlobalValue::LinkageTypes>(ASum->flags().Linkage),
              static_cast<bool>(ASum->flags().NotEligibleToImport),
              static_cast<bool>(ASum->flags().Live),
              P.second.Index->findOidToValueName(
                  ASum->getAliasee().getOriginalName())});
      }
      if (!FSums.empty())
        io.mapRequired(P.first.c_str(), FSums);
      if (!GVSums.empty())
        io.mapRequired(P.first.c_str(), GVSums);
      if (!ASums.empty())
        io.mapRequired(P.first.c_str(), ASums);
    }
  }
};

template <> struct MappingTraits<ModuleSummaryIndex> {
  static void mapping(IO &io, ModuleSummaryIndex& index) {
    NamedGlobalValueMap M;
    for (gvsummary_iterator i = index.GlobalValueMap.begin();
         i != index.GlobalValueMap.end(); i++) {
      std::string name = index.findOidToValueName(i->first);
      M.insert(std::pair<std::string, struct NGVMValue>(
          name, NGVMValue{&index, i->first, &i->second}));
    }
    io.mapOptional("NamedGlobalValueMap", M);
    io.mapOptional("TypeIdMap", index.TypeIdMap);
    io.mapOptional("WithGlobalValueDeadStripping",
                   index.WithGlobalValueDeadStripping);
  }
};

} // End yaml namespace
} // End llvm namespace

#endif
