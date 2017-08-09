//===- CallGraphSort.h ------------------------------------------*- C++ -*-===//
//
//                             The LLVM Linker
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLD_ELF_CALL_GRAPH_SORT_H
#define LLD_ELF_CALL_GRAPH_SORT_H

#include "InputSection.h"

#include "llvm/ADT/DenseMap.h"

namespace lld {
namespace elf {
llvm::DenseMap<const InputSectionBase *, int>
computeCallGraphProfileOrder();
}
}

#endif
