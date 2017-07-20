// RUN: %clang_dfsan %s -o %t && %run %t
// RUN: %clang_dfsan -mllvm -dfsan-args-abi %s -o %t && %run %t

// Tests that dfsan runtime is reset correctly.

#include <sanitizer/dfsan_interface.h>
#include <assert.h>

int main(void) {
  int i = 1;
  int j = 1;
  dfsan_label i_label = dfsan_create_label("i", 0);
  dfsan_set_label(i_label, &i, sizeof(i));
  dfsan_label j_label = dfsan_create_label("j", 0);
  dfsan_add_label(j_label, &j, sizeof(j));
  assert(dfsan_get_label_count() == 2);

  dfsan_reset();
  assert(dfsan_get_label_count() == 0);

  return 0;
}
