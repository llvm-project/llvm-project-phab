//===-- DWARFCompileUnit.h --------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef SymbolFileDWARF_DWARFCompileUnit_h_
#define SymbolFileDWARF_DWARFCompileUnit_h_

#include "DWARFDIE.h"
#include "DWARFDebugInfoEntry.h"
#include "lldb/lldb-enumerations.h"

class NameToDIE;
class SymbolFileDWARF;
class SymbolFileDWARFDwo;

class DWARFCompileUnit {
public:
  enum Producer {
    eProducerInvalid = 0,
    eProducerClang,
    eProducerGCC,
    eProducerLLVMGCC,
    eProcucerOther
  };

  DWARFCompileUnit(SymbolFileDWARF *dwarf2Data);
  ~DWARFCompileUnit();

  bool Extract(const lldb_private::DWARFDataExtractor &debug_info,
               lldb::offset_t *offset_ptr, bool is_type_unit);
  const lldb_private::DWARFDataExtractor &GetData() const;
  size_t ExtractDIEsIfNeeded(bool cu_die_only);
  DWARFDIE LookupAddress(const dw_addr_t address);
  size_t AppendDIEsWithTag(const dw_tag_t tag,
                           DWARFDIECollection &matching_dies,
                           uint32_t depth = UINT32_MAX) const;
  void Clear();
  void Dump(lldb_private::Stream *s) const;
  dw_offset_t GetOffset() const { return m_offset; }
  size_t GetLengthByteSize() const { return m_is_dwarf64 ? 12 : 4; }
  lldb::user_id_t GetID() const;
  uint32_t Size() const {
    // Size in bytes of the compile or type unit header
    // Start with the common size between compile and type units
    uint32_t header_size = m_is_dwarf64 ? 23 : 11;
    // Add the extra type unit size if needed
    if (IsTypeUnit())
      header_size += m_is_dwarf64 ? 16 : 12;
    return header_size;
  }
  bool ContainsDIEOffset(dw_offset_t die_offset) const {
    return die_offset >= GetFirstDIEOffset() &&
           die_offset < GetNextCompileUnitOffset();
  }
  dw_offset_t GetFirstDIEOffset() const { return m_offset + Size(); }
  dw_offset_t GetNextCompileUnitOffset() const {
    return m_offset + m_length + GetLengthByteSize();
  }
  size_t GetDebugInfoSize() const {
    // Size in bytes of the .debug_info or .debug_types data associated with
    // this compile or type unit.
    return m_length + GetLengthByteSize() - Size();
  }
  uint32_t GetLength() const { return m_length; }
  uint16_t GetVersion() const { return m_version; }
  const DWARFAbbreviationDeclarationSet *GetAbbreviations() const {
    return m_abbrevs;
  }
  dw_offset_t GetAbbrevOffset() const;
  uint8_t GetAddressByteSize() const { return m_addr_size; }
  dw_addr_t GetBaseAddress() const { return m_base_addr; }
  dw_addr_t GetAddrBase() const { return m_addr_base; }
  dw_addr_t GetRangesBase() const { return m_ranges_base; }
  void SetAddrBase(dw_addr_t addr_base, dw_addr_t ranges_base, dw_offset_t base_obj_offset);
  void ClearDIEs(bool keep_compile_unit_die);
  void BuildAddressRangeTable(SymbolFileDWARF *dwarf2Data,
                              DWARFDebugAranges *debug_aranges);

  lldb::ByteOrder GetByteOrder() const;

  lldb_private::TypeSystem *GetTypeSystem();

  DWARFFormValue::FixedFormSizes GetFixedFormSizes();

  void SetBaseAddress(dw_addr_t base_addr) { m_base_addr = base_addr; }

  DWARFDIE
  GetCompileUnitDIEOnly() { return DWARFDIE(this, GetCompileUnitDIEPtrOnly()); }

  DWARFDIE
  DIE() { return DWARFDIE(this, DIEPtr()); }

  void AddDIE(DWARFDebugInfoEntry &die) {
    // The average bytes per DIE entry has been seen to be
    // around 14-20 so lets pre-reserve half of that since
    // we are now stripping the NULL tags.

    // Only reserve the memory if we are adding children of
    // the main compile unit DIE. The compile unit DIE is always
    // the first entry, so if our size is 1, then we are adding
    // the first compile unit child DIE and should reserve
    // the memory.
    if (m_die_array.empty())
      m_die_array.reserve(GetDebugInfoSize() / 24);
    m_die_array.push_back(die);
  }

  void AddCompileUnitDIE(DWARFDebugInfoEntry &die);

  bool HasDIEsParsed() const { return m_die_array.size() > 1; }

  DWARFDIE
  GetDIE(dw_offset_t die_offset);

  static uint8_t GetAddressByteSize(const DWARFCompileUnit *cu);

  static bool IsDWARF64(const DWARFCompileUnit *cu);

  static uint8_t GetDefaultAddressSize();

  static void SetDefaultAddressSize(uint8_t addr_size);

  void *GetUserData() const { return m_user_data; }

  void SetUserData(void *d);

  bool Supports_DW_AT_APPLE_objc_complete_type();

  bool DW_AT_decl_file_attributes_are_invalid();

  bool Supports_unnamed_objc_bitfields();

