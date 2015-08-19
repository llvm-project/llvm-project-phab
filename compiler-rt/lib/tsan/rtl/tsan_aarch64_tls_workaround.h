//===-- tsan_android_aarch64_tls_workaround.h -----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file is a part of ThreadSanitizer (TSan), a race detector.
//
// This file presents some macros
//===----------------------------------------------------------------------===//

#ifndef TSAN_AARCH64_TLS_WORKAROUND_H
#define TSAN_AARCH64_TLS_WORKAROUND_H

// This file implements a few macros
// For OSs that properly implement __thread, this will devolve to a no-op.
// For others (notably Android on AArch64), we get around the lack of support
// via using pthread calls internally to implement it.
#include "sanitizer_common/sanitizer_platform.h"

#if defined(__aarch64__) && SANITIZER_ANDROID
#define SANITIZER_TLS_WORKAROUND_NEEDED 1
#define GetFromTLS(tlsvar) get_from_tls_##tlsvar()

namespace __tsan {
extern "C" bool InitTLSWorkAround();
};

// Due to chicken-vs-egg decl issues, these are the real accessor functions.
// char &get_from_tls_cur_thread_placeholder() (used in tsan_rtl.cc)
//  __sanitizer_sigset_t &get_from_tls_emptset() (used in tsan_interceptors.cc)
//  __sanitizer_sigset_t &get_from_tls_oldset();

#else
#define GetFromTLS(tlsvar) tlsvar
#define SANITIZER_TLS_WORKAROUND_NEEDED 0
#endif // defined(__aarch64__) && SANITIZER_ANDROID

#endif
