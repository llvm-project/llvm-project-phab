// RUN: %clang_cc1 -analyze -analyzer-checker=core %s  -x c++ 2>&1 | FileCheck %s

// CHECK-NOT: analyzer-ignore.cpp:[[@LINE+5]]:{{[0-9]+}}: clang_sa_warning: Division by zero [core.DivideZero]
int foo1()
{
  int b = 5;
  int c = 0;
  int tx = b/c;  // clang_sa_ignore [core.DivideZero    ]
  return tx;
}

// CHECK-NOT: analyzer-ignore.cpp:[[@LINE+5]]:{{[0-9]+}}: clang_sa_warning: Division by zero [core.DivideZero]
int foo2()
{
  int b = 5;
  int c = 0;
  int tx = b/c;  // clang_sa_ignore [    core.DivideZero,  whatever        ]. Note testing...
  return tx;
}


// CHECK-NOT: analyzer-ignore.cpp:[[@LINE+5]]:{{[0-9]+}}: clang_sa_warning: Division by zero [core.DivideZero]
int foo3()
{
  int b = 5;
  int c = 0;
  int tx = b/c;  // clang_sa_ignore [    core.DivideZero        ]. Note testing...
  return tx;
}


// CHECK-NOT: analyzer-ignore.cpp:[[@LINE+5]]:{{[0-9]+}}: clang_sa_warning: Division by zero [core.DivideZero]
int foo4()
{
  int b = 5;
  int c = 0;
  int tx = b/c;  // clang_sa_ignore [    core.        ]. Note testing...
  return tx;
}

// CHECK-NOT: analyzer-ignore.cpp:[[@LINE+5]]:{{[0-9]+}}: clang_sa_warning: Division by zero [core.DivideZero]
int foo5()
{
  int b = 5;
  int c = 0;
  int tx = b/c;  // clang_sa_ignore [    core        ]. Note testing...
  return tx;
}

// CHECK-NOT: analyzer-ignore.cpp:[[@LINE+5]]:{{[0-9]+}}: clang_sa_warning: Division by zero [core.DivideZero]
int foo6()
{
  int b = 5;
  int c = 0;
  int tx = b/c;  // clang_sa_ignore [    core,        ]. Note testing...
  return tx;
}

// CHECK-NOT: warning:
// CHECK: analyzer-ignore.cpp:[[@LINE+5]]:{{[0-9]+}}: clang_sa_warning: Division by zero [core.DivideZero]
int foo7()
{
  int b = 5;
  int c = 0;
  int tx = b/c;  // clang_sa_ignore [    core    .     ]. Note testing...
  return tx;
}

// CHECK-NOT: warning:
// CHECK: analyzer-ignore.cpp:[[@LINE+5]]:{{[0-9]+}}: clang_sa_warning: Division by zero [core.DivideZero]
int foo8()
{
  int b = 5;
  int c = 0;
  int tx = b/c;  // clang_sa_ignore
  return tx;
}

// CHECK-NOT: warning:
// CHECK: analyzer-ignore.cpp:[[@LINE+5]]:{{[0-9]+}}: clang_sa_warning: Division by zero [core.DivideZero]
int foo9()
{
  int b = 5;
  int c = 0;
  int tx = b/c;  // clang_sa_ignore [    asd kf jh sad        ]. Note testing...
  return tx;
}

// CHECK-NOT: warning:
// CHECK: analyzer-ignore.cpp:[[@LINE+5]]:{{[0-9]+}}: clang_sa_warning: Division by zero [core.DivideZero]
int foo10()
{
  int b = 5;
  int c = 0;
  int tx = b/c;  // clang_sa_ignore [  asd  fkjgifm    ]. Note testing...
  return tx;
}

// CHECK-NOT: warning:
// CHECK: analyzer-ignore.cpp:[[@LINE+5]]:{{[0-9]+}}: clang_sa_warning: Division by zero [core.DivideZero]
int foo11()
{
  int b = 5;
  int c = 0;
  int tx = b/c;  // clang_sa_ignore [asdk,feojff        ]. Note testing...
  return tx;
}

// CHECK-NOT: warning:
// CHECK: analyzer-ignore.cpp:[[@LINE+5]]:{{[0-9]+}}: clang_sa_warning: Division by zero [core.DivideZero]
int foo12()
{
  int b = 5;
  int c = 0;
  int tx = b/c;  // clang_sa_ignore [askjdlaksjd       ]. Note testing...
  return tx;
}

// CHECK-NOT: warning:
// CHECK: analyzer-ignore.cpp:[[@LINE+5]]:{{[0-9]+}}: clang_sa_warning: Division by zero [core.DivideZero]
int foo13()
{
  int b = 5;
  int c = 0;
  int tx = b/c;  // clang_sa_ignore [    asdkfgsad        ]. Note testing...
  return tx;
}

// CHECK-NOT: warning:
// CHECK: analyzer-ignore.cpp:[[@LINE+8]]:{{[0-9]+}}: clang_sa_warning: Division by zero [core.DivideZero]
// CHECK-NOT: analyzer-ignore.cpp:[[@LINE+6]]:{{[0-9]+}}: clang_sa_warning: Result of 'malloc' is converted to a pointer of type 'unsigned int', which is incompatible with sizeof operand type 'int' [unix.MallocSizeof]
void* malloc (unsigned int size);
int foo14()
{
  int b = 5;
  int c = 0;
  unsigned int *p = (unsigned int*)malloc(sizeof(int)*100); // clang_sa_ignore [unix.MallocSizeof]
  int tx = b/c;  // clang_sa_ignore [askjdlaksjd       ]. Note testing...
  return tx;
}
