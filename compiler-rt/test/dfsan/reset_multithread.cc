// RUN: %clang_dfsan %s -o %t && %run %t
// RUN: %clang_dfsan -mllvm -dfsan-args-abi %s -o %t && %run %t
// XFAIL: *
// Tests that dfsan runtime is reset can be violated in multithreaded settings.

#include <sanitizer/dfsan_interface.h>

#include <assert.h>
#include <pthread.h>

void *baz(void* unused) {
  for(int i = 0; i < 1000000000; i++) {
    for(int k = 0; k < 100; k++) {
      int j = 1;
      dfsan_label j_label = dfsan_create_label("j", 0);
      dfsan_set_label(j_label, &j, sizeof(j));
    }
    pthread_yield();
    dfsan_reset();
    pthread_yield();
    assert(dfsan_get_label_count() == 0);
  }
}

int main(void) {
  pthread_t t1, t2;
  pthread_create(&t1, NULL, baz, NULL);
  pthread_create(&t2, NULL, baz, NULL);
  pthread_join(t1, NULL);
  pthread_join(t2, NULL);
  return 0;
}
