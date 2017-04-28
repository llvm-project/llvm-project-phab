// RUN: %clang_cc1 -fsyntax-only -Wmissing-braces -verify %s

#ifdef BE_THE_HEADER
#pragma clang system_header

typedef struct _GUID {
  unsigned Data1;
  unsigned short  Data2;
  unsigned short  Data3;
  unsigned char Data4[8];
} GUID;

#define REGISTRY_EXTENSION_GUID { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10  }

#else

#define BE_THE_HEADER
#include __FILE__

int a[2][2] = { 0, 1, 2, 3 }; // expected-warning{{suggest braces}} expected-warning{{suggest braces}}

GUID g = REGISTRY_EXTENSION_GUID; // should not show warnings

#endif
