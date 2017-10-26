//===- RISCV.cpp ----------------------------------------------------------===//
//
//                             The LLVM Linker
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "Error.h"
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
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/Statistic.h"
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

namespace lld {
namespace elf {

template <class ELFT>
class RISCV final : public TargetInfo {
public:
  RISCV() {}
  RelExpr getRelExpr(uint32_t Type, const SymbolBody &S,
                     const uint8_t *Loc) const override;
  void relocateOne(uint8_t *Loc, uint32_t Type, uint64_t Val) const override;
};

// RISC-V instructions are stored as 16-bit parcels starting with lower bits;
// each parcel is stored according to implementation's endianness.
template <endianness E>
static void writeInsn32(uint8_t* const Buf, const uint32_t Insn) {
  write16<E>(Buf, Insn & 0xFFFF);
  write16<E>(Buf + 2, Insn >> 16);
}

template <endianness E>
static uint32_t readInsn32(const uint8_t* const Buf) {
  return read16<E>(Buf) | static_cast<uint32_t>(read16<E>(Buf + 2)) << 16;
}

template <class ELFT>
RelExpr RISCV<ELFT>::getRelExpr(const uint32_t Type, const SymbolBody &S,
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

// FIXME: Is this defined somewhere in llvm that we can use?
uint32_t bitsExtraction(const uint64_t V, const uint32_t Begin,
                        const uint32_t End, const uint32_t LShift) {
  return ((V >> End) & ((0xFFFFFFFF) >> (32 - (Begin - End + 1)))) << LShift;
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
    const uint16_t Inst = read16<E>(Loc) & 0xE383;
    const uint16_t Imm8 = bitsExtraction(Val, 8, 8, 12);
    const uint16_t Imm4_3 = bitsExtraction(Val, 4, 3, 10);
    const uint16_t Imm7_6 = bitsExtraction(Val, 7, 6, 5);
    const uint16_t Imm2_1 = bitsExtraction(Val, 2, 1, 3);
    const uint16_t Imm5 = bitsExtraction(Val, 5, 5, 2);
    write16<E>(Loc, Inst | Imm8 | Imm4_3 | Imm7_6 | Imm2_1 | Imm5);
    return;
  }

  case R_RISCV_RVC_JUMP: {
    checkInt<11>(Loc, static_cast<int64_t>(Val) >> 1, Type);
    checkAlignment<2>(Loc, Val, Type);
    const uint16_t Inst = read16<E>(Loc) & 0xE003;
    const uint16_t Imm11 = bitsExtraction(Val, 11, 11, 12);
    const uint16_t Imm4 = bitsExtraction(Val, 4, 4, 11);
    const uint16_t Imm9_8 = bitsExtraction(Val, 9, 8, 9);
    const uint16_t Imm10 = bitsExtraction(Val, 10, 10, 8);
    const uint16_t Imm6 = bitsExtraction(Val, 6, 6, 7);
    const uint16_t Imm7 = bitsExtraction(Val, 7, 7, 6);;
    const uint16_t Imm3_1 = bitsExtraction(Val, 3, 1, 3);
    const uint16_t Imm5 = bitsExtraction(Val, 5, 5, 2);
    write16<E>(Loc,
        Inst | Imm11 | Imm4 | Imm9_8 | Imm10 | Imm6 | Imm7 | Imm3_1 | Imm5);
    return;
  }

  case R_RISCV_RVC_LUI: {
    const int32_t Imm = ((Val + 0x800) >> 12);
    checkUInt<6>(Loc, Imm, Type);
    if (Imm == 0) { // `c.lui rd, 0` is illegal, convert to `c.li rd, 0`
      write16<E>(Loc, (read16<E>(Loc) & 0x0F83) | 0x4000);
    } else {
      const uint16_t Imm17 = bitsExtraction(Val + 0x800, 17, 17, 12);
      const uint16_t Imm16_12 = bitsExtraction(Val + 0x800, 16, 12, 2);
      write16<E>(Loc, (read16<E>(Loc) & 0xEF83) | Imm17 | Imm16_12);
    }
    return;
  }

  case R_RISCV_JAL: {
    checkInt<20>(Loc, static_cast<int64_t>(Val) >> 1, Type);
    checkAlignment<2>(Loc, Val, Type);

    const uint32_t Inst = readInsn32<E>(Loc) & 0xFFF;
    const uint32_t Imm20 = bitsExtraction(Val, 20, 20, 31);
    const uint32_t Imm10_1 = bitsExtraction(Val, 10, 1, 21);
    const uint32_t Imm11 = bitsExtraction(Val, 11, 11, 20);
    const uint32_t Imm19_12 = bitsExtraction(Val, 19, 12, 12);
    const uint32_t Imm20_1 = Imm20 | Imm10_1 | Imm11 | Imm19_12;

    writeInsn32<E>(Loc, Inst | Imm20_1);
    return;
  }

  case R_RISCV_BRANCH: {
    checkInt<12>(Loc, static_cast<int64_t>(Val) >> 1, Type);
    checkAlignment<2>(Loc, Val, Type);

    const uint32_t Inst = readInsn32<E>(Loc) & 0x1FFF07F;
    const uint32_t Imm12 = bitsExtraction(Val, 12, 12, 31);
    const uint32_t Imm10_5 = bitsExtraction(Val, 10, 5, 25);
    const uint32_t Imm4_1 = bitsExtraction(Val, 4, 1, 8);
    const uint32_t Imm11 = bitsExtraction(Val, 11, 11, 7);
    const uint32_t Imm12_1 = Imm12 | Imm10_5 | Imm4_1 | Imm11;

    writeInsn32<E>(Loc, Inst | Imm12_1);
    return;
  }
  // auipc + jalr pair
  case R_RISCV_CALL: {
    checkInt<32>(Loc, static_cast<int64_t>(Val), Type);
    const uint32_t Hi = Val + 0x800;
    const uint32_t Lo = Val - (Hi & 0xFFFFF000);
    writeInsn32<E>(Loc, (readInsn32<E>(Loc) & 0xFFF) | (Hi & 0xFFFFF000));
    writeInsn32<E>(Loc + 4,
      (readInsn32<E>(Loc + 4) & 0xFFFFF) | ((Lo & 0xFFF) << 20));
    return;
  }

  case R_RISCV_PCREL_HI20:
  case R_RISCV_HI20: {
    checkInt<32>(Loc, static_cast<int64_t>(Val), Type);
    const uint32_t Hi = Val + 0x800;
    writeInsn32<E>(Loc, (readInsn32<E>(Loc) & 0xFFF) | (Hi & 0xFFFFF000));
    return;
  }
  case R_RISCV_PCREL_LO12_I:
  case R_RISCV_LO12_I: {
    checkInt<32>(Loc, static_cast<int64_t>(Val), Type);
    const uint32_t Hi = Val + 0x800;
    const uint32_t Lo = Val - (Hi & 0xFFFFF000);
    writeInsn32<E>(Loc, (readInsn32<E>(Loc) & 0xFFFFF) | ((Lo & 0xFFF) << 20));
    return;
  }
  case R_RISCV_PCREL_LO12_S:
  case R_RISCV_LO12_S: {
    checkInt<32>(Loc, static_cast<int64_t>(Val), Type);
    const uint32_t Hi = Val + 0x800;
    const uint32_t Lo = Val - (Hi & 0xFFFFF000);
    const uint32_t Imm11_5 = bitsExtraction(Lo, 11, 5, 25);
    const uint32_t Imm4_0 = bitsExtraction(Lo, 4, 0, 7);
    writeInsn32<E>(Loc, (readInsn32<E>(Loc) & 0x1FFF07F) | Imm11_5 | Imm4_0);
    return;
  }

  case R_RISCV_ADD8:
    *Loc = *Loc + Val;
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
    *Loc = *Loc - Val;
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

 // These are handled by the dynamic linker
  case R_RISCV_RELATIVE:
  case R_RISCV_COPY:
  case R_RISCV_JUMP_SLOT:
    goto unimp;

  // GP-relative relocations are only produced after relaxation, which
  // we don't support for now
  case R_RISCV_GPREL_I:
  case R_RISCV_GPREL_S:
    goto unimp;

  case R_RISCV_ALIGN:
  case R_RISCV_RELAX:
    return; // Ignored (for now)
  case R_RISCV_NONE:
    return; // Do nothing
unimp:
  default:
    error(getErrorLocation(Loc) +
      "unimplemented relocation type: " + Twine(Type));
    return;
  }
}

template <class ELFT> TargetInfo *getRISCVTargetInfo() {
  static RISCV<ELFT> Target;
  return &Target;
}

template RISCV<ELF32LE>::RISCV();
template RISCV<ELF32BE>::RISCV();
template RISCV<ELF64LE>::RISCV();
template RISCV<ELF64BE>::RISCV();

template TargetInfo *getRISCVTargetInfo<ELF32LE>();
template TargetInfo *getRISCVTargetInfo<ELF32BE>();
template TargetInfo *getRISCVTargetInfo<ELF64LE>();
template TargetInfo *getRISCVTargetInfo<ELF64BE>();

} // namespace elf
} // namespace lld
