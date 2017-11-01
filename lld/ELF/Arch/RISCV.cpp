//===- RISCV.cpp ----------------------------------------------------------===//
//
//                             The LLVM Linker
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

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

namespace {

class RISCV final : public TargetInfo {
public:
  RelExpr getRelExpr(RelType Type, const SymbolBody &S,
                     const uint8_t *Loc) const override;
  void relocateOne(uint8_t *Loc, RelType Type, uint64_t Val) const override;
};

} // end anonymous namespace

RelExpr RISCV::getRelExpr(const RelType Type, const SymbolBody &S,
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
  case R_RISCV_PCREL_LO12_I:
  case R_RISCV_PCREL_LO12_S:
    return R_RISCV_PC_INDIRECT;
  case R_RISCV_RELAX:
  case R_RISCV_ALIGN:
    return R_HINT;
  default:
    return R_ABS;
  }
}

// Extract bits V[Begin:End].
static uint32_t extractBits(uint64_t V, uint32_t Begin, uint32_t End) {
  return (V & ((1ULL << (Begin + 1)) - 1)) >> End;
}

void RISCV::relocateOne(uint8_t *Loc, const RelType Type,
                        const uint64_t Val) const {
  switch (Type) {
  case R_RISCV_32:
    write32le(Loc, Val);
    return;
  case R_RISCV_64:
    write64le(Loc, Val);
    return;

  case R_RISCV_RVC_BRANCH: {
    checkInt<8>(Loc, static_cast<int64_t>(Val) >> 1, Type);
    checkAlignment<2>(Loc, Val, Type);
    uint16_t Inst = read16le(Loc) & 0xE383;
    uint16_t Imm8 = extractBits(Val, 8, 8) << 12;
    uint16_t Imm4_3 = extractBits(Val, 4, 3) << 10;
    uint16_t Imm7_6 = extractBits(Val, 7, 6) << 5;
    uint16_t Imm2_1 = extractBits(Val, 2, 1) << 3;
    uint16_t Imm5 = extractBits(Val, 5, 5) << 2;
    write16le(Loc, Inst | Imm8 | Imm4_3 | Imm7_6 | Imm2_1 | Imm5);
    return;
  }

  case R_RISCV_RVC_JUMP: {
    checkInt<11>(Loc, static_cast<int64_t>(Val) >> 1, Type);
    checkAlignment<2>(Loc, Val, Type);
    uint16_t Inst = read16le(Loc) & 0xE003;
    uint16_t Imm11 = extractBits(Val, 11, 11) << 12;
    uint16_t Imm4 = extractBits(Val, 4, 4) << 11;
    uint16_t Imm9_8 = extractBits(Val, 9, 8) << 9;
    uint16_t Imm10 = extractBits(Val, 10, 10) << 8;
    uint16_t Imm6 = extractBits(Val, 6, 6) << 7;
    uint16_t Imm7 = extractBits(Val, 7, 7) << 6;
    uint16_t Imm3_1 = extractBits(Val, 3, 1) << 3;
    uint16_t Imm5 = extractBits(Val, 5, 5) << 2;
    write16le(Loc,
        Inst | Imm11 | Imm4 | Imm9_8 | Imm10 | Imm6 | Imm7 | Imm3_1 | Imm5);
    return;
  }

  case R_RISCV_RVC_LUI: {
    int32_t Imm = ((Val + 0x800) >> 12);
    checkUInt<6>(Loc, Imm, Type);
    if (Imm == 0) { // `c.lui rd, 0` is illegal, convert to `c.li rd, 0`
      write16le(Loc, (read16le(Loc) & 0x0F83) | 0x4000);
    } else {
      uint16_t Imm17 = extractBits(Val + 0x800, 17, 17) << 12;
      uint16_t Imm16_12 = extractBits(Val + 0x800, 16, 12) << 2;
      write16le(Loc, (read16le(Loc) & 0xEF83) | Imm17 | Imm16_12);
    }
    return;
  }

  case R_RISCV_JAL: {
    checkInt<20>(Loc, static_cast<int64_t>(Val) >> 1, Type);
    checkAlignment<2>(Loc, Val, Type);

    uint32_t Inst = read32le(Loc) & 0xFFF;
    uint32_t Imm20 = extractBits(Val, 20, 20) << 31;
    uint32_t Imm10_1 = extractBits(Val, 10, 1) << 21;
    uint32_t Imm11 = extractBits(Val, 11, 11) << 20;
    uint32_t Imm19_12 = extractBits(Val, 19, 12) << 12;
    uint32_t Imm20_1 = Imm20 | Imm10_1 | Imm11 | Imm19_12;

    write32le(Loc, Inst | Imm20_1);
    return;
  }

  case R_RISCV_BRANCH: {
    checkInt<12>(Loc, static_cast<int64_t>(Val) >> 1, Type);
    checkAlignment<2>(Loc, Val, Type);

    uint32_t Inst = read32le(Loc) & 0x1FFF07F;
    uint32_t Imm12 = extractBits(Val, 12, 12) << 31;
    uint32_t Imm10_5 = extractBits(Val, 10, 5) << 25;
    uint32_t Imm4_1 = extractBits(Val, 4, 1) << 8;
    uint32_t Imm11 = extractBits(Val, 11, 11) << 7;
    uint32_t Imm12_1 = Imm12 | Imm10_5 | Imm4_1 | Imm11;

    write32le(Loc, Inst | Imm12_1);
    return;
  }
  // auipc + jalr pair
  case R_RISCV_CALL: {
    checkInt<32>(Loc, Val, Type);
    relocateOne(Loc, R_RISCV_PCREL_HI20, Val);
    relocateOne(Loc + 4, R_RISCV_PCREL_LO12_I, Val);
    return;
  }

  case R_RISCV_PCREL_HI20:
  case R_RISCV_HI20: {
    checkInt<32>(Loc, Val, Type);
    uint32_t Hi = Val + 0x800;
    write32le(Loc, (read32le(Loc) & 0xFFF) | (Hi & 0xFFFFF000));
    return;
  }
  case R_RISCV_PCREL_LO12_I:
  case R_RISCV_LO12_I: {
    checkInt<32>(Loc, Val, Type);
    uint32_t Hi = Val + 0x800;
    uint32_t Lo = Val - (Hi & 0xFFFFF000);
    write32le(Loc, (read32le(Loc) & 0xFFFFF) | ((Lo & 0xFFF) << 20));
    return;
  }
  case R_RISCV_PCREL_LO12_S:
  case R_RISCV_LO12_S: {
    checkInt<32>(Loc, Val, Type);
    uint32_t Hi = Val + 0x800;
    uint32_t Lo = Val - (Hi & 0xFFFFF000);
    uint32_t Imm11_5 = extractBits(Lo, 11, 5) << 25;
    uint32_t Imm4_0 = extractBits(Lo, 4, 0) << 7;
    write32le(Loc, (read32le(Loc) & 0x1FFF07F) | Imm11_5 | Imm4_0);
    return;
  }

  case R_RISCV_ADD8:
    *Loc += Val;
    return;
  case R_RISCV_ADD16:
    write16le(Loc, read16le(Loc) + Val);
    return;
  case R_RISCV_ADD32:
    write32le(Loc, read32le(Loc) + Val);
    return;
  case R_RISCV_ADD64:
    write64le(Loc, read64le(Loc) + Val);
    return;
  case R_RISCV_SUB6:
    *Loc = (*Loc & 0xc0) | (((*Loc & 0x3f) - Val) & 0x3f);
    return;
  case R_RISCV_SUB8:
    *Loc -= Val;
    return;
  case R_RISCV_SUB16:
    write16le(Loc, read16le(Loc) - Val);
    return;
  case R_RISCV_SUB32:
    write32le(Loc, read32le(Loc) - Val);
    return;
  case R_RISCV_SUB64:
    write64le(Loc, read64le(Loc) - Val);
    return;
  case R_RISCV_SET6:
    *Loc = (*Loc & 0xc0) | (Val & 0x3f);
    return;
  case R_RISCV_SET8:
    *Loc = Val;
    return;
  case R_RISCV_SET16:
    write16le(Loc, Val);
    return;
  case R_RISCV_SET32:
  case R_RISCV_32_PCREL:
    write32le(Loc, Val);
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

TargetInfo *elf::getRISCVTargetInfo() {
  static RISCV Target;
  return &Target;
}
