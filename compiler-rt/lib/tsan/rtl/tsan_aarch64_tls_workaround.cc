#include <cassert>

#include "sanitizer_common/sanitizer_atomic.h"
#include "sanitizer_common/sanitizer_libc.h"
#include "sanitizer_common/sanitizer_linux.h"
#include "sanitizer_common/sanitizer_platform_limits_posix.h"
#include "sanitizer_common/sanitizer_placement_new.h"
#include "sanitizer_common/sanitizer_stacktrace.h"
#include "interception/interception.h"

#include "tsan_interface.h"
#include "tsan_platform.h"
#include "tsan_rtl.h"
#include "interception/interception.h"
#include <dlfcn.h>

#if SANITIZER_TLS_WORKAROUND_NEEDED

namespace __tsan {

// This needs to be aligned(64)
struct TSanTLSHack {
  uptr tid;
  char cur_thread_placeholder[sizeof(ThreadState)] ALIGNED(64);
  __sanitizer_sigset_t emptyset, oldset;
};

static const int kPrimeNumber = 2053;
static TSanTLSHack* TLSHack[kPrimeNumber];

extern
TSanTLSHack *GetTSanTLSHack() {
  uptr tid = GetThreadSelf();
  uptr hash = tid % kPrimeNumber;

  if (TLSHack[hash] == 0ll) {
    TLSHack[hash] = (TSanTLSHack*)InternalAlloc(sizeof(TSanTLSHack));
    TLSHack[hash]->tid = tid;
    DPrintf("Thread %p at slot %p getting ThreadState %p\n",
            tid, hash, &(TLSHack[hash]->cur_thread_placeholder[0]));
    return TLSHack[hash];
  }
  CHECK_EQ(TLSHack[hash]->tid, tid);
  return TLSHack[hash];
}

char &get_from_tls_cur_thread_placeholder() {
  return GetTSanTLSHack()->cur_thread_placeholder[0];
};

__sanitizer_sigset_t &get_from_tls_emptyset() {
  return GetTSanTLSHack()->emptyset;
};

__sanitizer_sigset_t &get_from_tls_oldset() {
  return GetTSanTLSHack()->oldset;
};

} // namespace __tsan
#endif // SANITIZER_TLS_WORKAROUND_NEEDED
