//===-- sanitizer_fuchsia.h ------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------===//
//
// Fuchsia-specific sanitizer support.
//
//===---------------------------------------------------------------------===//
#ifndef SANITIZER_FUCHSIA_H
#define SANITIZER_FUCHSIA_H

#include "sanitizer_platform.h"
#if SANITIZER_FUCHSIA

#include "sanitizer_common.h"

#include <magenta/sanitizer.h>

namespace __sanitizer {

struct DataInfo;

void RenderStackFrame(InternalScopedString *buffer,
                      unsigned int frame_no, uptr pc);
void RenderDataInfo(InternalScopedString *buffer, const DataInfo *DI);

extern uptr MainThreadStackBase, MainThreadStackSize;
extern sanitizer_shadow_bounds_t ShadowBounds;

}  // namespace __sanitizer

#endif  // SANITIZER_FUCHSIA
#endif  // SANITIZER_FUCHSIA_H
