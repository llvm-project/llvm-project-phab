//===- SectionPatcher.cpp -------------------------------------------------===//
//
//                             The LLVM Linker
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "SectionPatcher.h"
#include "Config.h"
#include "LinkerScript.h"
#include "Memory.h"
#include "OutputSections.h"
#include "Relocations.h"
#include "Strings.h"
#include "SyntheticSections.h"
#include "Target.h"

#include "llvm/Support/Endian.h"
#include "llvm/Support/raw_ostream.h"
#include <algorithm>

using namespace llvm;
using namespace llvm::ELF;
using namespace llvm::object;
using namespace llvm::support::endian;

using namespace lld;
using namespace lld::elf;

// This file implements Section Patching for the purpose of working around
// errata in CPUs. The general principle is that an erratum sequence of one or
// more instructions is detected in the instruction stream, one of the
// instructions in the sequence is replaced with a branch to a patch sequence
// of replacement instructions. At the end of the replacement sequence the
// patch branches back to the instruction stream.

// This technique is only suitable for fixing an erratum when:
// - There is a set of necessary conditions required to trigger the erratum that
// can be detected at static link time.
// - There is a set of replacement instructions that can be used to remove at
// least one of the necessary conditions that trigger the erratum.
// - We can overwrite an instruction in the erratum sequence with a branch to
// the replacement sequence.
// - We can place the replacement sequence within range of the branch.

// FIXME:
// - The implementation here only supports one patch, the AArch64 Cortex-53
// errata 843419 that affects r0p0, r0p1, r0p2 and r0p4 versions of the core.
// To keep the initial version simple there is no generic support for multiple
// architectures or multiple patches.
//
// - At this stage the implementation only supports detection and not fixing,
// this is sufficient to test the decode and recognition of the erratum sequence

// Helper functions that decode AArch64 A64 instructions needed for the
// detection of the erratum sequence. The functions stand alone and can be
// reused outside the context of detecting the erratum sequence.
struct A64 {

  // ADRP 1 | immlo (2) | 1 | 0 0 0 0 | immhi (19) | Rd (5)
  static bool isADRP(uint32_t Instr) {
    return (Instr & 0x9f000000) == 0x90000000;
  }

  // Load and store bit patterns from ARMv8-A ARM ARM
  // Instructions appear in order of appearance starting from table in
  // C4.1.3 Loads and Stores

  // All loads and stores have 1 (in bit postion 27), 0 at 25.
  // | op0 x op1 (2) | 1 op2 0 op3 (2) | x | op4 (5) | xxxx | op5 (2) | x (10)
  static bool isLoadStoreClass(uint32_t Instr) {
    return (Instr & 0x0a000000) == 0x08000000;
  }

  // LDN/STN multiple no offset
  // 0 Q 00 | 1100 | 0 L 0 | 00000 | opcode (4) | size (2) | Rn (5) | Rt (5)
  // LDN/STN multiple post-indexed
  // 0 Q 00 | 1100 | 1 L 0 | Rm (5) | opcode (4) | size (2) | Rn (5) | Rt(5)
  // L == 0 for stores
  // RM != 11111 register offset
  // RM == 11111 immediate offset
  // opcode == 0010 ST1 4 registers
  // opcode == 0110 ST1 3 registers
  // opcode == 0111 ST1 1 register
  // opcode == 1010 ST1 2 registers
  static bool isST1MultipleOpcode(uint32_t Instr) {
    return (Instr & 0x0000f000) == 0x00002000 ||
           (Instr & 0x0000f000) == 0x00006000 ||
           (Instr & 0x0000f000) == 0x00007000 ||
           (Instr & 0x0000f000) == 0x0000a000;
  }

  static bool isST1Multiple(uint32_t Instr) {
    return (Instr & 0xbfff0000) == 0x0c000000 && isST1MultipleOpcode(Instr);
  }

  // Writes to Rn (writeback)
  static bool isST1MultiplePost(uint32_t Instr) {
    return (Instr & 0xbfe00000) == 0x0c800000 && isST1MultipleOpcode(Instr);
  }

