// RUN: %clang_cc1 -triple aarch64-linux-eabi -general-regs-only %s -verify -DBANNED=int '-DVECATTR=__attribute__((ext_vector_type(2)))' -Wno-unused-value
// RUN: %clang_cc1 -triple aarch64-linux-eabi -general-regs-only %s -verify -DBANNED=float -Wno-unused-value
// RUN: %clang_cc1 -triple aarch64-linux-eabi -general-regs-only %s -verify -DBANNED=FloatTypedef -Wno-unused-value

// Try to diagnose every use of a floating-point or vector operation that we
// can't trivially fold. Declaring these, passing their addresses around, etc.
// is OK, assuming we never actually use them in this TU.

typedef float FloatTypedef;

#ifndef VECATTR
#define VECATTR
#endif
typedef BANNED BannedTy VECATTR;

// Whether or not this is the actual definition for uintptr_t doesn't matter.
typedef unsigned long long uintptr_t;

// We only start caring when the user actually tries to do things with floats;
// declarations on their own are fine.
// This allows a user to #include some headers that happen to use floats. As
// long as they're never actually used, no one cares.
BannedTy foo();
void bar(BannedTy);
extern BannedTy gBaz;

void calls() {
  __auto_type a = foo(); // expected-error{{use of floating-point or vector values is disabled}}
  BannedTy b = foo(); // expected-error{{use of floating-point or vector values is disabled}}
  foo(); // expected-error{{use of floating-point or vector values is disabled}}
  (void)foo(); // expected-error{{use of floating-point or vector values is disabled}}

  bar(1); // expected-error{{use of floating-point or vector values is disabled}}
  bar(1.); // expected-error{{use of floating-point or vector values is disabled}}

  gBaz = 1; // expected-error{{use of floating-point or vector values is disabled}}
  gBaz = 1.; // expected-error{{use of floating-point or vector values is disabled}}
}

BannedTy global_banned;

void literals() {
  volatile int i;
  i = 1.;
  i = 1.0 + 2;
  i = (int)(1.0 + 2);

  BannedTy j = 1; // expected-error{{use of floating-point or vector values is disabled}}
  BannedTy k = (BannedTy)1.1; // expected-error{{use of floating-point or vector values is disabled}}
  BannedTy l;
  (BannedTy)3; // expected-error{{use of floating-point or vector values is disabled}}
}

struct Baz {
  int i;
  BannedTy f;
};

union Qux {
  int i;
  BannedTy j;
  BannedTy *p;
};

struct Baz *getBaz(int i);

void structs(void *p) {
  struct Baz b;
  union Qux q;

  q = (union Qux){};
  q.i = 1;
  q.j = 2.; // expected-error{{use of floating-point or vector values is disabled}}
  q.j = 2.f; // expected-error{{use of floating-point or vector values is disabled}}
  q.j = 2; // expected-error{{use of floating-point or vector values is disabled}}
  q.p = (BannedTy *)p;
  q.p += 5;
  *q.p += 5; // expected-error{{use of floating-point or vector values is disabled}}

  b = (struct Baz){}; // expected-error{{use of floating-point or vector values is disabled}}
  b = *getBaz(2. + b.i); // expected-error{{use of floating-point or vector values is disabled}}
  *getBaz(2. + b.i) = b; // expected-error{{use of floating-point or vector values is disabled}}
}

struct Baz callBaz(struct Baz);
union Qux callQux(union Qux);
struct Baz *callBazPtr(struct Baz *);
union Qux *callQuxPtr(union Qux *);

void structCalls() {
  void *p;
  callBazPtr((struct Baz *)p);
  callQuxPtr((union Qux *)p);

  // One error for returning a `struct Baz`, one for taking a `struct Baz`, one
  // for constructing a `struct Baz`. Not ideal, but...
  callBaz((struct Baz){}); // expected-error 3{{use of floating-point or vector values is disabled}}
  callQux((union Qux){});
}

extern BannedTy extern_arr[4];
static BannedTy static_arr[4];

void arrays() {
  BannedTy bannedArr[] = { // expected-error{{use of floating-point or vector values is disabled}}
    1,
    1.0,
    2.0f,
  };
  int intArr[] = { 1.0, 2.0f };

  intArr[0] = 1.0;
  bannedArr[0] = 1; // expected-error{{use of floating-point or vector values is disabled}}
}

BannedTy *getMemberPtr(struct Baz *b, int i) {
  if (i)
    return &b->f;
  return &((struct Baz *)(uintptr_t)((uintptr_t)b + 1.0))->f; // expected-error{{use of floating-point or vector values is disabled}}
}

void casts() {
  void *volatile p;

  (BannedTy *)p;
  (void)*(BannedTy *)p; // expected-error{{use of floating-point or vector values is disabled}}

  (void)*(struct Baz *)p; // expected-error{{use of floating-point or vector values is disabled}}
  (void)*(union Qux *)p;
  (void)((union Qux *)p)->i;
  (void)((union Qux *)p)->j; // expected-error{{use of floating-point or vector values is disabled}}
}

