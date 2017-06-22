/*===---------------------------- gcov.h ----------------------------------===*\
|*
|*                     The LLVM Compiler Infrastructure
|*
|* This file is distributed under the University of Illinois Open Source
|* License. See LICENSE.TXT for details.
|*
|*===----------------------------------------------------------------------===*|
|*
|* This file provides symbols for calls which are available to end-users when
|* using gcov(3).
|*
\*===----------------------------------------------------------------------===*/

#ifndef	COMPILER_RT_GCOV_H
#define	COMPILER_RT_GCOV_H

/*
 * - Write profile information/counters out to their appropriate GCDA files.
 * - Reset profile counters to zero.
 */
extern void __gcov_flush(void);

#endif
