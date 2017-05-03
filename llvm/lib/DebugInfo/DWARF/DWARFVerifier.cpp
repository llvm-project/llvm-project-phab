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

namespace {
/// Returns true if the two ranges [a.first, a.second) and [b.first, b.second)
/// intersect.
bool RangesIntersect(const std::pair<uint64_t, uint64_t> &a,
                     const std::pair<uint64_t, uint64_t> &b) {
  if (a.first == a.second || b.first == b.second)
    return false; // Empty ranges can't intersect.
  return (a.first < b.second) && (a.second > b.first);
}

} // anonymous namespace

bool DWARFVerifier::RangeInfoType::CheckForRangeErrors(
    raw_ostream &OS, DWARFAddressRangesVector &Ranges) {
  bool HasErrors = false;
  size_t I = 0;
  while (I < Ranges.size()) {
    if (Ranges[I].first >= Ranges[I].second) {
      if (Ranges[I].first > Ranges[I].second) {
        OS << "error: invalid range in DIE\n";
        HasErrors = true;
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

void DWARFVerifier::RangeInfoType::Dump(raw_ostream &OS) const {
  OS << format("0x%08" PRIx32, Die.getOffset()) << ":";
  for (const auto &R : Ranges)
    OS << " [" << format("0x%" PRIx32, R.first) << "-"
       << format("0x%" PRIx32, R.second) << ")";
  OS << "\n";
}

bool DWARFVerifier::RangeInfoType::Contains(const RangeInfoType &RHS) {
  for (const auto &S : RHS.Ranges)
    for (const auto &R : Ranges)
      if (R.first <= S.first && S.first < R.second)
        return R.first < S.second && S.second <= R.second;
  return false;
}

bool DWARFVerifier::RangeInfoType::DoesIntersect(
    const RangeInfoType &RHS) const {
  for (const auto &R : RHS.Ranges)
    for (const auto &L : Ranges)
      if (RangesIntersect(R, L))
        return true;
  return false;
}

bool DWARFVerifier::RangeInfoType::operator<(const RangeInfoType &RHS) const {
  if (Ranges.empty()) {
    if (RHS.Ranges.empty())
      return Die.getOffset() < RHS.Die.getOffset();
    else
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

void DWARFVerifier::verifyDebugInfoTag(DWARFDie &Die) {
  const auto Tag = Die.getTag();
  const uint32_t Depth = Die.getDebugInfoEntry()->getDepth();
  if (Depth >= DieRangeInfos.size())
    DieRangeInfos.resize(Depth + 1);
  DieRangeInfos[Depth].Die = Die;
  DieRangeInfos[Depth].Ranges.clear();
  switch (Tag) {
  case DW_TAG_compile_unit: {
    DieRangeInfos[Depth].Ranges = Die.getAddressRanges();
    if (RangeInfoType::CheckForRangeErrors(OS, DieRangeInfos[Depth].Ranges)) {
      ++NumDebugInfoErrors;
      Die.dump(OS, 0);
      OS << "\n";
    }
  } break;
  case DW_TAG_subprogram:
  case DW_TAG_lexical_block:
  case DW_TAG_inlined_subroutine: {
    DieRangeInfos[Depth].Ranges = Die.getAddressRanges();
    if (RangeInfoType::CheckForRangeErrors(OS, DieRangeInfos[Depth].Ranges)) {
      ++NumDebugInfoErrors;
      Die.dump(OS, 0);
      OS << "\n";
    }
    if (DieRangeInfos[Depth].Ranges.empty())
      break;
    if (Tag == DW_TAG_subprogram) {
      // We need to keep track of all function ranges for
      // .debug_aranges and .debug_info verifiers
      AllFunctionRangeInfos.insert(DieRangeInfos[Depth]);

      // Check the if the compile unit has valid ranges
      if (!DieRangeInfos[0].Ranges.empty()) {
        if (!DieRangeInfos[0].Contains(DieRangeInfos[Depth])) {
          ++NumDebugInfoErrors;
          OS << "error: CU DIE has child with address ranges "
                "that are not contained in its ranges:\n";
          DieRangeInfos[0].Die.dump(OS, 0);
          Die.dump(OS, 0);
          OS << "\n";
        }
      }
    } else {
      if (!DieRangeInfos[Depth - 1].Contains(DieRangeInfos[Depth])) {
        ++NumDebugInfoErrors;
        OS << "error: DIE has a child " << TagString(Tag)
           << " DIE whose address ranges that are not contained "
              "in its ranges:\n";
        DieRangeInfos[Depth - 1].Die.dump(OS, 0);
        Die.dump(OS, 0);
      }
    }
  } break;
  default:
    break;
  }
}
void DWARFVerifier::verifyDebugInfoAttribute(DWARFDie &Die,
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

void DWARFVerifier::verifyDebugInfoForm(DWARFDie &Die,
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

void DWARFVerifier::verifyDebugInfoOverlappingFunctionRanges() {
  // Now go through all function address ranges and check for any that
  // overlap. All function range infos were added to a std::set that we can
  // traverse in order and do the checking for overlaps.
  OS << "Verifying .debug_info function address ranges for overlap...\n";

  auto End = AllFunctionRangeInfos.end();
  auto Prev = End;
  for (auto Curr = AllFunctionRangeInfos.begin(); Curr != End; ++Curr) {
    if (Prev != End) {
      if (Prev->DoesIntersect(*Curr)) {
        ++NumDebugInfoErrors;
        OS << "error: two function DIEs have overlapping address ranges:\n";
        Prev->Die.dump(OS, 0);
        Curr->Die.dump(OS, 0);
        OS << "\n";
      }
    }
    Prev = Curr;
  }
}

bool DWARFVerifier::handleDebugInfo() {
  NumDebugInfoErrors = 0;
  OS << "Verifying .debug_info...\n";
  for (const auto &CU : DCtx.compile_units()) {
    unsigned NumDies = CU->getNumDIEs();
    for (unsigned I = 0; I < NumDies; ++I) {
      auto Die = CU->getDIEAtIndex(I);
      const auto Tag = Die.getTag();
      if (Tag == DW_TAG_null)
        continue;
      verifyDebugInfoTag(Die);
      for (auto AttrValue : Die.attributes()) {
        verifyDebugInfoAttribute(Die, AttrValue);
        verifyDebugInfoForm(Die, AttrValue);
      }
    }
  }
  verifyDebugInfoReferences();
  verifyDebugInfoOverlappingFunctionRanges();
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
