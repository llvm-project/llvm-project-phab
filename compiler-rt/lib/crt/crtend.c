/* ===-- crtend.c - Provide .eh_frame --------------------------------------===
 *
 *      	       The LLVM Compiler Infrastructure
 *
 * This file is dual licensed under the MIT and the University of Illinois Open
 * Source Licenses. See LICENSE.TXT for details.
 *
 * ===----------------------------------------------------------------------===
 */

#include <inttypes.h>

static const int32_t __FRAME_END__[]
    __attribute__((section(".eh_frame"), aligned(4), used)) = { 0 };
