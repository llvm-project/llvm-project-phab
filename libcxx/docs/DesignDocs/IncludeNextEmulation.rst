=======================
Emulating #include_next
=======================
Why #include_next at all?
-------------------------
Libc++ does not come with a C library.  It is intended to work with many different C libraries.  The C++ standard also requires the <cname> wrappers around the C library headers [headers] and compatibility wrappers for the <name.h> headers [depr.c.headers].

The straightforward way to implement these wrappers is to have the C++ headers use the preprocessor extension #include_next in order to get access to the C version of the headers.  For example, the C++ <math.h> and <cmath> will need something like #include_next <math.h> in order to get the C version of math.h, rather than recursively include the C++ math.h.

Why emulate #include_next?
--------------------------
#include_next isn't part of the C++ standard.  It is a compiler extension that both GCC and Clang implement.  Notably, Microsoft Visual Studio 2017 and earlier don't implement it (Visual Studio 2017 is the most current version as of the writing of this document).  Libc++ still needs access to the underlying C libraries though.

How does one emulate #include_next?
-----------------------------------
Emulating #include_next requires extra knowledge about the file system layout of your headers.  Suppose you're file system was laid out this way...

::

 /
   libcxx/
     include/
       math.h
       cmath
   vc/
     include/
       math.h

We could then reach the vc/include/math.h include using a relative path include.

::

 #include <../../vc/include/math.h>

The "vc/include" portion of the #include makes it impossible to pull in the math.h from libcxx/include.

This has the obvious disadvantage of hard-coding C-library paths into multiple source code locations.  This can be solved by relying on implementation defined behavior of the preprocessor (C11, 6.10.2.4 ["Source file inclusion"]).  We will use what is referred to as a "computed include".

::

 #define _MY_INCLUDE_NEXT(header) <../../vc/include/header>
 #include _MY_INCLUDE_NEXT(math.h)

Libc++ goes an extra step with the computed includes.  __config_site.in allows those building libc++ to specify the relative path on the command line.

::

 #cmakedefine _MY_INCLUDE_NEXT(header) <@_MY_INCLUDE_NEXT@/header>

This works reasonably well for most headers.  <errno.h> is trouble though.  If the order of #includes is just right, then the token 'errno' will already be defined as a macro before we get to the #include _MY_INCLUDE_NEXT(errno.h) line.  In a normal #include <errno.h> or #include "errno.h", the 'errno' macro isn't expanded.  But in #include _MY_INCLUDE_NEXT(errno.h), the 'errno' macro will be expanded.  For this situation, we undef errno inside a #pragma push_macro / pop_macro block, and hope that we didn't overwrite a 'better' errno definition.

What is the right relative path for my version of Microsoft Visual Studio?
--------------------------------------------------------------------------
That depends on the header, and on the version of Microsoft Visual Studio.
My Visual Studio 2017 installation places most of the C headers in the "universal CRT":

::

 C:\Program Files (x86)\Windows Kits\10\Include\10.0.14393.0\ucrt

A few of the C headers (stdbool.h, limits.h, stdint.h, and setjmp.h) are found here though:

::

 C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\VC\Tools\MSVC\14.10.25017\include

In Visual Studio 2015, most of the C headers could still be found in the universal crt, but the remainders are here:

::

 C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\include

The universal crt was introduced in Microsoft Visual Studio 2015.  Prior to that, most of C headers could be found in the corresponding installation's VC\include directory.

Over the past three major versions of Microsoft Visual Studio, the locations for headers have changed three times.  Given that the path in Microsoft Visual Studio 2017 has a very specific version in the path (14.10.25017), it is reasonable to assume that the required relative path will change along with patches and updates.

Since the C headers in Microsoft Visual Studio 2017 aren't all located in one place, the Libc++ implementation uses multiple macros and multiple relative paths to find the right headers.  I have had some success with these paths:

::

 LIBCXX_INCLUDE_NEXT_VC=../../../MSVC/14.10.25017/include \
 LIBCXX_INCLUDE_NEXT_UCRT=../ucrt \


How does Libc++ emulate #include_next?
--------------------------------------
Each use of #include_next is guarded with a check of _LIBCPP_HAS_NO_INCLUDE_NEXT.  When include_next isn't present, we use the emulation macros instead, and either #include _LIBCPP_INCLUDE_NEXT_UCRT(name.h) or #include _LIBCPP_INCLUDE_NEXT_VC(name.h).

If the Microsoft Visual Studio headers change location again, in a way that doesn't put them all in the same location (e.g. all under ucrt), then more emulation macros will need to be added, possibly one macro per header.
