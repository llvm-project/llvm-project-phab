// RUN: %clang_cc1 -triple x86_64-linux -fsyntax-only -verify -emit-llvm-only %s

struct S {
  __attribute__((target("arch=sandybridge")))
  void mv(){}
  __attribute__((target("arch=ivybridge")))
  void mv(){}
  __attribute__((target("default")))
  void mv(){}
};

// Note: Template attribute 'target' isn't implemented in GCC either, and would
// end up causing some nasty issues attempting it, so ensure that it still gives
// the same errors as without the attribute.
template<typename T>
__attribute__((target("arch=sandybridge")))
void mv_temp(){}

template<typename T>
__attribute__((target("arch=ivybridge")))
//expected-error@+2 {{redefinition of}}
//expected-note@-5{{previous definition is here}}
void mv_temp(){}

template<typename T>
__attribute__((target("default")))
void mv_temp(){}

void foo() {
  //expected-error@+2{{no matching function for call to}}
  //expected-note@-8{{candidate template ignored}}
  mv_temp<int>();
}
