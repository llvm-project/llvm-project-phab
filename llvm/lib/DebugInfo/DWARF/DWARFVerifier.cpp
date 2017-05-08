//===- DWARFVerifier.cpp --------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/DebugInfo/DWARF/DWARFVerifier.h"
#include "llvm/DebugInfo/DWARF/DWARFCompileUnit.h"
#include "llvm/DebugInfo/DWARF/DWARFContext.h"
#include "llvm/DebugInfo/DWARF/DWARFDebugLine.h"
#include "llvm/DebugInfo/DWARF/DWARFDie.h"
#include "llvm/DebugInfo/DWARF/DWARFFormValue.h"
#include "llvm/DebugInfo/DWARF/DWARFSection.h"
#include "llvm/Support/raw_ostream.h"
#include <map>
#include <set>
#include <vector>

using namespace llvm;
using namespace dwarf;
using namespace object;

bool DWARFVerifier::DieRangeInfo::SortAndCheckRangesHasErrors(raw_ostream &OS) {
  bool HasErrors = false;
  size_t I = 0;
  while (I < Ranges.size()) {
    if (Ranges[I].first >= Ranges[I].second) {
      if (Ranges[I].first > Ranges[I].second) {
        if (!HasErrors) {
          HasErrors = true;
          OS << "error: invalid range in DIE:\n";
        }
      }
      // Remove empty or invalid ranges.
      Ranges.erase(Ranges.begin() + I);
      continue;
    } else {
      ++I; // Only increment if we didn't remove the range.
    }
  }
  std::sort(Ranges.begin(), Ranges.end());
  return HasErrors;
}

void DWARFVerifier::DieRangeInfo::Dump(raw_ostream &OS) const {
  OS << format("0x%08" PRIx32, Die.getOffset()) << ":";
  for (const auto &R : Ranges)
    OS << " [" << format("0x%" PRIx32, R.first) << "-"
       << format("0x%" PRIx32, R.second) << ")";
  OS << "\n";
}

void DWARFVerifier::verifyNoRangesOverlap(
    const std::set<DieRangeInfo> &DieRangeInfos) {
  auto End = DieRangeInfos.end();
  auto Prev = End;
  for (auto Curr = DieRangeInfos.begin(); Curr != End; ++Curr) {
    if (Prev != End) {
      if (Prev->DoesIntersect(*Curr)) {
        ++NumDebugInfoErrors;
        OS << "error: DIEs have overlapping address ranges:\n";
        Prev->Die.dump(OS, 0);
        Curr->Die.dump(OS, 0);
        OS << "\n";
      }
    }
    Prev = Curr;
  }
}

bool DWARFVerifier::DieRangeInfo::Contains(const DieRangeInfo &RHS) const {
  return DWARFRangesContains(Ranges, RHS.Ranges);
}

bool DWARFVerifier::DieRangeInfo::DoesIntersect(const DieRangeInfo &RHS) const {
  return AnyDWARFRangesIntersect(Ranges, RHS.Ranges);
}

bool DWARFVerifier::DieRangeInfo::operator<(const DieRangeInfo &RHS) const {
  if (Ranges.empty()) {
    if (RHS.Ranges.empty())
      return Die.getOffset() < RHS.Die.getOffset();
    return true;
  }
  if (RHS.Ranges.empty())
    return false;
  size_t LeftSize = Ranges.size();
  size_t RightSize = RHS.Ranges.size();
  for (size_t I = 0; I < LeftSize; ++I) {
    if (I >= RightSize)
      return false;
    if (Ranges[I].first != RHS.Ranges[I].first)
      return Ranges[I].first < RHS.Ranges[I].first;
    if (Ranges[I].second != RHS.Ranges[I].second)
      return Ranges[I].second < RHS.Ranges[I].second;
  }
  return false;
}

