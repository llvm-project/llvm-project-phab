//===- AArch64ErrataFix.h ---------------------------------------*- C++ -*-===//
//
//                             The LLVM Linker
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLD_ELF_AARCH64ERRATAFIX_H
#define LLD_ELF_AARCH64ERRATAFIX_H

#include "lld/Common/LLVM.h"

#include <map>
#include <vector>

namespace lld {
namespace elf {

class Defined;
class InputSection;
class InputSectionDescription;
class OutputSection;
class Patch843419Section;

// Implementation of the -fix-cortex-a53-843419 which affects early revisions
// of the cortex-a53 when running in the AArch64 execution state. The erratum
// occurs when a certain sequence of instructions occur on a 4k page boundary.
// To workaround this problem the linker will scan for these sequences and will
// replace one of the instructions with a branch to a thunk called a patch that
// will execute the instruction and return to the instruction after the branch
// to the patch. The replacement of the instruction with a branch is
// sufficient to prevent the erratum.
class SectionPatcher {
public:
  SectionPatcher();

  // return true if Patches have been added to the OutputSections.
  bool create843419Fixes(ArrayRef<OutputSection *> OutputSections);

private:
  void patchInputSectionDescription(InputSectionDescription &ISD,
                                    std::vector<Patch843419Section *> &Patches);

  void insertPatches(InputSectionDescription &ISD,
                     std::vector<Patch843419Section *> &Patches);

  // A cache of the mapping symbols defined by the InputSecion sorted in order
  // of ascending value with redundant symbols removed. These describe
  // the ranges of code and data in an executable InputSection.
  std::map<InputSection *, std::vector<const Defined *>> SectionMap;
};

} // namespace elf
} // namespace lld

#endif
