//===-- sanitizer_mac_libcdep.cc ------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file is shared between various sanitizers' runtime libraries and
// implements OSX-specific functions.
//===----------------------------------------------------------------------===//

#include "sanitizer_platform.h"
#if SANITIZER_MAC
#include "sanitizer_mac.h"

#include <mach/mach.h>
#include <sys/mman.h>

namespace __sanitizer {

void RestrictMemoryToMaxAddress(uptr max_address) {
  uptr size_to_mmap = GetMaxVirtualAddress() + 1 - max_address;
  void *res = MmapFixedNoAccess(max_address, size_to_mmap, "high gap");
  CHECK(res != MAP_FAILED);
}

static void *DarwinMmapFixed(uptr fixed_addr, uptr size, int flags,
                             bool noaccess = false) {
  uptr PageSize = GetPageSizeCached();
  vm_address_t p = (vm_address_t)(fixed_addr & ~(PageSize - 1));
  size = RoundUpTo(size, PageSize);
  flags |= VM_FLAGS_FIXED | VM_MAKE_TAG(VM_MEMORY_ANALYSIS_TOOL);
  kern_return_t r = vm_allocate(mach_task_self(), &p, size, flags);
  if (r != KERN_SUCCESS) {
    Report(
        "ERROR: %s failed to allocate 0x%zx (%zd) bytes at address %zx "
        "(vm_allocate returned: 0x%zx)\n",
        SanitizerToolName, size, size, p, (uptr)r);
    return 0;
  }
  if (noaccess) {
    r = vm_protect(mach_task_self(), p, size, 0, VM_PROT_NONE);
    if (r != KERN_SUCCESS) {
      Report(
          "ERROR: %s failed to vm_protect 0x%zx (%zd) bytes at address %zx "
          "(vm_protect returned: 0x%zx)\n",
          SanitizerToolName, size, size, p, (uptr)r);
      return 0;
    }
  }
  return (void *)p;
}

void *MmapFixedNoReserve(uptr fixed_addr, uptr size, const char *name) {
  return DarwinMmapFixed(fixed_addr, size, 0);
}

void *MmapFixedNoReserveAllowOverwrite(uptr fixed_addr, uptr size,
                                       const char *name) {
  return DarwinMmapFixed(fixed_addr, size, VM_FLAGS_OVERWRITE);
}

void *MmapFixedNoAccess(uptr fixed_addr, uptr size, const char *name) {
  return DarwinMmapFixed(fixed_addr, size, 0, /*noaccess*/ true);
}

void *MmapFixedNoAccessAllowOverwrite(uptr fixed_addr, uptr size,
                                      const char *name) {
  return DarwinMmapFixed(fixed_addr, size, VM_FLAGS_OVERWRITE,
                         /*noaccess*/ true);
}

}  // namespace __sanitizer

#endif  // SANITIZER_MAC