const DWARFVerifier::DieRangeInfo *
DWARFVerifier::NonOverlappingRanges::GetOverlappingRangeInfo(
    const DieRangeInfo &RI) const {
  if (!NonOverlappingRanges.empty()) {
    auto Iter = NonOverlappingRanges.lower_bound(RI);
    if (Iter != NonOverlappingRanges.end()) {
      if (AnyDWARFRangesIntersect(Iter->Ranges, RI.Ranges))
        return &*Iter;
    }
    if (Iter != NonOverlappingRanges.begin()) {
      --Iter;
      if (AnyDWARFRangesIntersect(Iter->Ranges, RI.Ranges))
        return &*Iter;
    }
  }
  return nullptr;
}
void DWARFVerifier::verifyDie(const DWARFDie &Die, const DieRangeInfo &ParentRI,
                              NonOverlappingRanges &NOR) {
  const auto Tag = Die.getTag();

  DieRangeInfo RI;
  RI.Die = Die;
  switch (Tag) {
  case DW_TAG_null:
    return;
  case DW_TAG_compile_unit: {
    UnitRI.Die = Die;
    UnitRI.Ranges = Die.getAddressRanges();
    if (UnitRI.SortAndCheckRangesHasErrors(OS)) {
      ++NumDebugInfoErrors;
      Die.dump(OS, 0);
      OS << "\n";
    }
  } break;
  case DW_TAG_subprogram: {
    RI.Ranges = Die.getAddressRanges();
    if (RI.SortAndCheckRangesHasErrors(OS)) {
      ++NumDebugInfoErrors;
      Die.dump(OS, 0);
      OS << "\n";
    }
    if (RI.Ranges.empty())
      break;

    // If the compile unit has address ranges, make sure it contains this DIE's
    // ranges.
    if (!UnitRI.Ranges.empty() && !UnitRI.Contains(RI)) {
      ++NumDebugInfoErrors;
      OS << "error: CU DIE has child with address ranges "
            "that are not contained in its ranges:\n";
      UnitRI.Die.dump(OS, 0);
      Die.dump(OS, 0);
      OS << "\n";
    }

    // Check if this function's range overlaps with any previous function ranges
    // from any compile unit and make an error if needed.
    auto OverlappingRI = AllFunctionRanges.GetOverlappingRangeInfo(RI);
    if (OverlappingRI) {
      ++NumDebugInfoErrors;
      OS << "error: DIEs have overlapping address ranges:\n";
      OverlappingRI->Die.dump(OS, 0);
      Die.dump(OS, 0);
      OS << "\n";
    }
    // Add the function to the compile unit's verify info
    AllFunctionRanges.NonOverlappingRanges.insert(RI);

  } break;
  case DW_TAG_lexical_block:
  case DW_TAG_inlined_subroutine: {
    RI.Ranges = Die.getAddressRanges();
    if (RI.SortAndCheckRangesHasErrors(OS)) {
      ++NumDebugInfoErrors;
      Die.dump(OS, 0);
      OS << "\n";
    }
    if (RI.Ranges.empty())
      break;
    // Check to make sure our parent contains all ranges for this Die.
    if (!ParentRI.Contains(RI)) {
      ++NumDebugInfoErrors;
      OS << "error: DIE has a child " << TagString(Tag)
         << " DIE whose address ranges that are not contained "
            "in its ranges:\n";
      ParentRI.Die.dump(OS, 0);
      Die.dump(OS, 0);
    }

    // Check if this range overlaps with any previous sibling ranges and make an
    // error if needed.
    auto OverlappingRI = NOR.GetOverlappingRangeInfo(RI);
    if (OverlappingRI) {
      ++NumDebugInfoErrors;
      OS << "error: DIEs have overlapping address ranges:\n";
      OverlappingRI->Die.dump(OS, 0);
      Die.dump(OS, 0);
      OS << "\n";
    }
    NOR.NonOverlappingRanges.insert(RI);

  } break;
  default:
    break;
  }
  // Verify the attributes and forms.
  for (auto AttrValue : Die.attributes()) {
    verifyDebugInfoAttribute(Die, AttrValue);
    verifyDebugInfoForm(Die, AttrValue);
  }

  NonOverlappingRanges DieNRO;
  for (DWARFDie Child : Die)
    verifyDie(Child, RI, DieNRO);
}
void DWARFVerifier::verifyDebugInfoAttribute(const DWARFDie &Die,
                                             DWARFAttribute &AttrValue) {
  const auto Attr = AttrValue.Attr;
  switch (Attr) {
  case DW_AT_ranges:
    // Make sure the offset in the DW_AT_ranges attribute is valid.
    if (auto SectionOffset = AttrValue.Value.getAsSectionOffset()) {
      if (*SectionOffset >= DCtx.getRangeSection().Data.size()) {
        ++NumDebugInfoErrors;
        OS << "error: DW_AT_ranges offset is beyond .debug_ranges "
              "bounds:\n";
        Die.dump(OS, 0);
        OS << "\n";
      }
    } else {
      ++NumDebugInfoErrors;
      OS << "error: DIE has invalid DW_AT_ranges encoding:\n";
      Die.dump(OS, 0);
      OS << "\n";
    }
    break;
  case DW_AT_stmt_list:
    // Make sure the offset in the DW_AT_stmt_list attribute is valid.
    if (auto SectionOffset = AttrValue.Value.getAsSectionOffset()) {
      if (*SectionOffset >= DCtx.getLineSection().Data.size()) {
        ++NumDebugInfoErrors;
        OS << "error: DW_AT_stmt_list offset is beyond .debug_line "
              "bounds: "
           << format("0x%08" PRIx32, *SectionOffset) << "\n";
        Die.dump(OS, 0);
        OS << "\n";
      }
    } else {
      ++NumDebugInfoErrors;
      OS << "error: DIE has invalid DW_AT_stmt_list encoding:\n";
      Die.dump(OS, 0);
      OS << "\n";
    }
    break;

  default:
    break;
  }
}

