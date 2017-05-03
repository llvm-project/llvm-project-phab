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
  struct RangeInfoType {
    DWARFDie Die;
    DWARFAddressRangesVector Ranges;

    RangeInfoType() : Die(), Ranges() {}
    RangeInfoType(DWARFDie D, const DWARFAddressRangesVector &R);
    /// Return true if this object contains all ranges within RHS.
    bool Contains(const RangeInfoType &RHS);
    bool DoesIntersect(const RangeInfoType &RHS) const;
    void Dump(raw_ostream &OS) const;
    bool operator<(const RangeInfoType &RHS) const;
    static bool CheckForRangeErrors(raw_ostream &OS,
                                    DWARFAddressRangesVector &Ranges);
  };

  raw_ostream &OS;
  DWARFContext &DCtx;
  /// A map that tracks all references (converted absolute references) so we
  /// can verify each reference points to a valid DIE and not an offset that
  /// lies between to valid DIEs.
  std::map<uint64_t, std::set<uint32_t>> ReferenceToDIEOffsets;

  /// Keep a set of all DW_TAG_subprogram range infos across all compile
  /// units so we can look for overlapping function ranges after we go through
  /// all of the DIEs.
  std::set<RangeInfoType> AllFunctionRangeInfos;
  /// A vector of range infos for all DIEs where the index is the depth of the
  /// DIE. This helps to verify address ranges so we can tell if a child's
  /// address ranges are fully contained in a parent's address ranges.
  std::vector<RangeInfoType> DieRangeInfos;

  uint32_t NumDebugInfoErrors;
  uint32_t NumDebugLineErrors;

  /// Verifies the a DIE's tag and gathers information about all DIEs.
  ///
  /// This function currently checks for:
  /// - Checks DW_TAG_compile_unit address range(s)
  /// - Checks DW_TAG_subprogram address range(s) and if the compile unit
  ///   has ranges, verifies that its address range is fully contained in
  ///   the compile unit ranges. Also adds the functions address range info
  ///   to AllFunctionRangeInfos to look for functions with overlapping ranges
  ///   after all DIEs have been processed.
  /// - Checks that DW_TAG_lexical_block and DW_TAG_inlined_subroutine DIEs
  ///   have address range(s) that are fully contained in their parent DIEs
  ///   address range(s).
  ///
  /// @param Die          The DWARF DIE to check
  void verifyDebugInfoTag(DWARFDie &Die);

  /// Verifies that we have no overlapping function address ranges within all
  /// of the DWARF.
  void verifyDebugInfoOverlappingFunctionRanges();

  /// Verifies the attribute's DWARF attribute and its value.
  ///
  /// This function currently checks for:
  /// - DW_AT_ranges values is a valid .debug_ranges offset
  /// - DW_AT_stmt_list is a valid .debug_line offset
  ///
  /// @param Die          The DWARF DIE that owns the attribute value
  /// @param AttrValue    The DWARF attribute value to check
  void verifyDebugInfoAttribute(DWARFDie &Die, DWARFAttribute &AttrValue);

  /// Verifies the attribute's DWARF form.
  ///
  /// This function currently checks for:
  /// - All DW_FORM_ref values that are CU relative have valid CU offsets
  /// - All DW_FORM_ref_addr values have valid .debug_info offsets
  /// - All DW_FORM_strp values have valid .debug_str offsets
  ///
  /// @param Die          The DWARF DIE that owns the attribute value
  /// @param AttrValue    The DWARF attribute value to check
  void verifyDebugInfoForm(DWARFDie &Die, DWARFAttribute &AttrValue);

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
