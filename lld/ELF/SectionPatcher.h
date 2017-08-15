//===- SectionPatcher.h -----------------------------------------*- C++ -*-===//
//
//                             The LLVM Linker
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLD_ELF_SECTIONPATCHER_H
#define LLD_ELF_SECTIONPATCHER_H

#include "Relocations.h"
#include "lld/Core/LLVM.h"

namespace lld {
namespace elf {

class OutputSection;
class SymbolBody;
class InputSection;
class PatchSection;

// Class to describe an instance of a Patch.
// A Patch is a sequence of instructions that are a substitute for one or more
// instructions in an InputSection. The linker will insert a branch to the
// Patch at PatcheeOffset within the InputSection Patchee. This will transfer
// control to the contents of the Patch. The Patch is responsible for writing
// the return sequence, which is typically a branch to the next instruction
// after PatcheeOffset.
//
// FIXME: We only support one patch -fix-cortex-a53-843419, if support for
// multiple patches is needed we will need this will need to be generalized.
// FIXME: Patches have some similarity to Thunks, and it may be profitable to
// merge them or derive from a common base class if a Target needs both Thunks
// and Patches.
class Patch843419 {
public:
  Patch843419(InputSection *P, uint64_t Off);
  void writeTo(uint8_t *Buf, PatchSection &PS) const;
  void addSymbols(PatchSection &PS);

  uint64_t getLDSTAddr() const;
  Relocation transformRelocation(const Relocation &R) const;

  size_t size() const;

  // The Section we are patching.
  const InputSection *Patchee;
  // The Offset of the instruction in Target we are patching.
  uint64_t PatcheeOffset;
  // The Offset of the patch in PatchSection
  uint64_t Offset;
  // A label for the start of the Patch
  SymbolBody *PatchSym;
  uint64_t Alignment = 4;
};

void createA53Errata843419Fixes(ArrayRef<OutputSection *> OutputSections);

} // namespace elf
} // namespace lld

#endif