  // LDN/STN single no offset
  // | 0 Q 00 1101 | 0 L R 0 | 0000 | opcode (3) | Size (2) | Rn (5) | Rt (5)
  // LDN/STN single post-indexed
  // | 0 Q 00 1101 | 1 L R Rm (5) | opcode (3) | Size (2) | Rn (5) | Rt (5)
  // L == 0 for stores
  // R == 0 for ST1 and ST3
  // opcode == 000 ST1 8-bit
  // opcode == 010 ST1 16-bit
  // opcode == 100 ST1 32 or 64-bit (Size determines which)
  static bool isST1SingleOpcode(uint32_t Instr) {
    return (Instr & 0x0040e000) == 0x00000000 ||
           (Instr & 0x0040e000) == 0x00004000 ||
           (Instr & 0x0040e000) == 0x00008000;
  }

  static bool isST1Single(uint32_t Instr) {
    return (Instr & 0xbfff0000) == 0x0d000000 && isST1SingleOpcode(Instr);
  }

  // Writes to Rn (writeback)
  static bool isST1SinglePost(uint32_t Instr) {
    return (Instr & 0xbfe00000) == 0x0d800000 && isST1SingleOpcode(Instr);
  }

  static bool isST1(uint32_t Instr) {
    return isST1Multiple(Instr) || isST1MultiplePost(Instr) ||
           isST1Single(Instr) || isST1SinglePost(Instr);
  }

  // Load/store exclusive
  // size (2) 00 | 1000 | o2 L o1 | Rs (5) | o0 | Rt2 (5) | Rn (5) | Rt (5)
  // L == 0 for Stores
  static bool isLoadStoreExclusive(uint32_t Instr) {
    return (Instr & 0x3f000000) == 0x08000000;
  }

  static bool isLoadExclusive(uint32_t Instr) {
    return (Instr & 0x3f400000) == 0x08400000;
  }

  // Load register literal
  // opc (2) 01 | 1 V 00 | imm19 | Rt (5)
  static bool isLoadLiteral(uint32_t Instr) {
    return (Instr & 0x3b000000) == 0x18000000;
  }

  // Load/store no-allocate pair
  // (offset)
  // | opc (2) 10 | 1 V 00 | 0 L | imm 7 | Rt2 (5) | Rn (5) | Rt (5)
  // L == 0 for stores.
  // Never writes to register
  static bool isSTNP(uint32_t Instr) {
    return (Instr & 0x3bc00000) == 0x28000000;
  }

  // Load/store register pair
  // (post-indexed)
  // | opc (2) 10 | 1 V 00 | 1 L | imm7 | Rt2 (5) | Rn (5) | Rt (5)
  // L == 0 for stores, V == 0 for Scalar, V == 1 for Simd/FP
  // Writes to Rn
  static bool isSTPPost(uint32_t Instr) {
    return (Instr & 0x3bc00000) == 0x28800000;
  }

  // (offset)
  // | opc (2) 10 | 1 V 01 | 0 L | imm7 | Rt2 (5) | Rn (5) | Rt (5)
  static bool isSTPOffset(uint32_t Instr) {
    return (Instr & 0x3bc00000) == 0x29000000;
  }

  // (pre-index)
  // | opc (2) 10 | 1 V 01 | 1 L | imm7 | Rt2 (5) | Rn (5) | Rt (5)
  // Writes to Rn
  static bool isSTPPre(uint32_t Instr) {
    return (Instr & 0x3fc00000) == 0x29800000;
  }

  static bool isSTP(uint32_t Instr) {
    return isSTPPost(Instr) || isSTPOffset(Instr) || isSTPPre(Instr);
  }

  // For Load and Store single register, Loads are derived from a combination
  // of the Size, V and Opc fields.
  static bool isLoadSingleRegister(uint32_t Instr) {
    uint32_t Size = (Instr >> 30) & 0xff;
    uint32_t V = (Instr >> 26) & 0x1;
    uint32_t Opc = (Instr >> 22) & 0x3;
    // Opc == 0 are all stores
    // Apart from Size == 00 (0), V == 1, Opc == 10 (2) which is a store and
    // Size == 11 (3), V == 0, Opc == 10 (2) which is prefetch
    // are loads or unallocated
    return Opc != 0 && !(Size == 0 && V == 1 && Opc == 2) &&
           !(Size == 3 && V == 0 && Opc == 2);
  }

  // Load/store register (unscaled immediate)
  // V == 0 for Scalar, V == 1 for Simd/FP
  // size (2) 11 | 1 V 00 | opc (2) 0 | imm9 | 0 0 | Rn (5) | Rt (5)
  static bool isLoadStoreUnscaled(uint32_t Instr) {
    return (Instr & 0x3b000c00) == 0x38000000;
  }

