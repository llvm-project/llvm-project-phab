//===- DWARFVerifier.h ----------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_DEBUGINFO_DWARF_DWARFVERIFIER_H
#define LLVM_DEBUGINFO_DWARF_DWARFVERIFIER_H

#include "llvm/DebugInfo/DWARF/DWARFDebugRangeList.h"
#include "llvm/DebugInfo/DWARF/DWARFDie.h"

#include <cstdint>
#include <map>
#include <set>
#include <vector>

namespace llvm {
class raw_ostream;
struct DWARFAttribute;
class DWARFContext;
class DWARFDie;
class DWARFUnit;

/// A class that verifies DWARF debug information given a DWARF Context.
class DWARFVerifier {
  class SortedRanges {
    DWARFAddressRangesVector Ranges;

  public:
    bool contains(const SortedRanges &RHS) const;
    bool doesIntersect(const SortedRanges &RHS) const;
    void dump(raw_ostream &OS) const;
    bool empty() const { return Ranges.empty(); }
    bool operator<(const SortedRanges &RHS) const;

    /// Remove any empty or invalid ranges and return the number if invalid
    /// ranges that were removed (empty ranges are not invalid).
    static uint32_t removeInvalidRanges(DWARFAddressRangesVector &Ranges);
    /// Inserts the unsorted ranges and returns true if errors were found
    /// during insertion
    bool insert(const DWARFAddressRangesVector &UnsortedRanges);
  };

  struct DieRangeInfo {
    DWARFDie Die;
    SortedRanges Ranges;

    DieRangeInfo() : Die(), Ranges() {}
    DieRangeInfo(DWARFDie D, const DWARFAddressRangesVector &R);
    /// Return true if this object contains all ranges within RHS.
    bool contains(const DieRangeInfo &RHS) const;
    bool doesIntersect(const DieRangeInfo &RHS) const;
    void dump(raw_ostream &OS) const;
    bool operator<(const DieRangeInfo &RHS) const;
    /// Return true if there are errors in the ranges R.
    bool setDieAndReportRangeErrors(raw_ostream &OS, DWARFDie Die);
    /// Return true if this object doesn't fully contain the ranges in RI
    /// and report errors to the stream.
    bool reportErrorIfNotContained(raw_ostream &OS, const DieRangeInfo &RI,
                                   const char *Error) const;
  };

  struct NonOverlappingRanges {
    std::set<DieRangeInfo> RangeSet;

    /// Returns true if the sibling ranges
    const DieRangeInfo *GetOverlappingRangeInfo(const DieRangeInfo &RI) const;

    bool insertAndReportErrors(raw_ostream &OS, const DieRangeInfo &RI);
  };

  raw_ostream &OS;
  DWARFContext &DCtx;
  DieRangeInfo UnitRI;
  NonOverlappingRanges AllFunctionRanges;

  /// A map that tracks all references (converted absolute references) so we
  /// can verify each reference points to a valid DIE and not an offset that
  /// lies between to valid DIEs.
  std::map<uint64_t, std::set<uint32_t>> ReferenceToDIEOffsets;

  uint32_t NumDebugInfoErrors;
  uint32_t NumDebugLineErrors;

  /// Verifies the a DIE's tag and gathers information about all DIEs.
  ///
  /// This function currently checks for:
  /// - Checks DW_TAG_compile_unit address range(s)
  /// - Checks DW_TAG_subprogram address range(s) and if the compile unit
  ///   has ranges, verifies that its address range is fully contained in
  ///   the compile unit ranges. Also adds the functions address range info
  ///   to AllFunctionDieRangeInfos to look for functions with overlapping
  ///   ranges after all DIEs have been processed.
  /// - Checks that DW_TAG_lexical_block and DW_TAG_inlined_subroutine DIEs
  ///   have address range(s) that are fully contained in their parent DIEs
  ///   address range(s).
  ///
  /// \param Die          The DWARF DIE to check
  /// \param ParantRI     The parent DIE's range information
  /// \param NOR          The parent DIE's list of ranges that can't overlap.
  void verifyDie(const DWARFDie &Die, const DieRangeInfo &ParentRI,
                 NonOverlappingRanges &NOR);

  /// Verify that no DIE ranges overlap.
  void verifyNoRangesOverlap(const std::set<DieRangeInfo> &DieRangeInfos);

  /// Verifies the attribute's DWARF attribute and its value.
  ///
  /// This function currently checks for:
  /// - DW_AT_ranges values is a valid .debug_ranges offset
  /// - DW_AT_stmt_list is a valid .debug_line offset
  ///
  /// @param Die          The DWARF DIE that owns the attribute value
  /// @param AttrValue    The DWARF attribute value to check
  void verifyDebugInfoAttribute(const DWARFDie &Die, DWARFAttribute &AttrValue);

  /// Verifies the attribute's DWARF form.
  ///
  /// This function currently checks for:
  /// - All DW_FORM_ref values that are CU relative have valid CU offsets
  /// - All DW_FORM_ref_addr values have valid .debug_info offsets
  /// - All DW_FORM_strp values have valid .debug_str offsets
  ///
  /// @param Die          The DWARF DIE that owns the attribute value
  /// @param AttrValue    The DWARF attribute value to check
  void verifyDebugInfoForm(const DWARFDie &Die, DWARFAttribute &AttrValue);

  /// Verifies the all valid references that were found when iterating through
  /// all of the DIE attributes.
  ///
  /// This function will verify that all references point to DIEs whose DIE
  /// offset matches. This helps to ensure if a DWARF link phase moved things
  /// around, that it doesn't create invalid references by failing to relocate
  /// CU relative and absolute references.
  void verifyDebugInfoReferences();

  /// Verify the the DW_AT_stmt_list encoding and value and ensure that no
  /// compile units that have the same DW_AT_stmt_list value.
  void verifyDebugLineStmtOffsets();

  /// Verify that all of the rows in the line table are valid.
  ///
  /// This function currently checks for:
  /// - addresses within a sequence that decrease in value
  /// - invalid file indexes
  void verifyDebugLineRows();

public:
  DWARFVerifier(raw_ostream &S, DWARFContext &D)
      : OS(S), DCtx(D), NumDebugInfoErrors(0), NumDebugLineErrors(0) {}
  /// Verify the information in the .debug_info section.
  ///
  /// Any errors are reported to the stream that was this object was
  /// constructed with.
  ///
  /// @return True if the .debug_info verifies successfully, false otherwise.
  bool handleDebugInfo();

  /// Verify the information in the .debug_line section.
  ///
  /// Any errors are reported to the stream that was this object was
  /// constructed with.
  ///
  /// @return True if the .debug_line verifies successfully, false otherwise.
  bool handleDebugLine();
};

} // end namespace llvm

#endif // LLVM_DEBUGINFO_DWARF_DWARFCONTEXT_H
