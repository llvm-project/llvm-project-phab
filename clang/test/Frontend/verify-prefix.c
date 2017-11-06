// gconst-note@2 {{variable 'glb' declared const here}}
GC int glb = 5;

// Check various -verify prefixes.
// RUN: %clang_cc1             -DGC=const -DLC=      -DSC= -verify=gconst %s
// RUN: %clang_cc1 -Wcast-qual -DGC=      -DLC=const -DSC= -verify=lconst %s
// RUN: %clang_cc1             -DGC=      -DLC=      -DSC= -verify=nconst %s

// Check empty -verify prefix.  Make sure there's no additional output, which
// might indicate diagnostic verification became enabled even though it was
// requested incorrectly.
// RUN: not %clang_cc1 -DGC=const -DLC=const -DSC=const -verify= %s 2> %t
// RUN: FileCheck --check-prefix=EMPTY %s < %t
// EMPTY-NOT: {{.}}
// EMPTY:     error: invalid value '' in '-verify='
// EMPTY-NOT: {{.}}

// Check omitting the -verify prefix.
// RUN: %clang_cc1 -DGC= -DLC= -DSC=const -verify %s

// Check omitting -verify: check that code actually has expected diagnostics.
// RUN: not %clang_cc1 -Wcast-qual -DGC=const -DLC=const -DSC=const %s 2> %t
// RUN: FileCheck --check-prefix=ALL %s < %t
// ALL: cannot assign to variable 'glb' with const-qualified type 'const int'
// ALL: variable 'glb' declared const here
// ALL: cast from 'const int *' to 'int *' drops const qualifier
// ALL: initializing 'int *' with an expression of type 'const int *' discards qualifiers

void foo() {
  LC int loc = 5;
  SC static int sta = 5;
  glb = 6; // gconst-error {{cannot assign to variable 'glb' with const-qualified type 'const int'}}
  // lconst-warning@+1 {{cast from 'const int *' to 'int *' drops const qualifier}}
  *(int*)(&loc) = 6;
  int *p = &sta; // expected-warning {{initializing 'int *' with an expression of type 'const int *' discards qualifiers}}
}

// nconst-no-diagnostics
