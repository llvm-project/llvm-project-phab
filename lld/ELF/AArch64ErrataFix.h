//===- AArch64ErrataFix.h ---------------------------------------*- C++ -*-===//
//
//                             The LLVM Linker
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLD_ELF_SECTIONPATCHER_H
#define LLD_ELF_SECTIONPATCHER_H

#include "lld/Common/LLVM.h"

namespace lld {
namespace elf {

class OutputSection;

// Implementation of the -fix-cortex-a53-843419 which affects early revisions
// of the cortex-a53 when running in the AArch64 execution state.

// FIXME: Only detects and reports the presence of the instruction sequence that
// can trigger the erratum 843419.
void createA53Errata843419Fixes(ArrayRef<OutputSection *> OutputSections);

} // namespace elf
} // namespace lld

#endif