  void Index(NameToDIE &func_basenames, NameToDIE &func_fullnames,
             NameToDIE &func_methods, NameToDIE &func_selectors,
             NameToDIE &objc_class_selectors, NameToDIE &globals,
             NameToDIE &types, NameToDIE &namespaces);

  const DWARFDebugAranges &GetFunctionAranges();

  SymbolFileDWARF *GetSymbolFileDWARF() const { return m_dwarf2Data; }

  Producer GetProducer();

  uint32_t GetProducerVersionMajor();

  uint32_t GetProducerVersionMinor();

  uint32_t GetProducerVersionUpdate();

  static lldb::LanguageType LanguageTypeFromDWARF(uint64_t val);

  lldb::LanguageType GetLanguageType();

  bool IsDWARF64() const;

  bool GetIsOptimized();

  SymbolFileDWARFDwo *GetDwoSymbolFile() const {
    return m_dwo_symbol_file.get();
  }

  dw_offset_t GetBaseObjOffset() const { return m_base_obj_offset; }

  // Return true if this compile unit is a type unit.
  bool IsTypeUnit() const { return m_type_offset != DW_INVALID_OFFSET; }
  // Return the type signature for the type contained in this type unit. This
  // value will not be valid if this compile unit is not a type unit.
  uint64_t GetTypeSignature() const { return m_type_signature; }
  // If this compile unit is a type unit, then return the DWARFDIE that
  // respresents the type contained in the type unit, else return an invalid
  // DIE.
  DWARFDIE GetTypeUnitDIE();
  // If this compile unit is a type unit, then return the DIE offset for the
  // type contained in the type unit, else return an invalid DIE offset.
  dw_offset_t GetTypeUnitDIEOffset() {
    if (IsTypeUnit())
      return m_offset + m_type_offset;
    return DW_INVALID_OFFSET;
  }
  
  // Find the DIE for any given type signature.
  //
  // This is a convenience function that allows anyone to use the current
  // compile unit to access the DWARF and use its debug info to retrieve a DIE
  // that represents a type given a type signature. This function will cause all
  // DIEs in the type unit to be parsed, only call if you need the actual DIE
  // object.
  DWARFDIE FindTypeSignatureDIE(uint64_t type_sig) const;
  
  // Find the DIE offset for any given type signature.
  //
  // This is a convenience function that allows anyone to use the current
  // compile unit to access the DWARF and use its debug info to retrieve a DIE
  // offset that represents a type given a type signature. This function doesn't
  // cause any debug information to be parsed, so if clients only need the
  // DIE offset of a type signature, this function is more efficient than
  // DWARFDIE FindTypeSignatureDIE(...) above.
  dw_offset_t FindTypeSignatureDIEOffset(uint64_t type_sig) const;
  
protected:
  SymbolFileDWARF *m_dwarf2Data;
  std::unique_ptr<SymbolFileDWARFDwo> m_dwo_symbol_file;
  const DWARFAbbreviationDeclarationSet *m_abbrevs;
  void *m_user_data;
  DWARFDebugInfoEntry::collection
      m_die_array; // The compile unit debug information entry item
  std::unique_ptr<DWARFDebugAranges> m_func_aranges_ap; // A table similar to
                                                        // the .debug_aranges
                                                        // table, but this one
                                                        // points to the exact
                                                        // DW_TAG_subprogram
                                                        // DIEs
  dw_addr_t m_base_addr;
  dw_offset_t m_offset;
  dw_offset_t m_length;
  uint16_t m_version;
  uint8_t m_addr_size;
  Producer m_producer;
  uint32_t m_producer_version_major;
  uint32_t m_producer_version_minor;
  uint32_t m_producer_version_update;
  lldb::LanguageType m_language_type;
  bool m_is_dwarf64;
  lldb_private::LazyBool m_is_optimized;
  dw_addr_t m_addr_base;         // Value of DW_AT_addr_base
  dw_addr_t m_ranges_base;       // Value of DW_AT_ranges_base
  dw_offset_t m_base_obj_offset; // If this is a dwo compile unit this is the
                                 // offset of the base compile unit in the main
                                 // object file
  uint64_t m_type_signature;     // Type signature contained in a type unit
                                 // which will be valid (non-zero) for type
                                 // units only.
  dw_offset_t m_type_offset;     // Compile unit relative type offset for type
                                 // units only.

  void ParseProducerInfo();

  static void
  IndexPrivate(DWARFCompileUnit *dwarf_cu, const lldb::LanguageType cu_language,
               const DWARFFormValue::FixedFormSizes &fixed_form_sizes,
               const dw_offset_t cu_offset, NameToDIE &func_basenames,
               NameToDIE &func_fullnames, NameToDIE &func_methods,
               NameToDIE &func_selectors, NameToDIE &objc_class_selectors,
               NameToDIE &globals, NameToDIE &types, NameToDIE &namespaces);

private:
  const DWARFDebugInfoEntry *GetCompileUnitDIEPtrOnly() {
    ExtractDIEsIfNeeded(true);
    if (m_die_array.empty())
      return NULL;
    return &m_die_array[0];
  }

  const DWARFDebugInfoEntry *DIEPtr() {
    ExtractDIEsIfNeeded(false);
    if (m_die_array.empty())
      return NULL;
    return &m_die_array[0];
  }

  DISALLOW_COPY_AND_ASSIGN(DWARFCompileUnit);
};

#endif // SymbolFileDWARF_DWARFCompileUnit_h_
