// RUN: not %clang_cc1 -triple i686-pc-win32 %s -std=c++17 -emit-obj 2>&1 | FileCheck %s

template <bool> struct B;
template <class T> constexpr bool is_floating_point_v = false;

struct StrictNumeric {
  StrictNumeric(int);
  template <typename Dst, B<is_floating_point_v<Dst>> = nullptr> operator Dst();
};

static_assert(StrictNumeric(1) > 0);

// CHECK-NOT: cannot mangle this built-in __float128 type
// CHECK: invalid operands to binary expression
