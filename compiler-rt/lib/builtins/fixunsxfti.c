/* ===-- fixunsxfti.c - Implement __fixunsxfti -----------------------------===
 *
 *      	       The LLVM Compiler Infrastructure
 *
 * This file is dual licensed under the MIT and the University of Illinois Open
 * Source Licenses. See LICENSE.TXT for details.
 *
 * ===----------------------------------------------------------------------===
 *
 * This file implements __fixunsxfti for the compiler_rt library.
 *
 * ===----------------------------------------------------------------------===
 */

#include "int_lib.h"

#ifdef CRT_HAS_128BIT

/* Returns: convert a to a unsigned long long, rounding toward zero.
 *          Negative values all become zero.
 */

/* Assumption:
 *  if __LDBL_MANT_DIG__ == 64, it's Intel 80-bit FP representation:
 *     long double is an intel 80 bit floating point type padded with 6 bytes:
 *     gggg gggg gggg gggg gggg gggg gggg gggg | gggg gggg gggg gggg seee eeee eeee eeee |
 *     1mmm mmmm mmmm mmmm mmmm mmmm mmmm mmmm | mmmm mmmm mmmm mmmm mmmm mmmm mmmm mmmm
 *  if __LDBL_MANT_DIG__ == 113, it's IEEE754 quad-precision floating point type:
 *      sign: bit 127, exp: bit 126 - 112, mantissa: bit 111 - 0.
 *  tu_int is a 128 bit integral type
 *  value in long double is representable in tu_int or is negative
 */


COMPILER_RT_ABI tu_int
__fixunsxfti(long double a)
{
    long_double_bits fb;
    fb.f = a;
#if __LDBL_MANT_DIG__ == 113
    // IEEE754 quad-precision floating point type.
    // sign: fb.u.high.s.high[31]
    // exp: fb.u.high.s.high[16:30]
    // mantissa: fb.u.high.s.high[0:15] | fb.u.high.s.low |  fb.u.low.all
    int isNeg = (fb.u.high.s.high >> 31);
    int e = (fb.u.high.s.high >> 16) - 16383;
    // clear the sign and exponent fields and set implicit one.
    tu_int r = (tu_int)((fb.u.high.s.high & 0xFFFF) | (1 << 16)) << 96;
    r |= (tu_int)fb.u.high.s.low << 64;
    r |= fb.u.low.all;
#elif __LDBL_MANT_DIG__ == 64
    int isNeg = fb.u.high.s.low & 0x00008000;
    int e = (fb.u.high.s.low & 0x00007FFF) - 16383;
    tu_int r = fb.u.low.all;
#else
#   error("Unsupported __LDBL_MANT_DIG__:" ## __LDBL_MANT_DIG__)
#endif
    if (e < 0 || isNeg)
        return 0;
    if ((unsigned)e > sizeof(tu_int) * CHAR_BIT)
        return ~(tu_int)0;
    if (e >= __LDBL_MANT_DIG__)
        r <<= (e - __LDBL_MANT_DIG__ + 1);
    else
        r >>= (__LDBL_MANT_DIG__ - 1 - e);
    return r;
}

#endif /* CRT_HAS_128BIT */
