// RUN: %clang_cc1 -triple x86_64-linux-gnu -fsyntax-only -DLP64 -Wmaybe-tautological-constant-compare -DTEST  -verify %s
// RUN: %clang_cc1 -triple x86_64-linux-gnu -fsyntax-only -DLP64 -verify %s
// RUN: %clang_cc1 -triple x86_64-linux-gnu -fsyntax-only -DLP64 -Wmaybe-tautological-constant-compare -DTEST -verify -x c++ %s
// RUN: %clang_cc1 -triple x86_64-linux-gnu -fsyntax-only -DLP64 -verify -x c++ %s
// RUN: %clang_cc1 -triple i386-linux-gnu -fsyntax-only -DILP32 -Wmaybe-tautological-constant-compare -DTEST -verify %s
// RUN: %clang_cc1 -triple i386-linux-gnu -fsyntax-only -DILP32 -verify %s
// R1UN: %clang_cc1 -triple i386-linux-gnu -fsyntax-only -DILP32 -Wmaybe-tautological-constant-compare -DTEST -verify -x c++ %s
// R1UN: %clang_cc1 -triple i386-linux-gnu -fsyntax-only -DILP32 -verify -x c++ %s

int value(void);

int main() {
  unsigned long ul = value();

#if defined(LP64)
#if __SIZEOF_INT__ != 4
#error Expecting 32-bit int
#endif
#if __SIZEOF_LONG__ != 8
#error Expecting 64-bit int
#endif
#elif defined(ILP32)
#if __SIZEOF_INT__ != 4
#error Expecting 32-bit int
#endif
#if __SIZEOF_LONG__ != 4
#error Expecting 32-bit int
#endif
#else
#error LP64 or ILP32 should be defined
#endif

#if defined(LP64)
  // expected-no-diagnostics

  if (ul < 4294967295)
    return 0;
  if (4294967295 >= ul)
    return 0;
  if (ul > 4294967295)
    return 0;
  if (4294967295 <= ul)
    return 0;
  if (ul <= 4294967295)
    return 0;
  if (4294967295 > ul)
    return 0;
  if (ul >= 4294967295)
    return 0;
  if (4294967295 < ul)
    return 0;
  if (ul == 4294967295)
    return 0;
  if (4294967295 != ul)
    return 0;
  if (ul != 4294967295)
    return 0;
  if (4294967295 == ul)
    return 0;

  if (ul < 4294967295U)
    return 0;
  if (4294967295U >= ul)
    return 0;
  if (ul > 4294967295U)
    return 0;
  if (4294967295U <= ul)
    return 0;
  if (ul <= 4294967295U)
    return 0;
  if (4294967295U > ul)
    return 0;
  if (ul >= 4294967295U)
    return 0;
  if (4294967295U < ul)
    return 0;
  if (ul == 4294967295U)
    return 0;
  if (4294967295U != ul)
    return 0;
  if (ul != 4294967295U)
    return 0;
  if (4294967295U == ul)
    return 0;

  if (ul < 4294967295L)
    return 0;
  if (4294967295L >= ul)
    return 0;
  if (ul > 4294967295L)
    return 0;
  if (4294967295L <= ul)
    return 0;
  if (ul <= 4294967295L)
    return 0;
  if (4294967295L > ul)
    return 0;
  if (ul >= 4294967295L)
    return 0;
  if (4294967295L < ul)
    return 0;
  if (ul == 4294967295L)
    return 0;
  if (4294967295L != ul)
    return 0;
  if (ul != 4294967295L)
    return 0;
  if (4294967295L == ul)
    return 0;

  if (ul < 4294967295UL)
    return 0;
  if (4294967295UL >= ul)
    return 0;
  if (ul > 4294967295UL)
    return 0;
  if (4294967295UL <= ul)
    return 0;
  if (ul <= 4294967295UL)
    return 0;
  if (4294967295UL > ul)
    return 0;
  if (ul >= 4294967295UL)
    return 0;
  if (4294967295UL < ul)
    return 0;
  if (ul == 4294967295UL)
    return 0;
  if (4294967295UL != ul)
    return 0;
  if (ul != 4294967295UL)
    return 0;
  if (4294967295UL == ul)
    return 0;
#elif defined(ILP32)
#if defined(TEST)
  if (ul < 4294967295)
    return 0;
  if (4294967295 >= ul) // expected-warning {{comparison 4294967295 >= 'unsigned long' is always true}}
    return 0;
  if (ul > 4294967295) // expected-warning {{comparison 'unsigned long' > 4294967295 is always false}}
    return 0;
  if (4294967295 <= ul)
    return 0;
  if (ul <= 4294967295) // expected-warning {{comparison 'unsigned long' <= 4294967295 is always true}}
    return 0;
  if (4294967295 > ul)
    return 0;
  if (ul >= 4294967295)
    return 0;
  if (4294967295 < ul) // expected-warning {{comparison 4294967295 < 'unsigned long' is always false}}
    return 0;
  if (ul == 4294967295)
    return 0;
  if (4294967295 != ul)
    return 0;
  if (ul != 4294967295)
    return 0;
  if (4294967295 == ul)
    return 0;

  if (ul < 4294967295U)
    return 0;
  if (4294967295U >= ul) // expected-warning {{with current data model, comparison 4294967295 >= 'unsigned long' is always true}}
    return 0;
  if (ul > 4294967295U) // expected-warning {{with current data model, comparison 'unsigned long' > 4294967295 is always false}}
    return 0;
  if (4294967295U <= ul)
    return 0;
  if (ul <= 4294967295U) // expected-warning {{with current data model, comparison 'unsigned long' <= 4294967295 is always true}}
    return 0;
  if (4294967295U > ul)
    return 0;
  if (ul >= 4294967295U)
    return 0;
  if (4294967295U < ul) // expected-warning {{with current data model, comparison 4294967295 < 'unsigned long' is always false}}
    return 0;
  if (ul == 4294967295U)
    return 0;
  if (4294967295U != ul)
    return 0;
  if (ul != 4294967295U)
    return 0;
  if (4294967295U == ul)
    return 0;

  if (ul < 4294967295L)
    return 0;
  if (4294967295L >= ul) // expected-warning {{comparison 4294967295 >= 'unsigned long' is always true}}
    return 0;
  if (ul > 4294967295L) // expected-warning {{comparison 'unsigned long' > 4294967295 is always false}}
    return 0;
  if (4294967295L <= ul)
    return 0;
  if (ul <= 4294967295L) // expected-warning {{comparison 'unsigned long' <= 4294967295 is always true}}
    return 0;
  if (4294967295L > ul)
    return 0;
  if (ul >= 4294967295L)
    return 0;
  if (4294967295L < ul) // expected-warning {{comparison 4294967295 < 'unsigned long' is always false}}
    return 0;
  if (ul == 4294967295L)
    return 0;
  if (4294967295L != ul)
    return 0;
  if (ul != 4294967295L)
    return 0;
  if (4294967295L == ul)
    return 0;

  if (ul < 4294967295UL)
    return 0;
  if (4294967295UL >= ul) // expected-warning {{comparison 4294967295 >= 'unsigned long' is always true}}
    return 0;
  if (ul > 4294967295UL) // expected-warning {{comparison 'unsigned long' > 4294967295 is always false}}
    return 0;
  if (4294967295UL <= ul)
    return 0;
  if (ul <= 4294967295UL) // expected-warning {{comparison 'unsigned long' <= 4294967295 is always true}}
    return 0;
  if (4294967295UL > ul)
    return 0;
  if (ul >= 4294967295UL)
    return 0;
  if (4294967295UL < ul) // expected-warning {{comparison 4294967295 < 'unsigned long' is always false}}
    return 0;
  if (ul == 4294967295UL)
    return 0;
  if (4294967295UL != ul)
    return 0;
  if (ul != 4294967295UL)
    return 0;
  if (4294967295UL == ul)
    return 0;
#else // defined(TEST)
  if (ul < 4294967295)
    return 0;
  if (4294967295 >= ul) // expected-warning {{comparison 4294967295 >= 'unsigned long' is always true}}
    return 0;
  if (ul > 4294967295) // expected-warning {{comparison 'unsigned long' > 4294967295 is always false}}
    return 0;
  if (4294967295 <= ul)
    return 0;
  if (ul <= 4294967295) // expected-warning {{comparison 'unsigned long' <= 4294967295 is always true}}
    return 0;
  if (4294967295 > ul)
    return 0;
  if (ul >= 4294967295)
    return 0;
  if (4294967295 < ul) // expected-warning {{comparison 4294967295 < 'unsigned long' is always false}}
    return 0;
  if (ul == 4294967295)
    return 0;
  if (4294967295 != ul)
    return 0;
  if (ul != 4294967295)
    return 0;
  if (4294967295 == ul)
    return 0;

  if (ul < 4294967295U)
    return 0;
  if (4294967295U >= ul)
    return 0;
  if (ul > 4294967295U)
    return 0;
  if (4294967295U <= ul)
    return 0;
  if (ul <= 4294967295U)
    return 0;
  if (4294967295U > ul)
    return 0;
  if (ul >= 4294967295U)
    return 0;
  if (4294967295U < ul)
    return 0;
  if (ul == 4294967295U)
    return 0;
  if (4294967295U != ul)
    return 0;
  if (ul != 4294967295U)
    return 0;
  if (4294967295U == ul)
    return 0;

  if (ul < 4294967295L)
    return 0;
  if (4294967295L >= ul) // expected-warning {{comparison 4294967295 >= 'unsigned long' is always true}}
    return 0;
  if (ul > 4294967295L) // expected-warning {{comparison 'unsigned long' > 4294967295 is always false}}
    return 0;
  if (4294967295L <= ul)
    return 0;
  if (ul <= 4294967295L) // expected-warning {{comparison 'unsigned long' <= 4294967295 is always true}}
    return 0;
  if (4294967295L > ul)
    return 0;
  if (ul >= 4294967295L)
    return 0;
  if (4294967295L < ul) // expected-warning {{comparison 4294967295 < 'unsigned long' is always false}}
    return 0;
  if (ul == 4294967295L)
    return 0;
  if (4294967295L != ul)
    return 0;
  if (ul != 4294967295L)
    return 0;
  if (4294967295L == ul)
    return 0;

  if (ul < 4294967295UL)
    return 0;
  if (4294967295UL >= ul) // expected-warning {{comparison 4294967295 >= 'unsigned long' is always true}}
    return 0;
  if (ul > 4294967295UL) // expected-warning {{comparison 'unsigned long' > 4294967295 is always false}}
    return 0;
  if (4294967295UL <= ul)
    return 0;
  if (ul <= 4294967295UL) // expected-warning {{comparison 'unsigned long' <= 4294967295 is always true}}
    return 0;
  if (4294967295UL > ul)
    return 0;
  if (ul >= 4294967295UL)
    return 0;
  if (4294967295UL < ul) // expected-warning {{comparison 4294967295 < 'unsigned long' is always false}}
    return 0;
  if (ul == 4294967295UL)
    return 0;
  if (4294967295UL != ul)
    return 0;
  if (ul != 4294967295UL)
    return 0;
  if (4294967295UL == ul)
    return 0;
#endif // defined(TEST)
#else
#error LP64 or ILP32 should be defined
#endif

  return 1;
}
