// RUN: %clang_cc1 -triple aarch64-linux-eabi -std=c++11 -general-regs-only %s -verify -DBANNED=float -Wno-unused-value
// RUN: %clang_cc1 -triple aarch64-linux-eabi -std=c++11 -general-regs-only %s -verify -DBANNED=int '-DVECATTR=__attribute__((ext_vector_type(2)))' -Wno-unused-value
// RUN: %clang_cc1 -triple aarch64-linux-eabi -std=c++11 -general-regs-only %s -verify -DBANNED=FloatTypedef -Wno-unused-value

using FloatTypedef = float;

#ifndef VECATTR
#define VECATTR
#define BannedToInt(x) (x)
#else
#define BannedToInt(x) ((x)[0])
#endif

typedef BANNED BannedTy VECATTR;

namespace default_args {
int foo(BannedTy = 0); // expected-error 2{{use of floating-point or vector values is disabled}}
int bar(int = 1.0);

void baz(int a = foo()); // expected-note{{from use of default argument here}}
void bazz(int a = bar());

void test() {
  foo(); // expected-note{{from use of default argument here}}
  bar();
  baz(); // expected-note{{from use of default argument here}}
  baz(4);
  bazz();
}
}

namespace conversions {
struct ConvertToFloat { explicit operator BannedTy(); };
struct ConvertToFloatImplicit { /* implicit */ operator BannedTy(); };
struct ConvertFromFloat { ConvertFromFloat(BannedTy); };

void takeFloat(BannedTy);
void takeConvertFromFloat(ConvertFromFloat);
void takeConvertFromFloatRef(const ConvertFromFloat &);

void test() {
  BannedTy(ConvertToFloat());

  takeFloat(ConvertToFloatImplicit{}); // expected-error{{use of floating-point or vector values is disabled}}

  ConvertFromFloat(0); // expected-error{{use of floating-point or vector values is disabled}}
  ConvertFromFloat(ConvertToFloatImplicit{}); // expected-error{{use of floating-point or vector values is disabled}}

  ConvertFromFloat{0}; // expected-error{{use of floating-point or vector values is disabled}}
  ConvertFromFloat{ConvertToFloatImplicit{}}; // expected-error{{use of floating-point or vector values is disabled}}

  takeConvertFromFloat(0); // expected-error{{use of floating-point or vector values is disabled}}
  takeConvertFromFloatRef(0); // expected-error{{use of floating-point or vector values is disabled}}

  takeConvertFromFloat(ConvertFromFloat(0)); // expected-error{{use of floating-point or vector values is disabled}}
  takeConvertFromFloatRef(ConvertFromFloat(0)); // expected-error{{use of floating-point or vector values is disabled}}
}


void takeImplicitFloat(BannedTy = ConvertToFloatImplicit()); // expected-error{{use of floating-point or vector values is disabled}}
void test2() {
  takeImplicitFloat(); // expected-note{{from use of default argument here}}
}
}

namespace refs {
  struct BannedRef {
    const BannedTy &f;
    BannedRef(const BannedTy &f): f(f) {}
  };

  BannedTy &getBanned();
  BannedTy getBannedVal();

  void foo() {
    BannedTy &a = getBanned();
    BannedTy b = getBanned(); // expected-error{{use of floating-point or vector values is disabled}}
    const BannedTy &c = getBanned();
    const BannedTy &d = getBannedVal(); // expected-error{{use of floating-point or vector values is disabled}}

    const int &e = 1.0;
    const int &f = BannedToInt(getBannedVal()); // expected-error{{use of floating-point or vector values is disabled}}

    BannedRef{getBanned()};
    BannedRef{getBannedVal()}; // expected-error{{use of floating-point or vector values is disabled}}
  }
}

namespace class_init {
  struct Foo {
    float f = 1.0; // expected-error{{use of floating-point or vector values is disabled}}
    int i = 1.0;
    float j;

    Foo():
      j(1) // expected-error{{use of floating-point or vector values is disabled}}
    {}
  };
}

namespace copy_move_assign {
  struct Foo { float f; }; // expected-error 2{{use of floating-point or vector values is disabled}}

  void copyFoo(Foo &f) {
    Foo a = f; // expected-error{{use of floating-point or vector values is disabled}}
    Foo b(static_cast<Foo &&>(f)); // expected-error{{use of floating-point or vector values is disabled}}
    f = a; // expected-note{{in implicit copy assignment operator}}
    f = static_cast<Foo &&>(b); // expected-error{{use of floating-point or vector values is disabled}} expected-note {{in implicit move assignment operator}}
  }
}

namespace templates {
float bar();

template <typename T>
T foo(int t = bar()) { // expected-error 2{{use of floating-point or vector values is disabled}}
  return t; // expected-error{{use of floating-point or vector values is disabled}}
}

void test() {
  foo<float>(9); // expected-error{{use of floating-point or vector values is disabled}} expected-note{{in instantiation of function template specialization}}
  foo<float>(); // expected-error{{use of floating-point or vector values is disabled}} expected-note{{in instantiation of default function argument}} expected-note{{from use of default argument}}
}
}