  // Load/store register (immediate post-indexed)
  // size (2) 11 | 1 V 00 | opc (2) 0 | imm9 | 0 1 | Rn (5) | Rt (5)
  static bool isLoadStoreImmediatePost(uint32_t Instr) {
    return (Instr & 0x3b200c00) == 0x38000400;
  }

  // Load/store register (unprivileged)
  // size (2) 11 | 1 V 00 | opc (2) 0 | imm9 | 1 0 | Rn (5) | Rt(5)
  static bool isLoadStoreUnpriv(uint32_t Instr) {
    return (Instr & 0x3b200c00) == 0x38000800;
  }

  // Load/store register (immediate pre-indexed)
  // size (2) 11 | 1 V 00 | opc (2) 0 | imm9 | 1 1 | Rn (5) | Rt (5)
  static bool isLoadStoreImmediatePre(uint32_t Instr) {
    return (Instr & 0x3b200c00) == 0x38000c00;
  }

  // Load/store register (register offset)
  // size (2) 11 | 1 V 00 | opc (2) 1 | Rm (5) | option (3) S | 1 0 | Rn | Rt
  static bool isLoadStoreRegisterOff(uint32_t Instr) {
    return (Instr & 0x3b200c00) == 0x38200800;
  }

  // Load/store register (unsigned immediate)
  // size (2) 11 | 1 V 01 | opc (2) | imm12 | Rn (5) | Rt (5)
  static bool isLoadStoreRegisterUnsigned(uint32_t Instr) {
    return (Instr & 0x3b000000) == 0x39000000;
  }

  // Rt is always in bit position 0 - 4
  static uint32_t getRt(uint32_t Instr) { return (Instr & 0x1f); }

  // Rn is always in bit position 5 - 9
  static uint32_t getRn(uint32_t Instr) { return (Instr >> 5) & 0x1f; }

  // C4.1.2 Branches, Exception Generating and System instructions
  // | op0 (3) | 101 | op1 (4) | x (22)
  // op0 == 010 op1 == 0xxx Conditional Branch
  // op0 == 110 op1 == 1xxx Unconditional Branch Register
  // op0 == x00 op1 == xxxx Unconditional Branch immediate
  // op0 == x01 op1 == 0xxx Compare and branch immediate
  // op0 == x01 op1 == 1xxx Test and branch immediate
  static bool isBranch(uint32_t Instr) {
    return ((Instr & 0xfe000000) == 0x54000000) ||
           ((Instr & 0xfe000000) == 0xd6000000) ||
           ((Instr & 0x7c000000) == 0x14000000) ||
           ((Instr & 0x5c000000) == 0x14000000);
  }

  static bool isV8SingleRegisterLoadStore(uint32_t Instr) {
    return isLoadStoreUnscaled(Instr) || isLoadStoreImmediatePost(Instr) ||
           isLoadStoreUnpriv(Instr) || isLoadStoreImmediatePre(Instr) ||
           isLoadStoreRegisterOff(Instr) || isLoadStoreRegisterUnsigned(Instr);
  }

  // Note that this is V8 and does not include additional instructions for
  // later revisions of the architecture such as the Load/store exclusive
  // instructions introduced in v8.1
  static bool isV8Load(uint32_t Instr) {
    if (A64::isLoadExclusive(Instr))
      return true;
    if (A64::isLoadLiteral(Instr))
      return true;
    else if (A64::isV8SingleRegisterLoadStore(Instr))
      return A64::isLoadSingleRegister(Instr);
    return false;
  }
};

// Scanner for Cortex-A53 errata 843419
// Full details are available in the Cortex A53 MPCore revision 0 Software
// Developers Errata Notice (ARM-EPM-048406).
//
// The instruction sequence is common in compiled AArch64 code, however it is
// sensitive to address, which limits the number of times it has to be applied
// and limits the amount of disassembly that we have to do.
// 
// The erratum conditions are in summary:
// 1.) An ADRP instruction that writes to register Rn with low 12 bits of
//     address of instruction either 0xff8 or 0xffc
// 2.) A load or store instruction that can be:
// - A single register load or store, of either integer or vector registers
// - An STP or STNP, of either integer or vector registers
// - An Advanced SIMD ST1 store instruction
// - Must not write to Rn, but may optionally read from it.
// 3.) An optional instruction that is not a branch and does not write to Rn
// 4.) A load or store from the  Load/store register (unsigned immediate) class
//     that uses Rn as the base address register
//
// Note that we do not attempt to scan for Sequence 2 as described in the
// Software Developers Errata Notice as this has been assessed to be extremely
// unlikely to occur in compiled code. This matches gold and ld.bfd behavior.

