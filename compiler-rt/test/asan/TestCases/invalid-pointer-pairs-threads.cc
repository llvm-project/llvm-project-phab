// RUN: %clangxx_asan -O0 %s -o %t -mllvm -asan-detect-invalid-pointer-pair

// RUN: %env_asan_opts=detect_invalid_pointer_pairs=1 %run %t a 2>&1 | FileCheck %s -check-prefix=OK -allow-empty
// RUN: %env_asan_opts=detect_invalid_pointer_pairs=1 not %run %t b 2>&1 | FileCheck %s -check-prefix=B

#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

char *pointers[2];

void *thread_main(void *n) {
  char local;

  unsigned long id = (unsigned long)n;
  pointers[id] = &local;
  sleep(1);

  return NULL;
}

int main(int argc, char **argv) {
  assert(argc >= 2);

  char t = argv[1][0];

  pthread_t threads[2];
  pthread_create(&threads[0], 0, thread_main, (void *)0);
  pthread_create(&threads[1], 0, thread_main, (void *)1);

  do {
  } while (pointers[0] == NULL || pointers[1] == NULL);

  if (t == 'a') {
    // OK-NOT: not handled yet
    unsigned r = pointers[0] - pointers[1];
  } else {
    char local;
    char *parent_pointer = &local;

    // B: ERROR: AddressSanitizer: invalid-pointer-pair
    // B: #{{[0-9]+ .*}} in main {{.*}}invalid-pointer-pairs-threads.cc:[[@LINE+1]]
    unsigned r = parent_pointer - pointers[0];
  }

  pthread_join(threads[0], 0);
  pthread_join(threads[1], 0);

  return 0;
}
