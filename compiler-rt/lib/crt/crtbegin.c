/* ===-- crtbegin.c - Provide __dso_handle ---------------------------------===
 *
 *      	       The LLVM Compiler Infrastructure
 *
 * This file is dual licensed under the MIT and the University of Illinois Open
 * Source Licenses. See LICENSE.TXT for details.
 *
 * ===----------------------------------------------------------------------===
 */

__attribute__((visibility("hidden")))
#ifdef SHARED
  void *__dso_handle = &__dso_handle;
#else
  void *__dso_handle = (void *)0;
#endif

extern void __cxa_finalize(void *) __attribute__((weak));

static void __attribute__((used)) __do_fini() {
  static _Bool finalized;

  if (__builtin_expect(finalized, 0))
    return;

#ifdef SHARED
  if (__cxa_finalize)
    __cxa_finalize(__dso_handle);
#endif

  finalized = 1;
}

__attribute__((section(".fini_array"), used))
  void (*__fini)(void) = __do_fini;
