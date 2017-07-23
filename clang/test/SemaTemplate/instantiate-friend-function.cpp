// RUN: %clang_cc1 -fsyntax-only -verify -std=c++11 %s
// RUN: %clang_cc1 -S -triple %itanium_abi_triple -std=c++11 -emit-llvm %s -o - | FileCheck %s
// expected-no-diagnostics


// Instantiate friend function, pattern is at file level.


template<typename T> struct C01 {
  template<typename T1> friend void func_01(C01<T> &, T1);
  template<typename T1, typename T2> friend void func_01a(C01<T1> &, T2);
};

C01<int> c01;

void f_01() {
  func_01(c01, 0.0);
  func_01a(c01, 0.0);
}

template<typename T1> void func_01(C01<int> &, T1) {}
template<typename T1, typename T2> void func_01a(C01<T1> &, T2) {}

// void func_01<double>(C01<int>&, double)
// CHECK: define linkonce_odr void @_Z7func_01IdEvR3C01IiET_
//
// void func_01a<int, double>(C01<int>&, double)
// CHECK: define linkonce_odr void @_Z8func_01aIidEvR3C01IT_ET0_


template<typename T> struct C02 {
  template<typename T1> friend void func_02(const C02<T> &, T1) { T var; }
  template<typename T1, typename T2> friend void func_02a(const C02<T1> &, T2) { T var; }
  template<typename T1> friend constexpr unsigned func_02b(const C02<T> &, const T1 x) { return sizeof(T1); }
};

const C02<int> c02;

void f_02() {
  func_02(c02, 0.0);
  func_02a(c02, 0.0);
  static_assert(func_02b(c02, short(122)) == sizeof(short), "Invalid calculation");
  static_assert(func_02b(c02, 122L) == sizeof(long), "Invalid calculation");
}

// void func_02<double>(C02<int> const&, double)
// CHECK: define linkonce_odr void @_Z7func_02IdEvRK3C02IiET_
//
// void func_02a<int, double>(C02<int> const&, double)
// CHECK: define linkonce_odr void @_Z8func_02aIidEvRK3C02IT_ET0_


template<typename T> struct C03 {
  template<typename T1> friend void func_03(C03<T> &, T1);
  template<typename T1, typename T2> friend void func_03a(C03<T1> &, T2);
};

C03<int> c03;

void f_03() {
  func_03(c03, 0.0);
  func_03a(c03, 0.0);
}

template<typename T> struct C03A {
  template<typename T1> friend void func_03(C03<T> &, T1) { }
};
template<typename T> struct C03B {
  template<typename T1, typename T2> friend void func_03a(C03<T1> &, T2) { T var; }
};

C03A<int> c03a;
C03B<int> c03b;

// void func_03<double>(C03<int>&, double)
// CHECK: define linkonce_odr void @_Z7func_03IdEvR3C03IiET_  
//
// void func_03a<int, double>(C03<int>&, double)
// CHECK: define linkonce_odr void @_Z8func_03aIidEvR3C03IT_ET0_


// File level declaration, friend pattern.


template<typename T1> void func_10(T1 *x);
template<typename T1, typename T2> void func_10a(T1 *x, T2 *y);
template<typename T1> constexpr unsigned func_10b(const T1 x);
template<typename T1> constexpr unsigned func_10c(const T1 x);

template<typename T>
struct C10 {
  template<typename T1> friend void func_10(T1 *x) { T var; }
  template<typename T1, typename T2> friend void func_10a(T1 *x, T2 *y) { T var; }
  template<typename T1> friend constexpr unsigned func_10b(const T1 x) { return sizeof(T1); }
  template<typename T1> friend constexpr unsigned func_10c(const T1 x) { return sizeof(T); }
};

C10<int> v10;

void use_10(int *x) {
  func_10(x);
  func_10a(x, &x);
  static_assert(func_10b(short(122)) == sizeof(short), "Invalid calculation");
  static_assert(func_10b(122L) == sizeof(long), "Invalid calculation");
  static_assert(func_10c(short(122)) == sizeof(int), "Invalid calculation");
  static_assert(func_10c(122L) == sizeof(int), "Invalid calculation");
}

// void func_10<int>(int*)
// CHECK: define linkonce_odr void @_Z7func_10IiEvPT_
//
// void func_10a<int, int*>(int*, int**)
// CHECK: define linkonce_odr void @_Z8func_10aIiPiEvPT_PT0_


