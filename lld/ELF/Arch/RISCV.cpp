//===- RISCV.cpp ----------------------------------------------------------===//
//
//                             The LLVM Linker
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "Bits.h"
#include "InputFiles.h"
#include "InputSection.h"
#include "Memory.h"
#include "OutputSections.h"
#include "SymbolTable.h"
#include "Symbols.h"
#include "SyntheticSections.h"
#include "Target.h"
#include "Thunks.h"
#include "Writer.h"
#include "lld/Common/ErrorHandler.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/Object/ELF.h"
#include "llvm/Object/ELFTypes.h"
#include "llvm/Support/Endian.h"
#include "llvm/Support/raw_ostream.h"

#include <memory>

using namespace llvm;
using namespace llvm::object;
using namespace llvm::support::endian;
using namespace llvm::ELF;
using namespace lld;
using namespace lld::elf;

constexpr uint64_t TLS_DTV_OFFSET = 0x800;

namespace {

template <class ELFT> class RISCV final : public TargetInfo {
public:
  RISCV();
  RelExpr getRelExpr(RelType Type, const SymbolBody &S,
                     const uint8_t *Loc) const override;
  void relocateOne(uint8_t *Loc, uint32_t Type, uint64_t Val) const override;

  virtual void writeGotPltHeader(uint8_t *Buf) const override;
  virtual void writeGotPlt(uint8_t *Buf, const SymbolBody &S) const override;

  virtual void writePltHeader(uint8_t *Buf) const override;

  virtual void writePlt(uint8_t *Buf, uint64_t GotEntryAddr,
                        uint64_t PltEntryAddr, int32_t Index,
                        unsigned RelOff) const override;

  virtual bool usesOnlyLowPageBits(uint32_t Type) const override;
};

} // end anonymous namespace

// RISC-V instructions are stored as 16-bit parcels starting with lower bits;
// each parcel is stored according to implementation's endianness.
template <endianness E>
static void writeInsn32(uint8_t *const Buf, const uint32_t Insn) {
  write16<E>(Buf, Insn);
  write16<E>(Buf + 2, Insn >> 16);
}

template <endianness E> static uint32_t readInsn32(const uint8_t *const Buf) {
  return read16<E>(Buf) | static_cast<uint32_t>(read16<E>(Buf + 2)) << 16;
}

template <class ELFT> RISCV<ELFT>::RISCV() {
  CopyRel = R_RISCV_COPY;
  RelativeRel = R_RISCV_RELATIVE;
  GotRel = ELFT::Is64Bits ? R_RISCV_64 : R_RISCV_32;
  PltRel = R_RISCV_JUMP_SLOT;
  GotEntrySize = Config->Wordsize;
  GotPltEntrySize = Config->Wordsize;
  PltEntrySize = 16;
  PltHeaderSize = 32;
  GotPltHeaderEntriesNum = 2;
  if (ELFT::Is64Bits) {
    TlsGotRel = R_RISCV_TLS_TPREL64;
    TlsModuleIndexRel = R_RISCV_TLS_DTPMOD64;
    TlsOffsetRel = R_RISCV_TLS_DTPREL64;
  } else {
    TlsGotRel = R_RISCV_TLS_TPREL32;
    TlsModuleIndexRel = R_RISCV_TLS_DTPMOD32;
    TlsOffsetRel = R_RISCV_TLS_DTPREL32;
  }
}

template <class ELFT>
bool RISCV<ELFT>::usesOnlyLowPageBits(uint32_t Type) const {
  return Type == R_RISCV_LO12_I || Type == R_RISCV_PCREL_LO12_I ||
         Type == R_RISCV_LO12_S || Type == R_RISCV_PCREL_LO12_S ||
         // These are used in a pair to calculate relative address in debug
         // sections, so they aren't really absolute. We list those here as a
         // hack so the linker doesn't try to create dynamic relocations.
         Type == R_RISCV_ADD8 || Type == R_RISCV_ADD16 ||
         Type == R_RISCV_ADD32 || Type == R_RISCV_ADD64 ||
         Type == R_RISCV_SUB8 || Type == R_RISCV_SUB16 ||
         Type == R_RISCV_SUB32 || Type == R_RISCV_SUB64 ||
         Type == R_RISCV_SUB6 ||
         Type == R_RISCV_SET6 || Type == R_RISCV_SET8 ||
         Type == R_RISCV_SET16 || Type == R_RISCV_SET32;
}

