// RUN: %clang_cc1 -fsyntax-only -Wmissing-braces -verify %s

#ifdef BE_THE_HEADER
#pragma clang system_header

typedef struct _foo {
    unsigned char Data[2];
} foo;

#define BAR { 0 }

#else

#define BE_THE_HEADER
#include __FILE__

int a[2][2] = { 0, 1, 2, 3 }; // expected-warning{{suggest braces}} expected-warning{{suggest braces}}

foo g = BAR; // should not show warnings

#endif
