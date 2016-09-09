// RUN: %clang --analyze %s 2>&1 | FileCheck %s
// CHECK: clang_sa_warning: Address of stack memory associated with local variable 't0' returned to caller
char* foo()
{
  char t0[] = "012345";
  return t0;
}
