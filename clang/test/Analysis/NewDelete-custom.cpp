// RUN: %clang_analyze_cc1 -analyzer-checker=core,cplusplus.NewDelete,unix.Malloc -std=c++11 -fcxx-exceptions -fblocks -verify %s
// RUN: %clang_analyze_cc1 -analyzer-checker=core,cplusplus.NewDelete,cplusplus.NewDeleteLeaks,unix.Malloc -fcxx-exceptions -std=c++11 -DLEAKS -fblocks -verify %s
// RUN: %clang_analyze_cc1 -analyzer-checker=core,cplusplus.NewDelete,cplusplus.NewDeleteLeaks,unix.Malloc -fcxx-exceptions -std=c++11 -analyzer-config c++-allocator-inlining=true -DLEAKS -fblocks -verify %s
#include "Inputs/system-header-simulator-cxx.h"

#ifndef LEAKS
// expected-no-diagnostics
#endif


void *allocator(std::size_t size);

void *operator new[](std::size_t size) throw(std::bad_alloc) { return allocator(size); }
void *operator new(std::size_t size) throw(std::bad_alloc) { return allocator(size); }
void *operator new[](std::size_t size, const std::nothrow_t &nothrow) throw() {
  void *p = 0;
  try {
    p = allocator(size);
  } catch (...) {
  }
  return p;
}
void *operator new(std::size_t size, const std::nothrow_t &nothrow) throw() {
  void *p = 0;
  try {
    p = allocator(size);
  } catch (...) {
  }
  return p;
}
void *operator new(std::size_t, double d);

class C {
public:
  void *operator new(std::size_t);  
};

void testNewMethod() {
  void *p1 = C::operator new(0); // no warn

  C *p2 = new C; // no warn

  C *c3 = ::new C;
}
#ifdef LEAKS
// expected-warning@-2{{Potential leak of memory pointed to by 'c3'}}
#endif

void testOpNewArray() {
  void *p = operator new[](0);
}
#ifdef LEAKS
// expected-warning@-2{{Potential leak of memory pointed to by 'p'}}
#endif

void testNewExprArray() {
  int *p = new int[0];
}
#ifdef LEAKS
// expected-warning@-2{{Potential leak of memory pointed to by 'p'}}
#endif


//----- Custom non-placement operators
void testOpNew() {
  void *p = operator new(0);
}
#ifdef LEAKS
// expected-warning@-2{{Potential leak of memory pointed to by 'p'}}
#endif

void testNewExpr() {
  int *p = new int;
}
#ifdef LEAKS
// expected-warning@-2{{Potential leak of memory pointed to by 'p'}}
#endif


//----- Custom NoThrow placement operators
void testOpNewNoThrow() {
  void *p = operator new(0, std::nothrow);
}
#ifdef LEAKS
// expected-warning@-2{{Potential leak of memory pointed to by 'p'}}
#endif

void testNewExprNoThrow() {
  int *p = new(std::nothrow) int;
}
#ifdef LEAKS
// expected-warning@-2{{Potential leak of memory pointed to by 'p'}}
#endif

//----- Custom placement operators
void testOpNewPlacement() {
  void *p = operator new(0, 0.1); // no warn
} 

void testNewExprPlacement() {
  int *p = new(0.1) int; // no warn
}
