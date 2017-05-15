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
public:
  struct DWARFRange {
    uint64_t Start;
    uint64_t End;
    DWARFRange(uint64_t S, uint64_t E) : Start(S), End(E) {}
    /// Returns true if [Start, End) intersects with [RHS.Start, RHS.End).
    bool intersects(const DWARFRange &RHS) const {
      // Empty ranges can't intersect.
      if (Start == End || RHS.Start == RHS.End)
        return false;
      return (Start < RHS.End) && (End > RHS.Start);
    }
    /// Returns true if [Start, End) fully contains [RHS.Start, RHS.End).
    bool contains(const DWARFRange &RHS) const {
      if (Start <= RHS.Start && RHS.Start < End)
        return Start < RHS.End && RHS.End <= End;
      return false;
    }
  };

  /// A class that keeps the address range information for a single DIE.
  struct DieRangeInfo {
    DWARFDie Die;
    /// Sorted address ranges.
    std::vector<DWARFRange> Ranges;
    DieRangeInfo() = default;
    /// Constructor used for unit testing.
    DieRangeInfo(const DWARFAddressRangesVector &R) { insert(R); }
    /// Return true if ranges in this object contains all ranges within RHS.
    bool contains(const DieRangeInfo &RHS) const;
    /// Return true if any ranges in RHS overlap with ranges in this object.
    bool intersects(const DieRangeInfo &RHS) const;
    void dump(raw_ostream &OS) const;
    /// Extract address ranges from the Die in this object and return true if
    /// there are errors in the ranges.
    bool extractRangesAndReportErrors(raw_ostream &OS);
    /// Return true if this object doesn't fully contain the ranges in RI
    /// and report errors to the stream.
    bool reportErrorIfNotContained(raw_ostream &OS, const DieRangeInfo &RI,
                                   const char *Error) const;
    /// Inserts the unsorted ranges and returns true if errors were found
    /// during insertion
    bool insert(const DWARFAddressRangesVector &UnsortedRanges);
  };

  /// A class that ensures that no two DieRangeInfo's overlap.
  struct NonOverlappingRanges {
    std::set<DieRangeInfo> RangeSet;

    /// Returns true if the sibling ranges
    Optional<DWARFDie> GetOverlappingRangeInfo(const DieRangeInfo &RI) const;

    bool insertAndReportErrors(raw_ostream &OS, const DieRangeInfo &RI);
  };

  /// A class the keeps information for a DIE that contains child DIEs whose
  /// address ranges must be contained within its ranges and whose direct
  /// children that have address ranges must not overlap.
  struct DieInfo {
    DieRangeInfo RI;
    NonOverlappingRanges NOR;
    /// Returns true if an error was found when inserting Die.
    bool addContainedDieAndReportErrors(raw_ostream &OS, DieRangeInfo &DieRI);
  };

private:
  raw_ostream &OS;
  DWARFContext &DCtx;
  DieInfo UnitDI;
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
  /// \param ParantDI     The parent DIE's range and overlap information.
  void verifyDie(const DWARFDie &Die, DieInfo &ParentDI);

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

static inline bool operator<(const DWARFVerifier::DWARFRange &LHS,
                             const DWARFVerifier::DWARFRange &RHS) {
  return std::tie(LHS.Start, LHS.End) < std::tie(RHS.Start, RHS.End);
}

static inline bool operator<(const DWARFVerifier::DieRangeInfo &LHS,
                             const DWARFVerifier::DieRangeInfo &RHS) {
  return std::tie(LHS.Ranges, LHS.Die) < std::tie(RHS.Ranges, RHS.Die);
}

} // end namespace llvm

#endif // LLVM_DEBUGINFO_DWARF_DWARFCONTEXT_H
