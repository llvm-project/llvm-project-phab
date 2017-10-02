// RUN: %clang -target aarch64 -mgeneral-regs-only %s -o - -S -emit-llvm | FileCheck %s
// %clang -target aarch64 -mgeneral-regs-only %s -o - -S | FileCheck %s --check-prefix=ASM

// CHECK-LABEL: @j = constant i32 3

float arr[8];

// CHECK-LABEL: noimplicitfloat
// CHECK-NEXT: define void @foo
const int j = 1.0 + 2.0;
void foo(int *i) {
  // CHECK: store i32 4
  *i = 1.0 + j;
}

// CHECK-LABEL: noimplicitfloat
// CHECK-NEXT: define void @bar
void bar(float **j) {
  __builtin_memcpy(arr, j, sizeof(arr));
  *j = arr;
}

struct OneFloat { float i; };
struct TwoFloats { float i, j; };

static struct OneFloat oneFloat;
static struct TwoFloats twoFloats;

// CHECK-LABEL: testOneFloat
void testOneFloat(const struct OneFloat *o, struct OneFloat *f) {
  // These memcpys are necessary, so we don't generate FP ops in LLVM.
  // CHECK: @llvm.memcpy
  // CHECK: @llvm.memcpy
  *f = *o;
  oneFloat = *o;
}

// CHECK-LABEL: testTwoFloats
void testTwoFloats(const struct TwoFloats *o, struct TwoFloats *f) {
  // CHECK: @llvm.memcpy
  // CHECK: @llvm.memcpy
  *f = *o;
  twoFloats = *o;
}

// -mgeneral-regs-only implies that the compiler can't invent
// floating-point/vector instructions. The user should be allowed to do that,
// though.
//
// CHECK-LABEL: baz
// ASM-LABEL: baz:
void baz(float *i) {
  // CHECK: call float* asm sideeffect
  // ASM: fmov s1, #
  // ASM: fadd s0, s0, s1
  asm volatile("ldr s0, [%0]\n"
               "fmov s1, #1.00000000\n"
               "fadd s0, s0, s1\n"
               "str s0, [%0]\n"
               "ret\n"
               : "=r"(i)
               :
               : "s0", "s1");
}