template<typename T>
struct C11 {
  template<typename T1> friend void func_11(T1 *x) { T var; }
  template<typename T1, typename T2> friend void func_11a(T1 *x, T2 *y) { T var; }
  template<typename T1> friend constexpr unsigned func_11b(const T1 x) { return sizeof(T1); }
  template<typename T1> friend constexpr unsigned func_11c(const T1 x) { return sizeof(T); }
};

C11<int> v11;

template<typename T> void func_11(T *x);
template<typename T1, typename T2> void func_11a(T1 *x, T2 *y);
template<typename T1> constexpr unsigned func_11b(const T1 x);
template<typename T1> constexpr unsigned func_11c(const T1 x);

void use_11(int *x) {
  func_11(x);
  func_11a(x, &x);
  static_assert(func_11b(short(123)) == sizeof(short), "Invalid calculation");
  static_assert(func_11b(123L) == sizeof(long), "Invalid calculation");
  static_assert(func_11c(short(123)) == sizeof(int), "Invalid calculation");
  static_assert(func_11c(123L) == sizeof(int), "Invalid calculation");
}

// void func_11<int>(int*)
// CHECK: define linkonce_odr void @_Z7func_11IiEvPT_
//
// void func_11a<int, int*>(int*, int**)
// CHECK: define linkonce_odr void @_Z8func_11aIiPiEvPT_PT0_


template<typename T>
struct C12 {
  template<typename T1> friend void func_12(T1 *x) { T var; }
  template<typename T1, typename T2> friend void func_12a(T1 *x, T2 *y) { T var; }
};

template<typename T> void func_12(T *x);
template<typename T1, typename T2> void func_12a(T1 *x, T2 *y);

void use_12(int *x) {
  func_12(x);
  func_12a(x, &x);
}

C12<int> v12;

// void func_12<int>(int*)
// CHECK: define linkonce_odr void @_Z7func_12IiEvPT_
//
// void func_12a<int, int*>(int*, int**)
// CHECK: define linkonce_odr void @_Z8func_12aIiPiEvPT_PT0_


template<typename T1> void func_13(T1 *x);
template<typename T1, typename T2> void func_13a(T1 *x, T2 *y);

template<typename T>
struct C13 {
  template<typename T1> friend void func_13(T1 *x) { T var; }
  template<typename T1, typename T2> friend void func_13a(T1 *x, T2 *y) { T var; }
};

void use_13(int *x) {
  func_13(x);
  func_13a(x, &x);
}

C13<int> v13;

// void func_13<int>(int*)
// CHECK: define linkonce_odr void @_Z7func_13IiEvPT_
//
// void func_13a<int, int*>(int*, int**)
// CHECK: define linkonce_odr void @_Z8func_13aIiPiEvPT_PT0_


template<typename T1> void func_14(T1 *x);
template<typename T1, typename T2> void func_14a(T1 *x, T2 *y);

void use_14(int *x) {
  func_14(x);
  func_14a(x, &x);
}

template<typename T>
struct C14 {
  template<typename T1> friend void func_14(T1 *x) { T var; }
  template<typename T1, typename T2> friend void func_14a(T1 *x, T2 *y) { T var; }
};

C14<int> v14;

// void func_14<int>(int*)
// CHECK: define linkonce_odr void @_Z7func_14IiEvPT_
//
// void func_14a<int, int*>(int*, int**)
// CHECK: define linkonce_odr void @_Z8func_14aIiPiEvPT_PT0_


template<typename T>
struct C15 {
  template<typename T1> friend void func_15(T1 *x) { T var; }
  template<typename T1, typename T2> friend void func_15a(T1 *x, T2 *y) { T var; }
  template<typename T1> friend constexpr unsigned func_15b(const T1 x) { return sizeof(T1); }
  template<typename T1> friend constexpr unsigned func_15c(const T1 x) { return sizeof(T); }
};

template<typename T1> void func_15(T1 *x);
template<typename T1, typename T2> void func_15a(T1 *x, T2 *y);
template<typename T1> constexpr unsigned func_15b(const T1 x);
template<typename T1> constexpr unsigned func_15c(const T1 x);

C15<int> v15;

void use_15(int *x) {
  func_15(x);
  func_15a(x, &x);
  static_assert(func_15b(short(122)) == sizeof(short), "Invalid calculation");
  static_assert(func_15b(122L) == sizeof(long), "Invalid calculation");
  static_assert(func_15c(short(122)) == sizeof(int), "Invalid calculation");
  static_assert(func_15c(122L) == sizeof(int), "Invalid calculation");
}

