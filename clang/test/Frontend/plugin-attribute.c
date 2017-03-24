// RUN: %clang -fplugin=%llvmshlibdir/Attribute%pluginext -emit-llvm -S %s -o - | FileCheck %s --check-prefix=ATTRIBUTE
// RUN: not %clang -fplugin=%llvmshlibdir/Attribute%pluginext -emit-llvm -DBAD_ATTRIBUTE -S %s -o - 2>&1 | FileCheck %s --check-prefix=BADATTRIBUTE
// REQUIRES: plugins, examples

// ATTRIBUTE: [[STR1_VAR:@.+]] = private unnamed_addr constant [10 x i8] c"example()\00"
// ATTRIBUTE: [[STR2_VAR:@.+]] = private unnamed_addr constant [20 x i8] c"example(somestring)\00"
// ATTRIBUTE: [[STR3_VAR:@.+]] = private unnamed_addr constant [21 x i8] c"example(otherstring)\00"
// ATTRIBUTE: @llvm.global.annotations = {{.*}}@fn1{{.*}}[[STR1_VAR]]{{.*}}@fn2{{.*}}[[STR2_VAR]]{{.*}}@var1{{.*}}[[STR3_VAR]]
void fn1() __attribute__((example)) { }
void fn2() __attribute__((example("somestring"))) { }
int var1 __attribute__((example("otherstring"))) = 1;

#ifdef BAD_ATTRIBUTE
void fn3() {
  // BADATTRIBUTE: error: 'example' attribute only allowed at file scop
  int var2 __attribute__((example));
}
// BADATTRIBUTE: error: 'example' attribute requires a string
void fn4() __attribute__((example(123))) { }
// BADATTRIBUTE: error: 'example' attribute takes no more than 1 argument
void fn5() __attribute__((example("a","b"))) { }
#endif