void DWARFVerifier::verifyDebugInfoForm(const DWARFDie &Die,
                                        DWARFAttribute &AttrValue) {
  const auto Form = AttrValue.Value.getForm();
  switch (Form) {
  case DW_FORM_ref1:
  case DW_FORM_ref2:
  case DW_FORM_ref4:
  case DW_FORM_ref8:
  case DW_FORM_ref_udata: {
    // Verify all CU relative references are valid CU offsets.
    Optional<uint64_t> RefVal = AttrValue.Value.getAsReference();
    assert(RefVal);
    if (RefVal) {
      auto DieCU = Die.getDwarfUnit();
      auto CUSize = DieCU->getNextUnitOffset() - DieCU->getOffset();
      auto CUOffset = AttrValue.Value.getRawUValue();
      if (CUOffset >= CUSize) {
        ++NumDebugInfoErrors;
        OS << "error: " << FormEncodingString(Form) << " CU offset "
           << format("0x%08" PRIx32, CUOffset)
           << " is invalid (must be less than CU size of "
           << format("0x%08" PRIx32, CUSize) << "):\n";
        Die.dump(OS, 0);
        OS << "\n";
      } else {
        // Valid reference, but we will verify it points to an actual
        // DIE later.
        ReferenceToDIEOffsets[*RefVal].insert(Die.getOffset());
      }
    }
    break;
  }
  case DW_FORM_ref_addr: {
    // Verify all absolute DIE references have valid offsets in the
    // .debug_info section.
    Optional<uint64_t> RefVal = AttrValue.Value.getAsReference();
    assert(RefVal);
    if (RefVal) {
      if (*RefVal >= DCtx.getInfoSection().Data.size()) {
        ++NumDebugInfoErrors;
        OS << "error: DW_FORM_ref_addr offset beyond .debug_info "
              "bounds:\n";
        Die.dump(OS, 0);
        OS << "\n";
      } else {
        // Valid reference, but we will verify it points to an actual
        // DIE later.
        ReferenceToDIEOffsets[*RefVal].insert(Die.getOffset());
      }
    }
    break;
  }
  case DW_FORM_strp: {
    auto SecOffset = AttrValue.Value.getAsSectionOffset();
    assert(SecOffset); // DW_FORM_strp is a section offset.
    if (SecOffset && *SecOffset >= DCtx.getStringSection().size()) {
      ++NumDebugInfoErrors;
      OS << "error: DW_FORM_strp offset beyond .debug_str bounds:\n";
      Die.dump(OS, 0);
      OS << "\n";
    }
    break;
  }
  default:
    break;
  }
}

void DWARFVerifier::verifyDebugInfoReferences() {
  // Take all references and make sure they point to an actual DIE by
  // getting the DIE by offset and emitting an error
  OS << "Verifying .debug_info references...\n";
  for (auto Pair : ReferenceToDIEOffsets) {
    auto Die = DCtx.getDIEForOffset(Pair.first);
    if (Die)
      continue;
    ++NumDebugInfoErrors;
    OS << "error: invalid DIE reference " << format("0x%08" PRIx64, Pair.first)
       << ". Offset is in between DIEs:\n";
    for (auto Offset : Pair.second) {
      auto ReferencingDie = DCtx.getDIEForOffset(Offset);
      ReferencingDie.dump(OS, 0);
      OS << "\n";
    }
    OS << "\n";
  }
}