// void func_15<int>(int*)
// CHECK: define linkonce_odr void @_Z7func_15IiEvPT_
//
// void func_15a<int, int*>(int*, int**)
// CHECK: define linkonce_odr void @_Z8func_15aIiPiEvPT_PT0_


// File level declaration, friend pattern and declaration.


template<typename T1> void func_16(T1 *x);
template<typename T1, typename T2> void func_16a(T1 *x, T2 *y);
template<typename T1> constexpr unsigned func_16b(const T1 x);
template<typename T1> constexpr unsigned func_16c(const T1 x);

template<typename T>
struct C16a {
  template<typename T1> friend void func_16(T1 *x) { T var; }
  template<typename T1, typename T2> friend void func_16a(T1 *x, T2 *y) { T var; }
  template<typename T1> friend constexpr unsigned func_16b(const T1 x) { return sizeof(T1); }
  template<typename T1> friend constexpr unsigned func_16c(const T1 x) { return sizeof(T); }
};

C16a<int> v16a;

template<typename T>
struct C16b {
  template<typename T1> friend void func_16(T1 *x);
  template<typename T1, typename T2> friend void func_16a(T1 *x, T2 *y);
  template<typename T1> friend constexpr unsigned func_16b(const T1 x);
  template<typename T1> friend constexpr unsigned func_16c(const T1 x);
};

C16b<int> v16b;

void use_16(int *x) {
  func_16(x);
  func_16a(x, &x);
  static_assert(func_16b(short(122)) == sizeof(short), "Invalid calculation");
  static_assert(func_16b(122L) == sizeof(long), "Invalid calculation");
  static_assert(func_16c(short(122)) == sizeof(int), "Invalid calculation");
  static_assert(func_16c(122L) == sizeof(int), "Invalid calculation");
}

// void func_16<int>(int*)
// CHECK: define linkonce_odr void @_Z7func_16IiEvPT_
//
// void func_16a<int, int*>(int*, int**)
// CHECK: define linkonce_odr void @_Z8func_16aIiPiEvPT_PT0_


template<typename T1> void func_17(T1 *x);
template<typename T1, typename T2> void func_17a(T1 *x, T2 *y);

template<typename T>
struct C17b {
  template<typename T1> friend void func_17(T1 *x);
  template<typename T1, typename T2> friend void func_17a(T1 *x, T2 *y);
};

C17b<int> v17b;

void use_17(int *x) {
  func_17(x);
  func_17a(x, &x);
}

template<typename T>
struct C17a {
  template<typename T1> friend void func_17(T1 *x) { T var; }
  template<typename T1, typename T2> friend void func_17a(T1 *x, T2 *y) { T var; }
};

C17a<int> v17a;

// void func_17<int>(int*)
// CHECK: define linkonce_odr void @_Z7func_17IiEvPT_
//
// void func_17a<int, int*>(int*, int**)
// CHECK: define linkonce_odr void @_Z8func_17aIiPiEvPT_PT0_


template<typename T1> void func_18(T1 *x);
template<typename T1, typename T2> void func_18a(T1 *x, T2 *y);
template<typename T1> constexpr unsigned func_18b(const T1 x);
template<typename T1> constexpr unsigned func_18c(const T1 x);

template<typename T>
struct C18b {
  template<typename T1> friend void func_18(T1 *x);
  template<typename T1, typename T2> friend void func_18a(T1 *x, T2 *y);
  template<typename T1> friend constexpr unsigned func_18b(const T1 x);
  template<typename T1> friend constexpr unsigned func_18c(const T1 x);
  struct Inner {
    template<typename T1> friend void func_18(T1 *x) { T var; }
    template<typename T1, typename T2> friend void func_18a(T1 *x, T2 *y) { T var; }
    template<typename T1> friend constexpr unsigned func_18b(const T1 x) { return sizeof(T1); }
    template<typename T1> friend constexpr unsigned func_18c(const T1 x) { return sizeof(T); }
  };
};

C18b<int>::Inner v18b;

void use_18(int *x) {
  func_18(x);
  func_18a(x, &x);
  static_assert(func_18b(short(122)) == sizeof(short), "Invalid calculation");
  static_assert(func_18b(122L) == sizeof(long), "Invalid calculation");
  static_assert(func_18c(short(122)) == sizeof(int), "Invalid calculation");
  static_assert(func_18c(122L) == sizeof(int), "Invalid calculation");
}

