// RUN: %clang_cc1 -triple x86_64-linux -fsyntax-only -verify -emit-llvm-only %s
// RUN: %clang_cc1 -triple x86_64-linux -fsyntax-only -verify -emit-llvm-only -DCHECK_DEFAULT %s

#if defined(CHECK_DEFAULT)
__attribute__((target("arch=sandybridge")))
//expected-error@+1 {{function multiversioning with 'target' requires a 'default' implementation}}
void no_default(){}
__attribute__((target("arch=ivybridge")))
void no_default(){}
#else
// Only multiversioning causes issues with redeclaration changing 'target'.
__attribute__((target("arch=sandybridge")))
void fine_since_no_mv();
void fine_since_no_mv();

void also_fine_since_no_mv();
__attribute__((target("arch=sandybridge")))
void also_fine_since_no_mv();

__attribute__((target("arch=sandybridge")))
void also_fine_since_no_mv2();
__attribute__((target("arch=sandybridge")))
void also_fine_since_no_mv2();
void also_fine_since_no_mv2();

__attribute__((target("arch=sandybridge")))
void mv(){}
__attribute__((target("arch=ivybridge")))
void mv(){}
__attribute__((target("default")))
void mv(){}

void redecl_causes_mv();
__attribute__((target("arch=sandybridge")))
void redecl_causes_mv();
// expected-error@+3 {{function redeclaration declares a multiversioned function, but a previous declaration lacks a 'target' attribute}}
// expected-note@-4 {{previous declaration is here}}
__attribute__((target("arch=ivybridge")))
void redecl_causes_mv();

__attribute__((target("arch=sandybridge")))
void redecl_causes_mv2();
void redecl_causes_mv2();
// expected-error@+3 {{function redeclaration declares a multiversioned function, but a previous declaration lacks a 'target' attribute}}
// expected-note@-2 {{previous declaration is here}}
__attribute__((target("arch=ivybridge")))
void redecl_causes_mv2();

__attribute__((target("arch=sandybridge")))
void redecl_without_attr();
__attribute__((target("arch=ivybridge")))
void redecl_without_attr();
// expected-error@+2 {{function redeclaration is missing 'target' attribute in a multiversioned function}}
// expected-note@-4 {{previous declaration is here}}
void redecl_without_attr();

__attribute__((target("arch=sandybridge")))
void multiversion_with_predecl();
__attribute__((target("default")))
void multiversion_with_predecl();
__attribute__((target("arch=sandybridge")))
void multiversion_with_predecl(){}
__attribute__((target("default")))
void multiversion_with_predecl(){}
//expected-error@+3 {{redefinition of 'multiversion_with_predecl'}}
//expected-note@-2 {{previous definition is here}}
__attribute__((target("arch=sandybridge")))
void multiversion_with_predecl(){}

__attribute__((target("arch=sandybridge")))
void multiversion_with_predecl2();
__attribute__((target("arch=sandybridge")))
void multiversion_with_predecl2(){}
__attribute__((target("default")))
void multiversion_with_predecl2();
//expected-error@+3 {{redefinition of 'multiversion_with_predecl2'}}
//expected-note@-4 {{previous definition is here}}
__attribute__((target("arch=sandybridge")))
void multiversion_with_predecl2(){}
__attribute__((target("default")))
void multiversion_with_predecl2(){}

// All the following are fine in normal 'target' mode, but not
// in multiversion mode.
// expected-error@+1 {{function multiversioning doesn't support feature 'sgx'}}
__attribute__((target("sgx")))
void badMVFeature();
__attribute__((target("sse")))
void badMVFeature();

__attribute__((target("mmx")))
void badMVFeature2();
// expected-error@+1 {{function multiversioning doesn't support feature 'lwp'}}
__attribute__((target("lwp")))
void badMVFeature2();

__attribute__((target("arch=ivybridge,mmx")))
void badMVFeature3();
// expected-error@+1 {{function multiversioning doesn't support feature 'lwp'}}
__attribute__((target("arch=skylake,lwp")))
void badMVFeature3();

// expected-error@+1 {{function multiversioning doesn't support architecture 'k8'}}
__attribute__((target("arch=k8")))
void badMVArch();
__attribute__((target("arch=nehalem")))
void badMVArch();

__attribute__((target("arch=nehalem")))
void badMVArch2();
// expected-error@+1 {{function multiversioning doesn't support architecture 'k8'}}
__attribute__((target("arch=k8")))
void badMVArch2();

__attribute__((target("mmx")))
void badMVNegate();
// expected-error@+1 {{function multiversioning doesn't support feature 'no-mmx'}}
__attribute__((target("no-mmx")))
void badMVNegate();

__attribute__((target("arch=ivybridge")))
void some_bad_archs();
__attribute__((target("arch=amdfam10")))
void some_bad_archs();
// expected-warning@+2 {{ignoring unsupported architecture 'amdfam15'}}
// expected-error@+1 {{function multiversioning doesn't support architecture 'amdfam15'}}
__attribute__((target("arch=amdfam15")))
void some_bad_archs();
// expected-warning@+1 {{ignoring unsupported architecture 'amdfam10h'}}
__attribute__((target("arch=amdfam10h")))
void some_bad_archs();
// expected-warning@+1 {{ignoring unsupported architecture 'amdfam15h'}}
__attribute__((target("arch=amdfam15h")))
void some_bad_archs();

#endif
