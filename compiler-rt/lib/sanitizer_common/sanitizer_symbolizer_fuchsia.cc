//===-- sanitizer_symbolizer_fuchsia.cc -----------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file is shared between various sanitizers' runtime libraries.
//
// Implementation of Fuchsia-specific symbolizer.
//===----------------------------------------------------------------------===//

#include "sanitizer_platform.h"
#if SANITIZER_FUCHSIA

#include "sanitizer_fuchsia.h"
#include "sanitizer_symbolizer.h"

namespace __sanitizer {

// For Fuchsia we don't do any actual symbolization per se.
// Instead, we emit text containing raw addresses and raw linkage
// symbol names, embedded in Fuchsia's symbolization markup format.
// Fuchsia's logging infrastructure emits enough information about
// process memory layout that a post-processing filter can do the
// symbolization and pretty-print the markup.
// TODO(mcgrathr): URL to markup format document

// This is used by UBSan for type names, and by ASan for global variable names.
constexpr const char *kFormatDemangle = "{{{symbol:%s}}}";
constexpr uptr kFormatDemangleMax = 1024;  // Arbitrary.

// Function name or equivalent from PC location.
constexpr const char *kFormatFunction = "{{{pc:%p}}}";
constexpr uptr kFormatFunctionMax = 64; // More than big enough for 64-bit hex.

// Global variable name or equivalent from data memory address.
constexpr const char *kFormatData = "{{{data:%p}}}";

// One frame in a backtrace (printed on a line by itself).
constexpr const char *kFormatFrame = "{{{bt:%u:%p}}}";

// Just a trivial singleton.  It could be static if the generic
// Symbolizer class supported linker initialization.
Symbolizer *Symbolizer::GetOrInit() {
  SpinMutexLock l(&init_mu_);
  if (!symbolizer_) {
    symbolizer_ = new(symbolizer_allocator_) Symbolizer({});
    CHECK(symbolizer_);
  }
  return symbolizer_;
}

// Nothing to do.
void Symbolizer::LateInitialize() { }

// This is used by UBSan for type names, and by ASan for global variable names.
// It's expected to return a static buffer that will be reused on each call.
const char *Symbolizer::Demangle(const char *name) {
  static char buffer[kFormatDemangleMax];
  internal_snprintf(buffer, sizeof(buffer), kFormatDemangle, name);
  return buffer;
}

// This is used mostly for suppression matching.  Making it work
// would enable "interceptor_via_lib" suppressions.  It's also used
// once in UBSan to say "in module ..." in a message that also
// includes an address in the module, so post-processing can already
// pretty-print that so as to indicate the module.
bool Symbolizer::GetModuleNameAndOffsetForPC(uptr pc, const char **module_name,
                                             uptr *module_address) {
  return false;
}

// This is used in some places for suppression checking, which we
// don't really support for Fuchsia.  It's also used in UBSan to
// identify a PC location to a function name, so we always fill in
// the function member with a string containing markup around the PC
// value.
// TODO(mcgrathr): Under SANITIZER_GO, it's currently used by TSan
// to render stack frames, but that should be changed to use
// RenderStackFrame.
SymbolizedStack *Symbolizer::SymbolizePC(uptr addr) {
  SymbolizedStack *s = SymbolizedStack::New(addr);
  char buffer[kFormatFunctionMax];
  internal_snprintf(buffer, sizeof(buffer), kFormatFunction, addr);
  s->info.function = internal_strdup(buffer);
  return s;
}

// Always claim we succeeded, so that RenderDataInfo will be called.
bool Symbolizer::SymbolizeData(uptr addr, DataInfo *info) {
  info->Clear();
  info->start = addr;
  return true;
}

void RenderDataInfo(InternalScopedString *buffer, const DataInfo *DI) {
  buffer->append(kFormatData, DI->start);
}

void RenderStackFrame(InternalScopedString *buffer,
                      unsigned int frame_no, uptr pc) {
  buffer->append(kFormatFrame, frame_no, pc);
}

}  // namespace __sanitizer

#endif  // SANITIZER_FUCHSIA
