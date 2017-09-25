// RUN: %check_clang_tidy %s cert-exp36-c %t -- -- -std=c11

void function(void) {
  char c = 'x';
  int *ip = (int *)&c;
  // CHECK-MESSAGES: :[[@LINE-1]]:13: warning: do not cast pointers into more strictly aligned pointer types [cert-exp36-c] 
}

struct foo_header {
  int len;
};
 
void function2(char *data, unsigned offset) {
  struct foo_header *tmp;
  struct foo_header header;
 
  tmp = (struct foo_header *)(data + offset);
  // CHECK-MESSAGES: :[[@LINE-1]]:9: warning: do not cast pointers into more strictly aligned pointer types [cert-exp36-c] 
}

// Test from -Wcast-align check:

// Simple casts.
void test0(char *P) {
  char *a  = (char*)  P;
  short *b = (short*) P;
  // CHECK-MESSAGES: :[[@LINE-1]]:14: warning: do not cast pointers into more strictly aligned pointer types [cert-exp36-c]
  int *c   = (int*)   P;
  // CHECK-MESSAGES: :[[@LINE-1]]:14: warning: do not cast pointers into more strictly aligned pointer types [cert-exp36-c]
}

// Casts from void* are a special case.
void test1(void *P) {
  char *a  = (char*)  P;
  short *b = (short*) P;
  int *c   = (int*)   P;

  const volatile void *P2 = P;
  char *d  = (char*)  P2;
  short *e = (short*) P2;
  int *f   = (int*)   P2;

  const char *g  = (const char*)  P2;
  const short *h = (const short*) P2;
  const int *i   = (const int*)   P2;

  const volatile char *j  = (const volatile char*)  P2;
  const volatile short *k = (const volatile short*) P2;
  const volatile int *l   = (const volatile int*)   P2;
}

// Aligned struct.
struct __attribute__((aligned(16))) A {
  char buffer[16];
};
void test2(char *P) {
  struct A *a = (struct A*) P;
  // CHECK-MESSAGES: :[[@LINE-1]]:17: warning: do not cast pointers into more strictly aligned pointer types [cert-exp36-c]
}

// Incomplete type.
void test3(char *P) {
  struct B *b = (struct B*) P;
}

// Do not issue a warning. The aligned attribute changes the alignment of the
// variables and fields.
char __attribute__((aligned(4))) a[16];

struct S0 {
  char a[16];
};

struct S {
  char __attribute__((aligned(4))) a[16];
  struct S0 __attribute__((aligned(4))) s0;
};

// same FIXME as in line 120
void test4() {
  struct S s;
  int *i = (int *)s.a;
  // CHECK-MESSAGES: :[[@LINE-1]]:12: warning: do not cast pointers into more strictly aligned pointer types [cert-exp36-c]
  i = (int *)&s.s0;
  // CHECK-MESSAGES: :[[@LINE-1]]:7: warning: do not cast pointers into more strictly aligned pointer types [cert-exp36-c]
  i = (int *)a;
  // CHECK-MESSAGES: :[[@LINE-1]]:7: warning: do not cast pointers into more strictly aligned pointer types [cert-exp36-c]
}

// No warnings.
typedef int (*FnTy)(void);
unsigned int func5(void);

FnTy test5(void) {
  return (FnTy)&func5;
}


// Something that does not trigger the check:

struct w;

void function3(struct w *v) {
  int *ip = (int *)v;
  struct w *u = (struct w *)ip;
}

struct x {
   _Alignas(int) char c;
};

void function4(void) {
  struct x c = {'x'};
  int *ip = (int *)&c;
}

// FIXME: we do not want a warning for this
void function5(void) {
  _Alignas(int) char c = 'x';
  int *ip = (int *)&c;
  // CHECK-MESSAGES: :[[@LINE-1]]:13: warning: do not cast pointers into more strictly aligned pointer types [cert-exp36-c]
}