bool DWARFVerifier::handleDebugInfo() {
  NumDebugInfoErrors = 0;
  OS << "Verifying .debug_info...\n";
  for (const auto &CU : DCtx.compile_units()) {
    NonOverlappingRanges NRO;
    verifyDie(CU->getUnitDIE(/* ExtractUnitDIEOnly = */ false), DieRangeInfo(),
              NRO);
  }
  verifyDebugInfoReferences();
  return NumDebugInfoErrors == 0;
}

void DWARFVerifier::verifyDebugLineStmtOffsets() {
  std::map<uint64_t, DWARFDie> StmtListToDie;
  for (const auto &CU : DCtx.compile_units()) {
    auto Die = CU->getUnitDIE();
    // Get the attribute value as a section offset. No need to produce an
    // error here if the encoding isn't correct because we validate this in
    // the .debug_info verifier.
    auto StmtSectionOffset = toSectionOffset(Die.find(DW_AT_stmt_list));
    if (!StmtSectionOffset)
      continue;
    const uint32_t LineTableOffset = *StmtSectionOffset;
    auto LineTable = DCtx.getLineTableForUnit(CU.get());
    if (LineTableOffset < DCtx.getLineSection().Data.size()) {
      if (!LineTable) {
        ++NumDebugLineErrors;
        OS << "error: .debug_line[" << format("0x%08" PRIx32, LineTableOffset)
           << "] was not able to be parsed for CU:\n";
        Die.dump(OS, 0);
        OS << '\n';
        continue;
      }
    } else {
      // Make sure we don't get a valid line table back if the offset is wrong.
      assert(LineTable == nullptr);
      // Skip this line table as it isn't valid. No need to create an error
      // here because we validate this in the .debug_info verifier.
      continue;
    }
    auto Iter = StmtListToDie.find(LineTableOffset);
    if (Iter != StmtListToDie.end()) {
      ++NumDebugLineErrors;
      OS << "error: two compile unit DIEs, "
         << format("0x%08" PRIx32, Iter->second.getOffset()) << " and "
         << format("0x%08" PRIx32, Die.getOffset())
         << ", have the same DW_AT_stmt_list section offset:\n";
      Iter->second.dump(OS, 0);
      Die.dump(OS, 0);
      OS << '\n';
      // Already verified this line table before, no need to do it again.
      continue;
    }
    StmtListToDie[LineTableOffset] = Die;
  }
}

void DWARFVerifier::verifyDebugLineRows() {
  for (const auto &CU : DCtx.compile_units()) {
    auto Die = CU->getUnitDIE();
    auto LineTable = DCtx.getLineTableForUnit(CU.get());
    // If there is no line table we will have created an error in the
    // .debug_info verifier or in verifyDebugLineStmtOffsets().
    if (!LineTable)
      continue;
    uint32_t MaxFileIndex = LineTable->Prologue.FileNames.size();
    uint64_t PrevAddress = 0;
    uint32_t RowIndex = 0;
    for (const auto &Row : LineTable->Rows) {
      if (Row.Address < PrevAddress) {
        ++NumDebugLineErrors;
        OS << "error: .debug_line["
           << format("0x%08" PRIx32,
                     *toSectionOffset(Die.find(DW_AT_stmt_list)))
           << "] row[" << RowIndex
           << "] decreases in address from previous row:\n";

        DWARFDebugLine::Row::dumpTableHeader(OS);
        if (RowIndex > 0)
          LineTable->Rows[RowIndex - 1].dump(OS);
        Row.dump(OS);
        OS << '\n';
      }

      if (Row.File > MaxFileIndex) {
        ++NumDebugLineErrors;
        OS << "error: .debug_line["
           << format("0x%08" PRIx32,
                     *toSectionOffset(Die.find(DW_AT_stmt_list)))
           << "][" << RowIndex << "] has invalid file index " << Row.File
           << " (valid values are [1," << MaxFileIndex << "]):\n";
        DWARFDebugLine::Row::dumpTableHeader(OS);
        Row.dump(OS);
        OS << '\n';
      }
      if (Row.EndSequence)
        PrevAddress = 0;
      else
        PrevAddress = Row.Address;
      ++RowIndex;
    }
  }
}

bool DWARFVerifier::handleDebugLine() {
  NumDebugLineErrors = 0;
  OS << "Verifying .debug_line...\n";
  verifyDebugLineStmtOffsets();
  verifyDebugLineRows();
  return NumDebugLineErrors == 0;
}