template <class ELFT> void RISCV<ELFT>::writeGotPltHeader(uint8_t *Buf) const {
  writeUint(Buf, -1);
  writeUint(Buf + GotPltEntrySize, 0);
}

template <class ELFT>
void RISCV<ELFT>::writeGotPlt(uint8_t *Buf, const SymbolBody &S) const {
  writeUint(Buf, InX::Plt->getVA());
}

template <class ELFT> void RISCV<ELFT>::writePltHeader(uint8_t *Buf) const {
  constexpr endianness E = ELFT::TargetEndianness;
  const uint64_t PcRelGotPlt = InX::GotPlt->getVA() - InX::Plt->getVA();

  writeInsn32<E>(Buf + 0, 0x00000397);    // 1: auipc  t2, %pcrel_hi(.got.plt)
  relocateOne(Buf + 0, R_RISCV_PCREL_HI20, PcRelGotPlt);
  writeInsn32<E>(Buf + 4, 0x41c30333);    // sub    t1, t1, t3
  if (ELFT::Is64Bits) {
    writeInsn32<E>(Buf + 8, 0x0003be03);  // ld     t3, %pcrel_lo(1b)(t2)
  } else {
    writeInsn32<E>(Buf + 8, 0x0003ae03);  // lw     t3, %pcrel_lo(1b)(t2)
  }
  relocateOne(Buf + 8, R_RISCV_PCREL_LO12_I, PcRelGotPlt);
  writeInsn32<E>(Buf + 12, 0xfd430313);   // addi   t1, t1, -44
  writeInsn32<E>(Buf + 16, 0x00038293);   // addi   t0, t2, %pcrel_lo(1b)
  relocateOne(Buf + 16, R_RISCV_PCREL_LO12_I, PcRelGotPlt);
  if (ELFT::Is64Bits) {
    writeInsn32<E>(Buf + 20, 0x00135313); // srli   t1, t1, 1
    writeInsn32<E>(Buf + 24, 0x0082b283); // ld     t0, 8(t0)
  } else {
    writeInsn32<E>(Buf + 20, 0x00235313); // srli   t1, t1, 2
    writeInsn32<E>(Buf + 24, 0x0042a283); // lw     t0, 4(t0)
  }
  writeInsn32<E>(Buf + 28, 0x000e0067);   // jr     t3
}

template <class ELFT>
void RISCV<ELFT>::writePlt(uint8_t *Buf, uint64_t GotEntryAddr,
                           uint64_t PltEntryAddr, int32_t Index,
                           unsigned RelOff) const {
  constexpr endianness E = ELFT::TargetEndianness;

  writeInsn32<E>(Buf +  0, 0x00000e17);   // auipc   t3, %pcrel_hi(f@.got.plt)
  if (ELFT::Is64Bits) {
    writeInsn32<E>(Buf +  4, 0x000e3e03); // ld      t3, %pcrel_lo(-4)(t3)
  } else {
    writeInsn32<E>(Buf +  4, 0x000e2e03); // lw      t3, %pcrel_lo(-4)(t3)
  }
  writeInsn32<E>(Buf +  8, 0x000e0367);   // jalr    t1, t3
  writeInsn32<E>(Buf + 12, 0x00000013);   // nop

  relocateOne(Buf + 0, R_RISCV_PCREL_HI20, GotEntryAddr - PltEntryAddr);
  relocateOne(Buf + 4, R_RISCV_PCREL_LO12_I, GotEntryAddr - PltEntryAddr);
}