// void func_18<int>(int*)
// CHECK: define linkonce_odr void @_Z7func_18IiEvPT_
//
// void func_18a<int, int*>(int*, int**)
// CHECK: define linkonce_odr void @_Z8func_18aIiPiEvPT_PT0_


template<typename T1> void func_19(T1 *x);
template<typename T1, typename T2> void func_19a(T1 *x, T2 *y);
template<typename T1> constexpr unsigned func_19b(const T1 x);
template<typename T1> constexpr unsigned func_19c(const T1 x);

template<typename T>
struct C19b {
  struct Inner {
    template<typename T1> friend void func_19(T1 *x) { T var; }
    template<typename T1, typename T2> friend void func_19a(T1 *x, T2 *y) { T var; }
    template<typename T1> friend constexpr unsigned func_19b(const T1 x) { return sizeof(T1); }
    template<typename T1> friend constexpr unsigned func_19c(const T1 x) { return sizeof(T); }
  };
  template<typename T1> friend void func_19(T1 *x);
  template<typename T1, typename T2> friend void func_19a(T1 *x, T2 *y);
  template<typename T1> friend constexpr unsigned func_19b(const T1 x);
  template<typename T1> friend constexpr unsigned func_19c(const T1 x);
};

C19b<int>::Inner v19b;

void use_19(int *x) {
  func_19(x);
  func_19a(x, &x);
  static_assert(func_19b(short(122)) == sizeof(short), "Invalid calculation");
  static_assert(func_19b(122L) == sizeof(long), "Invalid calculation");
  static_assert(func_19c(short(122)) == sizeof(int), "Invalid calculation");
  static_assert(func_19c(122L) == sizeof(int), "Invalid calculation");
}

// void func_19<int>(int*)
// CHECK: define linkonce_odr void @_Z7func_19IiEvPT_
//
// void func_19a<int, int*>(int*, int**)
// CHECK: define linkonce_odr void @_Z8func_19aIiPiEvPT_PT0_


template<typename T1> void func_20(T1 *x);
template<typename T1, typename T2> void func_20a(T1 *x, T2 *y);
template<typename T1> constexpr unsigned func_20b(const T1 x);
template<typename T1> constexpr unsigned func_20c(const T1 x);

template<typename T>
struct C20b {
  struct Inner {
    template<typename T1> friend void func_20(T1 *x);
    template<typename T1, typename T2> friend void func_20a(T1 *x, T2 *y);
    template<typename T1> friend constexpr unsigned func_20b(const T1 x);
    template<typename T1> friend constexpr unsigned func_20c(const T1 x);
  };
  template<typename T1> friend void func_20(T1 *x) { T var; }
  template<typename T1, typename T2> friend void func_20a(T1 *x, T2 *y) { T var; }
  template<typename T1> friend constexpr unsigned func_20b(const T1 x) { return sizeof(T1); }
  template<typename T1> friend constexpr unsigned func_20c(const T1 x) { return sizeof(T); }
};

C20b<int>::Inner v20b;

void use_20(int *x) {
  func_20(x);
  func_20a(x, &x);
  static_assert(func_20b(short(122)) == sizeof(short), "Invalid calculation");
  static_assert(func_20b(122L) == sizeof(long), "Invalid calculation");
  static_assert(func_20c(short(122)) == sizeof(int), "Invalid calculation");
  static_assert(func_20c(122L) == sizeof(int), "Invalid calculation");
}

// void func_20<int>(int*)
// CHECK: define linkonce_odr void @_Z7func_20IiEvPT_
//
// void func_20a<int, int*>(int*, int**)
// CHECK: define linkonce_odr void @_Z8func_20aIiPiEvPT_PT0_


template<typename T1> void func_21(T1 *x);
template<typename T1, typename T2> void func_21a(T1 *x, T2 *y);
template<typename T1> constexpr unsigned func_21b(const T1 x);
template<typename T1> constexpr unsigned func_21c(const T1 x);

