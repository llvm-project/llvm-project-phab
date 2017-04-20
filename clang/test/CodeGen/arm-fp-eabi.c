// RUN: %clang %s -target arm-none-eabi -O2 -S -o nofp.s
// RUN: grep -v '.setfp' nofp.s
// RUN: %clang %s -target arm-none-eabi -fno-omit-frame-pointer -O2 -S -o fp.s
// RUN: grep '.setfp' fp.s

void
foo(void);

int main()
{
  foo();
  return 0;
}
