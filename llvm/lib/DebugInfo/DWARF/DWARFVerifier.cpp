//===- DWARFVerifier.cpp --------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/DebugInfo/DWARF/DWARFVerifier.h"
#include "llvm/ADT/STLExtras.h"
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

bool DWARFVerifier::DieRangeInfo::insert(
    const DWARFAddressRangesVector &UnsortedRanges) {
  bool Error = false;
  for (const auto &UR : UnsortedRanges) {
    DWARFRange R(UR.first, UR.second);
    assert(R.Start < R.End);
    auto Begin = Ranges.begin();
    auto End = Ranges.end();
    auto InsertPos = std::lower_bound(Begin, End, R);
    if (!Error) {
      bool RangeOverlaps = false;
      if (InsertPos != End) {
        if (InsertPos->intersects(R))
          RangeOverlaps = true;
        else if (InsertPos != Begin) {
          auto Iter = InsertPos - 1;
          if (Iter->intersects(R))
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

bool DWARFVerifier::DieRangeInfo::extractRangesAndReportErrors(
    raw_ostream &OS) {
  Ranges.clear();
  auto UnsortedRanges = Die.getAddressRanges();
  bool HasErrors = false;

  auto ErasePos =
      remove_if(UnsortedRanges, [&](const std::pair<uint64_t, uint64_t> &R) {
        if (R.first < R.second)
          return false;
        if (R.first > R.second)
          HasErrors = true;
        return true;
      });
  UnsortedRanges.erase(ErasePos, UnsortedRanges.end());

  if (HasErrors)
    OS << "error: invalid range in DIE:\n";

  if (insert(UnsortedRanges)) {
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
  for (const auto &R : Ranges)
    OS << " [" << format("0x%" PRIx64, R.Start) << "-"
       << format("0x%" PRIx64, R.End) << ")";
  OS << "\n";
}

bool DWARFVerifier::DieRangeInfo::contains(const DieRangeInfo &RHS) const {
  // Both list of ranges are sorted so we can make this fasts
  auto Begin = Ranges.begin();
  auto End = Ranges.end();
  if (Ranges.empty() || RHS.Ranges.empty())
    return false;
  // Since the ranges are sorted we can advance where we start searching with
  // this object's ranges as we traverse RHS.Ranges.
  
  // First quickly find the first range that could contain the first range in
  // RHS using a binary search.
  auto Iter = std::upper_bound(Begin, End, RHS.Ranges.front());
  if (Iter != Begin)
    --Iter;
  // Now lineary walk the ranges in this object and see if they contain each
  // ranges from RHS.Ranges.
  for (const auto &R : RHS.Ranges) {
    while (Iter != End) {
      if (Iter->contains(R))
        break;
      ++Iter;
    }
    if (Iter == End)
      return false;
  }
  return true;
}

bool DWARFVerifier::DieRangeInfo::intersects(const DieRangeInfo &RHS) const {
  auto Begin = Ranges.begin();
  auto End = Ranges.end();
  if (Ranges.empty() || RHS.Ranges.empty())
    return false;

  // First quickly find the first range that could contain the first range in
  // RHS using a binary search.
  auto Iter = std::upper_bound(Begin, End, RHS.Ranges.front());
  if (Iter != Begin)
    --Iter;
  for (const auto &R : RHS.Ranges) {
    if (R.End <= Iter->Start)
      continue;
    while (Iter != End) {
      if (Iter->intersects(R))
        return true;
      ++Iter;
    }
  }
  return false;
}

bool DWARFVerifier::DieRangeInfo::reportErrorIfNotContained(
    raw_ostream &OS, const DieRangeInfo &RI, const char *Error) const {
  if (contains(RI))
    return false;
  OS << Error;
  Die.dump(OS, 0);
  RI.Die.dump(OS, 0);
  OS << "\n";
  return true;
}

bool DWARFVerifier::DieInfo::addContainedDieAndReportErrors(
    raw_ostream &OS, DieRangeInfo &DieRI) {
  bool HasErrors = DieRI.extractRangesAndReportErrors(OS);

  // If this DIE has not valid ranges to check then we are done.
  if (DieRI.Ranges.empty())
    return HasErrors;

  // Die has address ranges so we need to check that its address ranges are
  // contained in this DieInfo's address ranges except if this is a compile
  // unit that doesn't have address range(s).
  bool IsCompileUnit = RI.Die.getTag() == DW_TAG_compile_unit;
  bool CheckContains = !IsCompileUnit || !RI.Ranges.empty();

  const char *Error = "error: DIE has a child DIE whose address ranges are not "
                      "contained in its ranges:";
  if (CheckContains && RI.reportErrorIfNotContained(OS, DieRI, Error))
    HasErrors = true;

  // Check if this range overlaps with any previous sibling ranges and make an
  // error if needed.
  if (NOR.insertAndReportErrors(OS, DieRI))
    HasErrors = true;
  return HasErrors;
}

Optional<DWARFDie> DWARFVerifier::NonOverlappingRanges::GetOverlappingRangeInfo(
    const DieRangeInfo &RI) const {
  if (RangeSet.empty())
    return None;
  auto Iter = RangeSet.lower_bound(RI);
  // Since we are searching using lower_bound, Iter might contain the address
  // range that follows RI, so we need to check it in case RI overlaps with the
  // next range.
  if (Iter != RangeSet.end() && Iter->intersects(RI))
    return Iter->Die;
  // We also need to check the previous range to ensure it doesn't overlap with
  // this range.
  if (Iter != RangeSet.begin() && (--Iter)->intersects(RI))
    return Iter->Die;
  return None;
}

bool DWARFVerifier::NonOverlappingRanges::insertAndReportErrors(
    raw_ostream &OS, const DieRangeInfo &RI) {
  bool Error = false;
  if (auto OverlappingDie = GetOverlappingRangeInfo(RI)) {
    OS << "error: DIEs have overlapping address ranges:\n";
    OverlappingDie->dump(OS, 0);
    RI.Die.dump(OS, 0);
    OS << "\n";
    Error = true;
  }
  RangeSet.insert(RI);
  return Error;
}

void DWARFVerifier::verifyDie(const DWARFDie &Die, DieInfo &ParentDI) {
  DieInfo DI;
  DI.RI.Die = Die;
  switch (Die.getTag()) {
  case DW_TAG_null:
    return;
  case DW_TAG_compile_unit:
    // Set the Unit DIE info.
    UnitDI.RI.Die = Die;
    if (UnitDI.RI.extractRangesAndReportErrors(OS))
      ++NumDebugInfoErrors;
    break;
  case DW_TAG_subprogram:
    // Add the subprogram to the unit DIE and make it is contained.
    if (UnitDI.addContainedDieAndReportErrors(OS, DI.RI))
      ++NumDebugInfoErrors;
    break;
  case DW_TAG_lexical_block:
  case DW_TAG_inlined_subroutine:
    if (ParentDI.addContainedDieAndReportErrors(OS, DI.RI))
      ++NumDebugInfoErrors;
    break;
  default:
    break;
  }
  // Verify the attributes and forms.
  for (auto AttrValue : Die.attributes()) {
    verifyDebugInfoAttribute(Die, AttrValue);
    verifyDebugInfoForm(Die, AttrValue);
  }

  for (DWARFDie Child : Die)
    verifyDie(Child, DI);
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
    DieInfo DI;
    DWARFDie Die = CU->getUnitDIE(/* ExtractUnitDIEOnly = */ false);
    verifyDie(Die, DI);
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
