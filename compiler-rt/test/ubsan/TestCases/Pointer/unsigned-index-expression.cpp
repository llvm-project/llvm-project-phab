// RUN: %clangxx -fsanitize=pointer-overflow %s -o %t
// RUN: %t 2>&1 | FileCheck %s

int main(int argc, char *argv[]) {
  unsigned long long offset = -1;
  char *p = (char *)7;

  // CHECK: runtime error: unsigned pointer index expression result is 0x{{0+}}6, preceding its base 0x{{0+}}7
  char *q = p + offset;

  return 0;
}
