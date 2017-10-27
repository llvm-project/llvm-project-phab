// RUN: %clang_cc1 -triple x86_64-linux -fsyntax-only -verify -emit-llvm-only %s

struct S {
  __attribute__((target("arch=sandybridge")))
  void mv(){}
  __attribute__((target("arch=ivybridge")))
  void mv(){}
  __attribute__((target("default")))
  void mv(){}

  // NOTE: Virtual functions aren't implement for multiversioning in GCC either,
  // so this is a 'TBD' feature.
  // expected-error@+2 {{function multiversioning with 'target' doesn't support virtual functions yet}}
  __attribute__((target("arch=sandybridge")))
  virtual void mv_2(){}
  // expected-error@+3 {{class member cannot be redeclared}}
  // expected-note@-2 {{previous definition is here}}
  __attribute__((target("arch=ivybridge")))
  virtual void mv_2(){}
  // expected-error@+3 {{class member cannot be redeclared}}
  // expected-note@-6 {{previous definition is here}}
  __attribute__((target("default")))
  virtual void mv_2(){}
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
