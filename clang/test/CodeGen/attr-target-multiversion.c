// RUN: %clang_cc1 -triple x86_64-linux-gnu -target-cpu x86-64 -emit-llvm %s -o - | FileCheck %s

//CHECK: @foo.ifunc = ifunc i8 (i{{[0-9]+}}), i8 (i{{[0-9]+}})* ()* @foo.resolver
//CHECK: @bar.ifunc = ifunc i8 (...), i8 (...)* ()* @bar.resolver
//CHECK: @baz.ifunc = ifunc i8 (), i8 ()* ()* @baz.resolver

__attribute__((target("arch=knl"))) char foo(int);

// This function causes emission of the resolver, check it.
// CHECK: define i8 (i{{[0-9]+}})* @foo.resolver() {
// CHECK: ret i8 (i{{[0-9]+}})* @foo.arch_knl
// AAACHECK: ret i8 (i{{[0-9]+}})* @foo.arch_knl
// AAACHECK: ret i8 (i{{[0-9]+}})* @foo.arch_skylake
// AAACHECK: ret i8 (i{{[0-9]+}})* @foo.arch_sse4.2
// AAACHECK: ret i8 (i{{[0-9]+}})* @foo.arch_amdfam10
// AAACHECK: ret i8 (i{{[0-9]+}})* @foo.arch_atom
// AAACHECK: ret i8 (i{{[0-9]+}})* @foo

// CHECK: define signext i8 @foo(
__attribute__((target("default"))) char foo(int i) {
  return 0 + i;
}

// CHECK: define signext i8 @user1()
// CHECK: call signext i8 @foo.ifunc(i{{[0-9]+}} 1)
char user1(){
  return foo(1);
}

// CHECK: define signext i8 @foo.sse4.2(
__attribute__((target("sse4.2"))) char foo(int i) {
  return 1 + i;
}

// CHECK: define signext i8 @user2()
// CHECK: call signext i8 @foo.ifunc(i{{[0-9]+}} 2)
char user2(){
  return foo(2);
}

// CHECK: define signext i8 @foo.arch_atom(
__attribute__((target("arch=atom"))) char foo(int i) {
  return 2 + i;
}

__attribute__((target("arch=amdfam10"))) char foo(int);

// CHECK: define signext i8 @user3()
// CHECK: call signext i8 @foo.ifunc(i{{[0-9]+}} 3)
char user3() {
  return foo(3);
}

__attribute__((target("arch=skylake"))) char foo(int);

__attribute__((target("arch=knl"))) char bar();

__attribute__((target("default"))) char bar();


// CHECK: define signext i8 @user4()
// CHECK: call signext i8 (...) @bar.ifunc()
char user4() {
  return bar();
}

// CHECK: declare i8 (...)* @bar.resolver(){{$}}

__attribute__((target("arch=knl"))) char baz(void);

__attribute__((target("default"))) char baz();


// CHECK: define signext i8 @user5()
// CHECK: call signext i8 @baz.ifunc()
char user5() {
  return baz();
}
// CHECK: declare i8 ()* @baz.resolver(){{$}}

// Function Declarations are generated later, so ensure that they
// actually exist.
// CHECK-DAG: declare signext i8 @foo.arch_knl
// CHECK-DAG: declare signext i8 @foo.arch_amdfam10
// CHECK-DAG: declare signext i8 @foo.arch_skylake