// The following decode instructions are only complete up to the instructions
// needed for errata 843419.

// Writeback updates the index register after the load/store
static bool hasWriteback(uint32_t Instr) {
  return A64::isLoadStoreImmediatePre(Instr) ||
         A64::isLoadStoreImmediatePost(Instr) || A64::isSTPPre(Instr) ||
         A64::isSTPPost(Instr) || A64::isST1SinglePost(Instr) ||
         A64::isST1MultiplePost(Instr);
}

// For the load and store class of instructions, a load can write to the
// destination register, a load and a store can write to the base register when
// the instruction has writeback
static bool LoadStoreWritesToReg(uint32_t Instr, uint32_t Reg) {
  return (A64::isV8Load(Instr) && A64::getRt(Instr) == Reg) ||
         (hasWriteback(Instr) && A64::getRn(Instr) == Reg);
}

// Pre conditions:
// Adrp corresponds to 1.) and must be an ADRP instruction
// Instr2 corresponds to 2.) in the erratum description.
// The optional non branch instruction 3.) is not passed in, it must be filtered
// by the caller.
// Instr4 corresponds to 4.) in the erratum description.
// Post conditions:
// Returns true if the instruction sequence starting at Adrp may trigger the
// the erratum.
static bool is843419ErratumSequence(uint32_t Adrp, uint32_t Instr2,
                                    uint32_t Instr4) {
  uint32_t Rn = A64::getRt(Adrp);

  return A64::isLoadStoreClass(Instr2) &&
         (A64::isLoadStoreExclusive(Instr2) || A64::isLoadLiteral(Instr2) ||
          A64::isV8SingleRegisterLoadStore(Instr2) || A64::isSTP(Instr2) ||
          A64::isSTNP(Instr2) || A64::isST1(Instr2)) &&
         !LoadStoreWritesToReg(Instr2, Rn) &&
         A64::isLoadStoreRegisterUnsigned(Instr4) && A64::getRn(Instr4) == Rn;
}

static void report843419Fix(uint64_t AdrpAddr) {
  if (!Config->PrintFixes)
    return;
  message("detected cortex-a53-843419 erratum sequence starting at " +
          utohexstr(AdrpAddr) + " in unpatched output.");
}

static void scanCortexA53Errata843419(InputSection *IS, uint64_t &Off,
                                      uint64_t Size) {
  uint64_t ISAddr = IS->getParent()->Addr + IS->OutSecOff;
  const uint8_t *Buf = IS->Data.begin();

  // Skip to next address that is 0xff8 modulo 0x1000
  uint64_t CurPageOff = (ISAddr + Off) & 0xfff;
  if (CurPageOff < 0xff8)
    Off += 0xff8 - CurPageOff;

  bool OptionalAllowed = Size - Off > 12;
  if (Off >= Size || Size - Off < 12) {
    // Need at least 3 instructions to detect sequence
    Off = Size;
    return;
  }

  uint32_t Instr1 = *reinterpret_cast<const uint32_t *>(Buf + Off);
  if (A64::isADRP(Instr1)) {
    uint32_t Instr2 = *reinterpret_cast<const uint32_t *>(Buf + Off + 4);
    uint32_t Instr3 = *reinterpret_cast<const uint32_t *>(Buf + Off + 8);
    if (is843419ErratumSequence(Instr1, Instr2, Instr3))
      report843419Fix(ISAddr + Off);
    else if (OptionalAllowed && !A64::isBranch(Instr3)) {
      uint32_t Instr4 = *reinterpret_cast<const uint32_t *>(Buf + Off + 12);
      if (is843419ErratumSequence(Instr1, Instr2, Instr4))
        report843419Fix(ISAddr + Off);
    }
  }
  if (((ISAddr + Off) & 0xfff) == 0xff8)
    Off += 4;
  else
    // Skip to next 0xff8
    Off += 0xffc;
}

