// RUN: %clang -S -emit-llvm --target=aarch64 %s -o - | FileCheck %s
//
/*  Various contexts where type _Float16 can appear. The different check
    prefixes are due to different mangling on X86 and different calling
    convention on SystemZ. */

/*  Namespace */
namespace {
  _Float16 f1n;
  _Float16 f2n = 33.f16;
  _Float16 arr1n[10];
  _Float16 arr2n[] = { 1.2, 3.0, 3.e4 };
  const volatile _Float16 func1n(const _Float16 &arg) {
    return arg + f2n + arr1n[4] - arr2n[1];
  }
}

/* File */
_Float16 f1f;
_Float16 f2f = 32.4;
static _Float16 f3f = f2f;
_Float16 arr1f[10];
_Float16 arr2f[] = { -1.2, -3.0, -3.e4 };
_Float16 func1f(_Float16 arg);

/* Class */
class C1 {
  _Float16 f1c;
  static const _Float16 f2c;
  volatile _Float16 f3c;
public:
  C1(_Float16 arg) : f1c(arg), f3c(arg) { }
  _Float16 func1c(_Float16 arg ) {
    return f1c + arg;
  }
  static _Float16 func2c(_Float16 arg) {
    return arg * C1::f2c;
  }
};

/*  Template */
template <class C> C func1t(C arg) { return arg * 2.f16; }
template <class C> struct S1 {
  C mem1;
};
template <> struct S1<_Float16> {
  _Float16 mem2;
};

/* Local */
int main(void) {
  _Float16 f1l = 123e220q;
  _Float16 f2l = -0.f16;
  _Float16 f3l = 1.000976562;
  C1 c1(f1l);
  S1<_Float16> s1 = { 132.f16 };
  _Float16 f4l = func1n(f1l) + func1f(f2l) + c1.func1c(f3l) + c1.func2c(f1l) +
    func1t(f1l) + s1.mem2 - f1n + f2n;
#if (__cplusplus >= 201103L)
  auto f5l = -1.f16, *f6l = &f2l, f7l = func1t(f3l);
#endif
  _Float16 f8l = f4l++;
  _Float16 arr1l[] = { -1.f16, -0.f16, -11.f16 };
}


// CHECK-DAG: @f1f = global half 0xH0000, align 2
// CHECK-DAG: @f2f = global half 0xH500D, align 2
// CHECK-DAG: @arr1f = global [10 x half] zeroinitializer, align 2
// CHECK-DAG: @arr2f = global [3 x half] [half 0xHBCCD, half 0xHC200, half 0xHF753], align 2
// CHECK-DAG: @_ZZ4mainE2s1 = private unnamed_addr constant %struct.S1 { half 0xH5820 }, align 2
// CHECK-DAG: @_ZN12_GLOBAL__N_13f1nE = internal global half 0xH0000, align 2
// CHECK-DAG: @_ZN12_GLOBAL__N_13f2nE = internal global half 0xH5020, align 2
// CHECK-DAG: @_ZZ4mainE5arr1l = private unnamed_addr constant [3 x half] [half 0xHBC00, half 0xH8000, half 0xHC980], align 2
// CHECK-DAG: @_ZN12_GLOBAL__N_15arr1nE = internal global [10 x half] zeroinitializer, align 2
// CHECK-DAG: @_ZN12_GLOBAL__N_15arr2nE = internal global [3 x half] [half 0xH3CCD, half 0xH4200, half 0xH7753], align 2
// CHECK-DAG: @_ZN2C13f2cE = external constant half, align 2

// CHECK-DAG: define linkonce_odr void @_ZN2C1C2EDh(%class.C1* %this, half %arg)
// CHECK-DAG: define linkonce_odr half @_ZN2C16func1cEDh(%class.C1* %this, half %arg)
// CHECK-DAG: define linkonce_odr half @_ZN2C16func2cEDh(half %arg)
// CHECK-DAG: define linkonce_odr half @_Z6func1tIDhET_S0_(half %arg)

// CHECK-DAG: store half 0xH7C00, half* %f1l, align 2
// CHECK-DAG: store half 0xH8000, half* %f2l, align 2
// CHECK-DAG: store half 0xH3C01, half* %f3l, align 2

// CHECK-DAG: [[F4L:%[a-z0-9]+]] = load half, half* %f4l, align 2
// CHECK-DAG: [[INC:%[a-z0-9]+]] = fadd half [[F4L]], 0xH3C00
// CHECK-DAG: store half [[INC]], half* %f4l, align 2
