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

#include "lld/Core/LLVM.h"

namespace lld {
namespace elf {

class OutputSection;

void createA53Errata843419Fixes(ArrayRef<OutputSection *> OutputSections);

} // namespace elf
} // namespace lld

#endif
