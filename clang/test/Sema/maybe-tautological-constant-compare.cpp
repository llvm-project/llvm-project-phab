// RUN: %clang_cc1 -triple x86_64-linux-gnu -fsyntax-only -DLP64 -Wmaybe-tautological-constant-compare -DTEST -std=c++11  -verify %s
// RUN: %clang_cc1 -triple x86_64-linux-gnu -fsyntax-only -DLP64 -std=c++11 -verify %s
// RUN: %clang_cc1 -triple i386-linux-gnu -fsyntax-only -DILP32 -Wmaybe-tautological-constant-compare -DTEST -std=c++11 -verify %s
// RUN: %clang_cc1 -triple i386-linux-gnu -fsyntax-only -DILP32 -std=c++11 -verify %s

template <typename T> struct numeric_limits {
  static constexpr T max() noexcept { return T(~0); }
};

int value(void);

int main() {
  unsigned long ul = value();

  using T1 = unsigned int;
  T1 t1 = value();
  using T2 = unsigned int;

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

#if defined(TEST)
  if (t1 < numeric_limits<T1>::max())
    return 0;
  if (numeric_limits<T1>::max() >= t1) // expected-warning {{for the current target platform, comparison 4294967295 >= 'T1' (aka 'unsigned int') is always true}}
    return 0;
  if (t1 > numeric_limits<T1>::max()) // expected-warning {{for the current target platform, comparison 'T1' (aka 'unsigned int') > 4294967295 is always false}}
    return 0;
  if (numeric_limits<T1>::max() <= t1)
    return 0;
  if (t1 <= numeric_limits<T1>::max()) // expected-warning {{for the current target platform, comparison 'T1' (aka 'unsigned int') <= 4294967295 is always true}}
    return 0;
  if (numeric_limits<T1>::max() > t1)
    return 0;
  if (t1 >= numeric_limits<T1>::max())
    return 0;
  if (numeric_limits<T1>::max() < t1) // expected-warning {{for the current target platform, comparison 4294967295 < 'T1' (aka 'unsigned int') is always false}}
    return 0;
  if (t1 == numeric_limits<T1>::max())
    return 0;
  if (numeric_limits<T1>::max() != t1)
    return 0;
  if (t1 != numeric_limits<T1>::max())
    return 0;
  if (numeric_limits<T1>::max() == t1)
    return 0;

  if (t1 < numeric_limits<T2>::max())
    return 0;
  if (numeric_limits<T2>::max() >= t1) // expected-warning {{for the current target platform, comparison 4294967295 >= 'T1' (aka 'unsigned int') is always true}}
    return 0;
  if (t1 > numeric_limits<T2>::max()) // expected-warning {{for the current target platform, comparison 'T1' (aka 'unsigned int') > 4294967295 is always false}}
    return 0;
  if (numeric_limits<T2>::max() <= t1)
    return 0;
  if (t1 <= numeric_limits<T2>::max()) // expected-warning {{for the current target platform, comparison 'T1' (aka 'unsigned int') <= 4294967295 is always true}}
    return 0;
  if (numeric_limits<T2>::max() > t1)
    return 0;
  if (t1 >= numeric_limits<T2>::max())
    return 0;
  if (numeric_limits<T2>::max() < t1) // expected-warning {{for the current target platform, comparison 4294967295 < 'T1' (aka 'unsigned int') is always false}}
    return 0;
  if (t1 == numeric_limits<T2>::max())
    return 0;
  if (numeric_limits<T2>::max() != t1)
    return 0;
  if (t1 != numeric_limits<T2>::max())
    return 0;
  if (numeric_limits<T2>::max() == t1)
    return 0;
#else
  // expected-no-diagnostics

  if (t1 < numeric_limits<T1>::max())
    return 0;
  if (numeric_limits<T1>::max() >= t1)
    return 0;
  if (t1 > numeric_limits<T1>::max())
    return 0;
  if (numeric_limits<T1>::max() <= t1)
    return 0;
  if (t1 <= numeric_limits<T1>::max())
    return 0;
  if (numeric_limits<T1>::max() > t1)
    return 0;
  if (t1 >= numeric_limits<T1>::max())
    return 0;
  if (numeric_limits<T1>::max() < t1)
    return 0;
  if (t1 == numeric_limits<T1>::max())
    return 0;
  if (numeric_limits<T1>::max() != t1)
    return 0;
  if (t1 != numeric_limits<T1>::max())
    return 0;
  if (numeric_limits<T1>::max() == t1)
    return 0;

  if (t1 < numeric_limits<T2>::max())
    return 0;
  if (numeric_limits<T2>::max() >= t1)
    return 0;
  if (t1 > numeric_limits<T2>::max())
    return 0;
  if (numeric_limits<T2>::max() <= t1)
    return 0;
  if (t1 <= numeric_limits<T2>::max())
    return 0;
  if (numeric_limits<T2>::max() > t1)
    return 0;
  if (t1 >= numeric_limits<T2>::max())
    return 0;
  if (numeric_limits<T2>::max() < t1)
    return 0;
  if (t1 == numeric_limits<T2>::max())
    return 0;
  if (numeric_limits<T2>::max() != t1)
    return 0;
  if (t1 != numeric_limits<T2>::max())
    return 0;
  if (numeric_limits<T2>::max() == t1)
    return 0;
#endif

#if defined(ILP32) && defined(TEST)
  if (ul < numeric_limits<unsigned int>::max())
    return 0;
  if (numeric_limits<unsigned int>::max() >= ul) // expected-warning {{for the current target platform, comparison 4294967295 >= 'unsigned long' is always true}}
    return 0;
  if (ul > numeric_limits<unsigned int>::max()) // expected-warning {{for the current target platform, comparison 'unsigned long' > 4294967295 is always false}}
    return 0;
  if (numeric_limits<unsigned int>::max() <= ul)
    return 0;
  if (ul <= numeric_limits<unsigned int>::max()) // expected-warning {{for the current target platform, comparison 'unsigned long' <= 4294967295 is always true}}
    return 0;
  if (numeric_limits<unsigned int>::max() > ul)
    return 0;
  if (ul >= numeric_limits<unsigned int>::max())
    return 0;
  if (numeric_limits<unsigned int>::max() < ul) // expected-warning {{for the current target platform, comparison 4294967295 < 'unsigned long' is always false}}
    return 0;
  if (ul == numeric_limits<unsigned int>::max())
    return 0;
  if (numeric_limits<unsigned int>::max() != ul)
    return 0;
  if (ul != numeric_limits<unsigned int>::max())
    return 0;
  if (numeric_limits<unsigned int>::max() == ul)
    return 0;
#else
  if (ul < numeric_limits<unsigned int>::max())
    return 0;
  if (numeric_limits<unsigned int>::max() >= ul)
    return 0;
  if (ul > numeric_limits<unsigned int>::max())
    return 0;
  if (numeric_limits<unsigned int>::max() <= ul)
    return 0;
  if (ul <= numeric_limits<unsigned int>::max())
    return 0;
  if (numeric_limits<unsigned int>::max() > ul)
    return 0;
  if (ul >= numeric_limits<unsigned int>::max())
    return 0;
  if (numeric_limits<unsigned int>::max() < ul)
    return 0;
  if (ul == numeric_limits<unsigned int>::max())
    return 0;
  if (numeric_limits<unsigned int>::max() != ul)
    return 0;
  if (ul != numeric_limits<unsigned int>::max())
    return 0;
  if (numeric_limits<unsigned int>::max() == ul)
    return 0;
#endif

  return 1;
}
