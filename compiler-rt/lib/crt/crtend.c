/* ===-- crtend.c - Provide .eh_frame --------------------------------------===
 *
 *      	       The LLVM Compiler Infrastructure
 *
 * This file is dual licensed under the MIT and the University of Illinois Open
 * Source Licenses. See LICENSE.TXT for details.
 *
 * ===----------------------------------------------------------------------===
 */

static const int __FRAME_END__[]
  __attribute__((section(".eh_frame"), aligned(4), used)) = {};
