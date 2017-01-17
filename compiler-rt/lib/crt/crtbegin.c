/* ===-- crtbegin.c - Provide __dso_handle ---------------------------------===
 *
 *      	       The LLVM Compiler Infrastructure
 *
 * This file is dual licensed under the MIT and the University of Illinois Open
 * Source Licenses. See LICENSE.TXT for details.
 *
 * ===----------------------------------------------------------------------===
 */

#ifdef SHARED
__attribute__((visibility("hidden"))) void *__dso_handle = &__dso_handle;
#else
__attribute__((visibility("hidden"))) void *__dso_handle = (void *)0;
#endif
