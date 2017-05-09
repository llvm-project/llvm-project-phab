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

bool DWARFVerifier::SortedRanges::contains(const SortedRanges &RHS) const {
  return DWARFRangesContains(Ranges, RHS.Ranges);
}

bool DWARFVerifier::SortedRanges::doesIntersect(const SortedRanges &RHS) const {
  return AnyDWARFRangesIntersect(Ranges, RHS.Ranges);
}

void DWARFVerifier::SortedRanges::dump(raw_ostream &OS) const {
  for (const auto &R : Ranges)
    OS << " [" << format("0x%" PRIx64, R.first) << "-"
       << format("0x%" PRIx64, R.second) << ")";
}

bool DWARFVerifier::SortedRanges::operator<(const SortedRanges &RHS) const {
  if (RHS.Ranges.empty())
    return false;
  if (Ranges.empty())
    return true;
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

uint32_t DWARFVerifier::SortedRanges::removeInvalidRanges(
    DWARFAddressRangesVector &Ranges) {
  uint32_t NumInvalidRanges = 0;
  size_t I = 0;
  while (I < Ranges.size()) {
    if (Ranges[I].first >= Ranges[I].second) {
      if (Ranges[I].first > Ranges[I].second)
        ++NumInvalidRanges;
      // Remove empty or invalid ranges.
      Ranges.erase(Ranges.begin() + I);
    } else {
      ++I; // Only increment if we didn't remove the range.
    }
  }
  return NumInvalidRanges;
}

bool DWARFVerifier::SortedRanges::insert(
    const DWARFAddressRangesVector &UnsortedRanges) {
  bool Error = false;
  for (const auto &R : UnsortedRanges) {
    if (R.first >= R.second)
      continue;
    auto Begin = Ranges.begin();
    auto End = Ranges.end();
    auto InsertPos = std::lower_bound(Begin, End, R);
    if (!Error) {
      bool RangeOverlaps = false;
      if (InsertPos != End) {
        if (DWARFRangesIntersect(*InsertPos, R))
          RangeOverlaps = true;
        else if (InsertPos != Begin) {
          auto Iter = InsertPos - 1;
          if (DWARFRangesIntersect(*Iter, R))
            RangeOverlaps = true;
        }
        if (RangeOverlaps) {
          Error = true;
        }
      }
      // Insert the range into our sorted list
      Ranges.insert(InsertPos, R);
    }
  }
  return Error;
}

bool DWARFVerifier::DieRangeInfo::setDieAndReportRangeErrors(raw_ostream &OS,
                                                             DWARFDie D) {
  Die = D;
  auto UnsortedRanges = Die.getAddressRanges();
  bool HasErrors = false;
  if (SortedRanges::removeInvalidRanges(UnsortedRanges)) {
    HasErrors = true;
    OS << "error: invalid range in DIE:\n";
  }
  if (Ranges.insert(UnsortedRanges)) {
    OS << "error: ranges in DIE overlap:\n";
    HasErrors = true;
  }
  if (HasErrors) {
    Die.dump(OS, 0);
    OS << "\n";
  }
  return HasErrors;
}

void DWARFVerifier::DieRangeInfo::dump(raw_ostream &OS) const {
  OS << format("0x%08" PRIx32, Die.getOffset()) << ":";
  Ranges.dump(OS);
  OS << "\n";
}

void DWARFVerifier::verifyNoRangesOverlap(
    const std::set<DieRangeInfo> &DieRangeInfos) {
  auto End = DieRangeInfos.end();
  auto Prev = End;
  for (auto Curr = DieRangeInfos.begin(); Curr != End; ++Curr) {
    if (Prev != End) {
      if (Prev->doesIntersect(*Curr)) {
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

bool DWARFVerifier::DieRangeInfo::contains(const DieRangeInfo &RHS) const {
  return Ranges.contains(RHS.Ranges);
}

bool DWARFVerifier::DieRangeInfo::doesIntersect(const DieRangeInfo &RHS) const {
  return Ranges.doesIntersect(RHS.Ranges);
}

bool DWARFVerifier::DieRangeInfo::operator<(const DieRangeInfo &RHS) const {
  return Ranges < RHS.Ranges;
}

bool DWARFVerifier::DieRangeInfo::reportErrorIfNotContained(
    raw_ostream &OS, const DieRangeInfo &RI, const char *Error) const {
  if (!contains(RI)) {
    OS << Error;
    Die.dump(OS, 0);
    RI.Die.dump(OS, 0);
    OS << "\n";
    return true;
  }
  return false;
}

const DWARFVerifier::DieRangeInfo *
DWARFVerifier::NonOverlappingRanges::GetOverlappingRangeInfo(
    const DieRangeInfo &RI) const {
  if (!RangeSet.empty()) {
    auto Iter = RangeSet.lower_bound(RI);
    if (Iter != RangeSet.end()) {
      if (Iter->doesIntersect(RI))
        return &*Iter;
    }
    if (Iter != RangeSet.begin()) {
      --Iter;
      if (Iter->doesIntersect(RI))
        return &*Iter;
    }
  }
  return nullptr;
}

bool DWARFVerifier::NonOverlappingRanges::insertAndReportErrors(
    raw_ostream &OS, const DieRangeInfo &RI) {
  bool Error = false;
  auto OverlappingRI = GetOverlappingRangeInfo(RI);
  if (OverlappingRI) {
    OS << "error: DIEs have overlapping address ranges:\n";
    OverlappingRI->Die.dump(OS, 0);
    RI.Die.dump(OS, 0);
    OS << "\n";
    Error = true;
  }
  RangeSet.insert(RI);
  return Error;
}

void DWARFVerifier::verifyDie(const DWARFDie &Die, const DieRangeInfo &ParentRI,
                              NonOverlappingRanges &NOR) {
  const auto Tag = Die.getTag();

  DieRangeInfo RI;
  switch (Tag) {
  case DW_TAG_null:
    return;
  case DW_TAG_compile_unit:
    if (UnitRI.setDieAndReportRangeErrors(OS, Die))
      ++NumDebugInfoErrors;
    break;
  case DW_TAG_subprogram: {
    if (RI.setDieAndReportRangeErrors(OS, Die))
      ++NumDebugInfoErrors;
    if (RI.Ranges.empty())
      break;

    // If the compile unit has address range(s), it must contain DIE's ranges.
    const char *Error = "error: CU DIE has child with address ranges that are "
                        "not contained in its ranges:\n";
    if (!UnitRI.Ranges.empty() &&
        UnitRI.reportErrorIfNotContained(OS, RI, Error)) {
      ++NumDebugInfoErrors;
    }

    // Check if this function's range overlaps with any previous function ranges
    // from any compile unit and make an error if needed.
    if (AllFunctionRanges.insertAndReportErrors(OS, RI))
      ++NumDebugInfoErrors;

  } break;
  case DW_TAG_lexical_block:
  case DW_TAG_inlined_subroutine: {
    if (RI.setDieAndReportRangeErrors(OS, Die)) {
      ++NumDebugInfoErrors;
      Die.dump(OS, 0);
      OS << "\n";
    }
    if (RI.Ranges.empty())
      break;
    // Check to make sure our parent contains all ranges for this Die.
    const char *Error = "error: DIE has a child DIE whose address ranges are "
                        "not contained in its ranges:\n";
    if (ParentRI.reportErrorIfNotContained(OS, RI, Error))
      ++NumDebugInfoErrors;

    // Check if this range overlaps with any previous sibling ranges and make an
    // error if needed.
    if (NOR.insertAndReportErrors(OS, RI))
      ++NumDebugInfoErrors;
  } break;
  default:
    RI.Die = Die;
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