template<typename T>
struct C21b {
  template<typename T1> friend void func_21(T1 *x) { T var; }
  template<typename T1, typename T2> friend void func_21a(T1 *x, T2 *y) { T var; }
  template<typename T1> friend constexpr unsigned func_21b(const T1 x) { return sizeof(T1); }
  template<typename T1> friend constexpr unsigned func_21c(const T1 x) { return sizeof(T); }
  struct Inner {
    template<typename T1> friend void func_21(T1 *x);
    template<typename T1, typename T2> friend void func_21a(T1 *x, T2 *y);
    template<typename T1> friend constexpr unsigned func_21b(const T1 x);
    template<typename T1> friend constexpr unsigned func_21c(const T1 x);
  };
};

C21b<int> v21b;

void use_21(int *x) {
  func_21(x);
  func_21a(x, &x);
  static_assert(func_21b(short(122)) == sizeof(short), "Invalid calculation");
  static_assert(func_21b(122L) == sizeof(long), "Invalid calculation");
  static_assert(func_21c(short(122)) == sizeof(int), "Invalid calculation");
  static_assert(func_21c(122L) == sizeof(int), "Invalid calculation");
}

// void func_21<int>(int*)
// CHECK: define linkonce_odr void @_Z7func_21IiEvPT_
//
// void func_21a<int, int*>(int*, int**)
// CHECK: define linkonce_odr void @_Z8func_21aIiPiEvPT_PT0_


template<typename T1> void func_22(T1 *x);
template<typename T1, typename T2> void func_22a(T1 *x, T2 *y);
template<typename T1> constexpr unsigned func_22b(const T1 x);
template<typename T1> constexpr unsigned func_22c(const T1 x);
template<typename T1> constexpr unsigned func_22d(const T1 x);

template<typename T>
struct C22b {
  template<typename T1> friend void func_22(T1 *x);
  template<typename T1, typename T2> friend void func_22a(T1 *x, T2 *y);
  template<typename T1> friend constexpr unsigned func_22b(const T1 x);
  template<typename T1> friend constexpr unsigned func_22c(const T1 x);
  template<typename T1> friend constexpr unsigned func_22d(const T1 x);
  template<typename TT>
  struct Inner {
    template<typename T1> friend void func_22(T1 *x) { T var; }
    template<typename T1, typename T2> friend void func_22a(T1 *x, T2 *y) { T var; }
    template<typename T1> friend constexpr unsigned func_22b(const T1 x) { return sizeof(T1); }
    template<typename T1> friend constexpr unsigned func_22c(const T1 x) { return sizeof(T); }
    template<typename T1> friend constexpr unsigned func_22d(const T1 x) { return sizeof(TT); }
  };
};

C22b<int>::Inner<char> v22b;

void use_22(int *x) {
  func_22(x);
  func_22a(x, &x);
  static_assert(func_22b(short(122)) == sizeof(short), "Invalid calculation");
  static_assert(func_22b(122L) == sizeof(long), "Invalid calculation");
  static_assert(func_22c(short(122)) == sizeof(int), "Invalid calculation");
  static_assert(func_22c(122L) == sizeof(int), "Invalid calculation");
  static_assert(func_22d(short(122)) == sizeof(char), "Invalid calculation");
  static_assert(func_22d(122L) == sizeof(char), "Invalid calculation");
}

// void func_22<int>(int*)
// CHECK: define linkonce_odr void @_Z7func_22IiEvPT_
//
// void func_22a<int, int*>(int*, int**)
// CHECK: define linkonce_odr void @_Z8func_22aIiPiEvPT_PT0_


namespace pr26512 {
struct A {
  template <class, bool> class B {
    template <class r_colony_allocator_type, bool r_is_const,
              class distance_type>
    friend void advance(B<r_colony_allocator_type, r_is_const> &,
                        distance_type);
  };
  template <class r_colony_allocator_type, bool r_is_const, class distance_type>
  friend void advance(B<r_colony_allocator_type, r_is_const> &, distance_type) {
    distance_type a;
  }
};
void main() {
  A::B<int, false> b;
  advance(b, 0);
}
}

// void pr26512::advance<int, false, int>(pr26512::A::B<int, false>&, int)
// CHECK: define linkonce_odr void @_ZN7pr265127advanceIiLb0EiEEvRNS_1A1BIT_XT0_EEET1_


namespace pr19095 {
template <class T> void f(T);

template <class U> class C { 
  template <class T> friend void f(T) {
     C<U> c;
     c.i = 3;
  }
  
public:
  void g() {
    f(3.0);
  }
  int i;
};

void main () {
  f(7);
  C<double> c;
  c.g();
}
}

// void pr19095::f<double>(double)
// CHECK: define linkonce_odr void @_ZN7pr190951fIdEEvT_