template <class ELFT>
RelExpr RISCV<ELFT>::getRelExpr(const RelType Type, const SymbolBody &S,
                                const uint8_t *Loc) const {
  switch (Type) {
  case R_RISCV_JAL:
  case R_RISCV_BRANCH:
  case R_RISCV_CALL:
  case R_RISCV_PCREL_HI20:
  case R_RISCV_RVC_BRANCH:
  case R_RISCV_RVC_JUMP:
  case R_RISCV_32_PCREL:
    return R_PC;
  case R_RISCV_CALL_PLT:
    return R_PLT_PC;
  case R_RISCV_PCREL_LO12_I:
  case R_RISCV_PCREL_LO12_S:
    return R_RISCV_PC_INDIRECT;
  case R_RISCV_GOT_HI20:
  case R_RISCV_TLS_GOT_HI20:
    return R_GOT_PC;
  case R_RISCV_TPREL_HI20:
  case R_RISCV_TPREL_LO12_I:
  case R_RISCV_TPREL_LO12_S:
    return R_TLS;
  case R_RISCV_TLS_GD_HI20:
    return R_TLSGD_PC;
  case R_RISCV_ALIGN:
  case R_RISCV_RELAX:
  case R_RISCV_TPREL_ADD:
    return R_HINT;
  default:
    return R_ABS;
  }
}

// Extract bits V[Begin:End].
uint32_t extractBits(uint64_t V, uint32_t Begin, uint32_t End) {
  return (V & ((static_cast<uint64_t>(1) << (Begin + 1)) - 1)) >> End;
}