// The AArch64 ABI permits data in executable sections. We must avoid scanning
// this data as if it were data to avoid false matches and patches within data
// The ABI Section 4.5.4 Mapping symbols defines local symbols that describe
// a half open intervals [Symbol Value, Next Symbol Value) of code and data
// within sections. If there is no next symbol then the half open interval is
// [Symbol Value, End of section). The type, code or data, is determined by the
// mapping symbol name.
std::map<InputSection *,
         std::vector<const DefinedRegular *>> static makeAArch64SectionMap() {
  std::map<InputSection *, std::vector<const DefinedRegular *>> SectionMap;
  auto IsCodeMapSymbol = [](const SymbolBody *B) {
    return B->getName() == "$x" || B->getName().startswith("$x.");
  };
  auto IsDataMapSymbol = [](const SymbolBody *B) {
    return B->getName() == "$d" || B->getName().startswith("$d.");
  };

  // Collect mapping symbols per executable InputSection
  for (ObjFile<ELF64LE> *F : ObjFile<ELF64LE>::Instances) {
    for (SymbolBody *B : F->getLocalSymbols()) {
      auto *DR = dyn_cast<DefinedRegular>(B);
      if (!DR)
        continue;
      if (!IsCodeMapSymbol(DR) && !IsDataMapSymbol(DR))
        continue;
      if (auto *Sec = dyn_cast<InputSection>(DR->Section)) {
        if (Sec->Flags & SHF_EXECINSTR)
          SectionMap[Sec].push_back(DR);
      }
    }
  }
  // There is no guarantee that the mapping symbols are in ascending order
  // although in practice they frequently are. If there is more than one
  // mapping symbol for a section make sure they are sorted in ascending Value,
  // and free from consecutive runs of mapping symbols with the same type. For
  // example $x.0 $d.0 $d.1 $x.1 is the same as $x.0 $d.0 $x.1
  for (auto &KV : SectionMap) {
    std::vector<const DefinedRegular *> &MapSyms = KV.second;
    if (MapSyms.size() > 1) {
      std::stable_sort(MapSyms.begin(), MapSyms.end(),
                       [](const DefinedRegular *A, const DefinedRegular *B) {
                         return A->Value < B->Value;
                       });
      MapSyms.erase(
          std::unique(MapSyms.begin(), MapSyms.end(),
                      [=](const DefinedRegular *A, const DefinedRegular *B) {
                        return (IsCodeMapSymbol(A) && IsCodeMapSymbol(B)) ||
                               (IsDataMapSymbol(A) && IsDataMapSymbol(B));
                      }),
          MapSyms.end());
    }
  }
  return SectionMap;
}

// Scan all the executable code in an AArch64 link to detect the Cortex-A53
// erratum 843419.
// FIXME: The current implementation only scans for the erratum sequence, it
// does not attempt to fix it.
void lld::elf::createA53Errata843419Fixes(
    ArrayRef<OutputSection *> OutputSections) {
  std::map<InputSection *, std::vector<const DefinedRegular *>> SectionMap =
      makeAArch64SectionMap();

  for (OutputSection *OS : OutputSections) {
    if (!(OS->Flags & SHF_ALLOC) || !(OS->Flags & SHF_EXECINSTR))
      continue;
    for (BaseCommand *BC : OS->Commands)
      if (auto *ISD = dyn_cast<InputSectionDescription>(BC)) {
        for (InputSection *IS : ISD->Sections) {
          //  LLD doesn't use the erratum sequence in SyntheticSections
          if (isa<SyntheticSection>(IS))
            continue;
          // Use SectionMap to make sure we only scan code and not inline data.
          std::vector<const DefinedRegular *> &MapSyms = SectionMap[IS];

          auto Code = llvm::find_if(MapSyms, [&](const DefinedRegular *MS) {
            return MS->getName().startswith("$x");
          });

          while (Code != MapSyms.end()) {
            auto Data = std::next(Code);
            uint64_t Off = (*Code)->Value;
            uint64_t Limit =
                (Data == MapSyms.end()) ? IS->Data.size() : (*Data)->Value;

            while (Off < Limit)
              scanCortexA53Errata843419(IS, Off, Limit);
            if (Data == MapSyms.end())
              break;
            Code = std::next(Data);
          }
        }
      }
  }
}
