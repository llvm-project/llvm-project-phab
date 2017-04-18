//===-- DWARFDataExtractor.h ------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef liblldb_DWARFDataExtractor_h_
#define liblldb_DWARFDataExtractor_h_

// Other libraries and framework includes.
#include "lldb/Core/dwarf.h"
#include "lldb/Utility/DataExtractor.h"

namespace lldb_private {

class DWARFDataExtractor : public DataExtractor {
public:
  DWARFDataExtractor() : DataExtractor(), m_is_dwarf64(false) {}

  DWARFDataExtractor(const DWARFDataExtractor &data, lldb::offset_t offset,
                     lldb::offset_t length)
      : DataExtractor(data, offset, length), m_is_dwarf64(false) {}

  uint64_t GetDWARFInitialLength(lldb::offset_t *offset_ptr) const;

  dw_offset_t GetDWARFOffset(lldb::offset_t *offset_ptr) const;

  size_t GetDWARFSizeofInitialLength() const { return m_is_dwarf64 ? 12 : 4; }
  size_t GetDWARFSizeOfOffset() const { return m_is_dwarf64 ? 8 : 4; }

  bool IsDWARF64() const { return m_is_dwarf64; }

  //------------------------------------------------------------------
  /// Slide the data in the buffer so that access to the data will
  /// start at offset \a offset.
  ///
  /// This is currently used to provide access to the .debug_types
  /// section and pretend it starts at at offset of the size of the
  /// .debug_info section. This allows a minimally invasive change to
  /// add support for .debug_types by allowing all DIEs to have unique
  /// offsets and thus allowing no code to change in the DWARF parser.
  /// Modifying the offsets in the .debug_types doesn't affect
  /// anything because since all info in the .debug_types is type unit
  /// relative and no types within a type unit can refer to any DIEs
  /// outside of the type unit without using DW_AT_signature. It also
  /// sets us up to move to DWARF5 where there is no .debug_types
  /// section as compile units and type units are in the .debug_info.
  //------------------------------------------------------------------
  void OffsetData(lldb::offset_t offset)
  {
    if (GetByteSize())
      m_start -= offset;
  }

protected:
  mutable bool m_is_dwarf64;
};
}

#endif // liblldb_DWARFDataExtractor_h_