// Reference:
// https://github.com/riscv/riscv-elf-psabi-doc/blob/master/riscv-elf.md
template <class ELFT>
void RISCV<ELFT>::relocateOne(uint8_t *Loc, const uint32_t Type,
                              const uint64_t Val) const {
  constexpr endianness E = ELFT::TargetEndianness;

  switch (Type) {
  case R_RISCV_32:
    write32<E>(Loc, Val);
    return;
  case R_RISCV_64:
    write64<E>(Loc, Val);
    return;

  case R_RISCV_RVC_BRANCH: {
    checkInt<8>(Loc, static_cast<int64_t>(Val) >> 1, Type);
    checkAlignment<2>(Loc, Val, Type);
    uint16_t Inst = read16<E>(Loc) & 0xE383;
    uint16_t Imm8 = extractBits(Val, 8, 8) << 12;
    uint16_t Imm4_3 = extractBits(Val, 4, 3) << 10;
    uint16_t Imm7_6 = extractBits(Val, 7, 6) << 5;
    uint16_t Imm2_1 = extractBits(Val, 2, 1) << 3;
    uint16_t Imm5 = extractBits(Val, 5, 5) << 2;
    write16<E>(Loc, Inst | Imm8 | Imm4_3 | Imm7_6 | Imm2_1 | Imm5);
    return;
  }

  case R_RISCV_RVC_JUMP: {
    checkInt<11>(Loc, static_cast<int64_t>(Val) >> 1, Type);
    checkAlignment<2>(Loc, Val, Type);
    uint16_t Inst = read16<E>(Loc) & 0xE003;
    uint16_t Imm11 = extractBits(Val, 11, 11) << 12;
    uint16_t Imm4 = extractBits(Val, 4, 4) << 11;
    uint16_t Imm9_8 = extractBits(Val, 9, 8) << 9;
    uint16_t Imm10 = extractBits(Val, 10, 10) << 8;
    uint16_t Imm6 = extractBits(Val, 6, 6) << 7;
    uint16_t Imm7 = extractBits(Val, 7, 7) << 6;
    uint16_t Imm3_1 = extractBits(Val, 3, 1) << 3;
    uint16_t Imm5 = extractBits(Val, 5, 5) << 2;
    write16<E>(Loc,
        Inst | Imm11 | Imm4 | Imm9_8 | Imm10 | Imm6 | Imm7 | Imm3_1 | Imm5);
    return;
  }

  case R_RISCV_RVC_LUI: {
    int32_t Imm = ((Val + 0x800) >> 12);
    checkUInt<6>(Loc, Imm, Type);
    if (Imm == 0) { // `c.lui rd, 0` is illegal, convert to `c.li rd, 0`
      write16<E>(Loc, (read16<E>(Loc) & 0x0F83) | 0x4000);
    } else {
      uint16_t Imm17 = extractBits(Val + 0x800, 17, 17) << 12;
      uint16_t Imm16_12 = extractBits(Val + 0x800, 16, 12) << 2;
      write16<E>(Loc, (read16<E>(Loc) & 0xEF83) | Imm17 | Imm16_12);
    }
    return;
  }

  case R_RISCV_JAL: {
    checkInt<20>(Loc, static_cast<int64_t>(Val) >> 1, Type);
    checkAlignment<2>(Loc, Val, Type);

    uint32_t Inst = readInsn32<E>(Loc) & 0xFFF;
    uint32_t Imm20 = extractBits(Val, 20, 20) << 31;
    uint32_t Imm10_1 = extractBits(Val, 10, 1) << 21;
    uint32_t Imm11 = extractBits(Val, 11, 11) << 20;
    uint32_t Imm19_12 = extractBits(Val, 19, 12) << 12;
    uint32_t Imm20_1 = Imm20 | Imm10_1 | Imm11 | Imm19_12;

    writeInsn32<E>(Loc, Inst | Imm20_1);
    return;
  }

  case R_RISCV_BRANCH: {
    checkInt<12>(Loc, static_cast<int64_t>(Val) >> 1, Type);
    checkAlignment<2>(Loc, Val, Type);

    uint32_t Inst = readInsn32<E>(Loc) & 0x1FFF07F;
    uint32_t Imm12 = extractBits(Val, 12, 12) << 31;
    uint32_t Imm10_5 = extractBits(Val, 10, 5) << 25;
    uint32_t Imm4_1 = extractBits(Val, 4, 1) << 8;
    uint32_t Imm11 = extractBits(Val, 11, 11) << 7;
    uint32_t Imm12_1 = Imm12 | Imm10_5 | Imm4_1 | Imm11;

    writeInsn32<E>(Loc, Inst | Imm12_1);
    return;
  }
  // auipc + jalr pair
  case R_RISCV_CALL_PLT:
  case R_RISCV_CALL: {
    checkInt<32>(Loc, Val, Type);
    uint32_t Hi = Val + 0x800;
    uint32_t Lo = Val - (Hi & 0xFFFFF000);
    writeInsn32<E>(Loc, (readInsn32<E>(Loc) & 0xFFF) | (Hi & 0xFFFFF000));
    writeInsn32<E>(Loc + 4,
                   (readInsn32<E>(Loc + 4) & 0xFFFFF) | ((Lo & 0xFFF) << 20));
    return;
  }

  case R_RISCV_PCREL_HI20:
  case R_RISCV_TPREL_HI20:
  case R_RISCV_GOT_HI20:
  case R_RISCV_TLS_GOT_HI20:
  case R_RISCV_TLS_GD_HI20:
  case R_RISCV_HI20: {
    checkInt<32>(Loc, Val, Type);
    uint32_t Hi = Val + 0x800;
    writeInsn32<E>(Loc, (readInsn32<E>(Loc) & 0xFFF) | (Hi & 0xFFFFF000));
    return;
  }
  case R_RISCV_PCREL_LO12_I:
  case R_RISCV_TPREL_LO12_I:
  case R_RISCV_LO12_I: {
    checkInt<32>(Loc, Val, Type);
    uint32_t Hi = Val + 0x800;
    uint32_t Lo = Val - (Hi & 0xFFFFF000);
    writeInsn32<E>(Loc, (readInsn32<E>(Loc) & 0xFFFFF) | ((Lo & 0xFFF) << 20));
    return;
  }
  case R_RISCV_PCREL_LO12_S:
  case R_RISCV_TPREL_LO12_S:
  case R_RISCV_LO12_S: {
    checkInt<32>(Loc, Val, Type);
    uint32_t Hi = Val + 0x800;
    uint32_t Lo = Val - (Hi & 0xFFFFF000);
    uint32_t Imm11_5 = extractBits(Lo, 11, 5) << 25;
    uint32_t Imm4_0 = extractBits(Lo, 4, 0) << 7;
    writeInsn32<E>(Loc, (readInsn32<E>(Loc) & 0x1FFF07F) | Imm11_5 | Imm4_0);
    return;
  }

  case R_RISCV_TLS_DTPMOD32:
    write32<E>(Loc, 1);
    return;
  case R_RISCV_TLS_DTPMOD64:
    write64<E>(Loc, 1);
    return;
  case R_RISCV_TLS_DTPREL32:
    write32<E>(Loc, Val - TLS_DTV_OFFSET);
    return;
  case R_RISCV_TLS_DTPREL64:
    write64<E>(Loc, Val - TLS_DTV_OFFSET);
    return;
  case R_RISCV_TLS_TPREL32:
    write32<E>(Loc, Val);
    return;
  case R_RISCV_TLS_TPREL64:
    write64<E>(Loc, Val);
    return;
  case R_RISCV_TPREL_ADD:
    return; // Do nothing

  case R_RISCV_ADD8:
    *Loc += Val;
    return;
  case R_RISCV_ADD16:
    write16<E>(Loc, read16<E>(Loc) + Val);
    return;
  case R_RISCV_ADD32:
    write32<E>(Loc, read32<E>(Loc) + Val);
    return;
  case R_RISCV_ADD64:
    write64<E>(Loc, read64<E>(Loc) + Val);
    return;
  case R_RISCV_SUB6:
    *Loc = (*Loc & 0xc0) | (((*Loc & 0x3f) - Val) & 0x3f);
    return;
  case R_RISCV_SUB8:
    *Loc -= Val;
    return;
  case R_RISCV_SUB16:
    write16<E>(Loc, read16<E>(Loc) - Val);
    return;
  case R_RISCV_SUB32:
    write32<E>(Loc, read32<E>(Loc) - Val);
    return;
  case R_RISCV_SUB64:
    write64<E>(Loc, read64<E>(Loc) - Val);
    return;
  case R_RISCV_SET6:
    *Loc = (*Loc & 0xc0) | (Val & 0x3f);
    return;
  case R_RISCV_SET8:
    *Loc = Val;
    return;
  case R_RISCV_SET16:
    write16<E>(Loc, Val);
    return;
  case R_RISCV_SET32:
  case R_RISCV_32_PCREL:
    write32<E>(Loc, Val);
    return;

  case R_RISCV_ALIGN:
  case R_RISCV_RELAX:
    return; // Ignored (for now)
  case R_RISCV_NONE:
    return; // Do nothing

  // These are handled by the dynamic linker
  case R_RISCV_RELATIVE:
  case R_RISCV_COPY:
  case R_RISCV_JUMP_SLOT:
  // GP-relative relocations are only produced after relaxation, which
  // we don't support for now
  case R_RISCV_GPREL_I:
  case R_RISCV_GPREL_S:
  default:
    error(getErrorLocation(Loc) +
          "unimplemented relocation type: " + Twine(Type));
    return;
  }
}

template <class ELFT> TargetInfo *elf::getRISCVTargetInfo() {
  static RISCV<ELFT> Target;
  return &Target;
}

template TargetInfo *elf::getRISCVTargetInfo<ELF32LE>();
template TargetInfo *elf::getRISCVTargetInfo<ELF64LE>();
