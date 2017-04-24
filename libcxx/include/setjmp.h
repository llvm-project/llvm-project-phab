// -*- C++ -*-
//===--------------------------- setjmp.h ---------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP_SETJMP_H
#define _LIBCPP_SETJMP_H

/*
    setjmp.h synopsis

Macros:

    setjmp

Types:

    jmp_buf

void longjmp(jmp_buf env, int val);

*/

#include <__config>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#pragma GCC system_header
#endif

#if defined(_LIBCPP_HAS_NO_INCLUDE_NEXT)
#include _LIBCPP_INCLUDE_NEXT(setjmp.h)
#else
#include_next <setjmp.h>
#endif

#ifdef __cplusplus

#ifndef setjmp
#define setjmp(env) setjmp(env)
#endif

#endif // __cplusplus

#endif  // _LIBCPP_SETJMP_H