BannedTy returns() { // expected-error{{use of floating-point or vector values is disabled}}
  return 0; // expected-error{{use of floating-point or vector values is disabled}}
}

int unevaluated() {
  return sizeof((BannedTy)0.0);
}

void moreUnevaluated(int x)
    __attribute__((diagnose_if(x + 1.1 == 2.1, "oh no", "warning"))) {
  moreUnevaluated(3);
  moreUnevaluated(1); // expected-warning{{oh no}} expected-note@-2{{from 'diagnose_if'}}
}

void noSpam() {
  float r = 1. + 2 + 3 + 4 + 5.; // expected-error 2{{use of floating-point or vector values is disabled}}
  float r2 = 1. + r + 3 + 4 + 5.; // expected-error 3{{use of floating-point or vector values is disabled}}
  float r3 = 1 + 2 + 3 + 4 + 5; // expected-error{{use of floating-point or vector values is disabled}}

  // no errors expected below: they can be trivially folded to a constant.
  int i = 1. + 2 + 3 + 4 + 5.; // no error: we can trivially fold this to a constant.
  int j = (int)(1. + 2 + 3) + 4; // no error: we can trivially fold this to a constant.
  int k = (int)(1. + 2 + 3) + j;
  int l = (int)(1. + 2 + 3) +
    r; //expected-error {{use of floating-point or vector values is disabled}}

  const int cj = (int)(1. + 2 + 3) + 4; // no error: we can trivially fold this to a constant.
  int ck = (int)(1. + cj + 3) +
    r; // expected-error{{use of floating-point or vector values is disabled}}
}

float fooFloat();
int exprStmt() {
  return ({ fooFloat() + 1 + 2; }) + 3; // expected-error 2{{use of floating-point or vector values is disabled}}
}

int longExprs() {
  // FIXME: Should we be able to fold this to a constant?
  return 1 + ((struct Baz){.i = 1}).i; // expected-error{{use of floating-point or vector values is disabled}}
}

struct RecursiveTy { // expected-note{{is not complete until}}
  int a;
  BannedTy b;
  struct RecursiveTy ty; // expected-error{{field has incomplete type}}
};

struct StructRec {
  int a;
  BannedTy b;
  struct RecursiveTy ty;
};

union UnionRec {
  int a;
  BannedTy b;
  struct RecursiveTy ty;
};

struct UndefType; // expected-note 3{{forward declaration}}

struct StructUndef {
  int a;
  BannedTy b;
  struct UndefType s; // expected-error{{field has incomplete type}}
};

union UnionUndef {
  int a;
  BannedTy b;
  struct UndefType s; // expected-error{{field has incomplete type}}
};

// ...Just be sure we don't crash on these.
void cornerCases() {
  struct RecInl {  // expected-note{{is not complete until the closing}}
    struct RecInl s; // expected-error{{field has incomplete type}}
    BannedTy f;
  } inl;
  __builtin_memset(&inl, 0, sizeof(inl));

  BannedTy fs[] = {
    ((struct RecursiveTy){}).a,
    ((struct StructRec){}).a,
    ((union UnionRec){}).a,
    ((struct StructUndef){}).a,
    ((union UnionUndef){}).a,
  };

  BannedTy fs2[] = {
    ((struct RecursiveTy){}).b,
    ((struct StructRec){}).b,
    ((union UnionRec){}).b,
    ((struct StructUndef){}).b,
    ((union UnionUndef){}).b,
  };

  struct UndefType a = {}; // expected-error{{has incomplete type}}
  struct RecursiveTy b = {};
  struct StructRec c = {};
  union UnionRec d = {};
  struct StructUndef e = {};
  union UnionUndef f = {};

  __builtin_memset(&a, 0, sizeof(a));
  __builtin_memset(&b, 0, sizeof(b));
  __builtin_memset(&c, 0, sizeof(c));
  __builtin_memset(&d, 0, sizeof(d));
  __builtin_memset(&e, 0, sizeof(e));
  __builtin_memset(&f, 0, sizeof(f));
}

void floatFunctionDefIn(BannedTy a) {} // expected-error{{use of floating-point or vector values is disabled}}
BannedTy floatFunctionDefOut(void) {} // expected-error{{use of floating-point or vector values is disabled}}
BannedTy // expected-error{{use of floating-point or vector values is disabled}}
floatFunctionDefInOut(BannedTy a) {} // expected-error{{use of floating-point or vector values is disabled}}

void floatInStructDefIn(struct Baz a) {} // expected-error{{use of floating-point or vector values is disabled}}
struct Baz floatInStructDefOut(void) {} // expected-error{{use of floating-point or vector values is disabled}}
