// RUN: %check_clang_tidy %s misc-terminating-continue %t

void f() {
  do {
    continue;
    // CHECK-MESSAGES: :[[@LINE-1]]:5: warning: terminating 'continue' [misc-terminating-continue]
  } while(false);

  do {
    continue;
    // CHECK-MESSAGES: :[[@LINE-1]]:5: warning: terminating 'continue' [misc-terminating-continue]
  } while(0);

  do {
    continue;
    // CHECK-MESSAGES: :[[@LINE-1]]:5: warning: terminating 'continue' [misc-terminating-continue]
  } while(nullptr);

  do {
    continue;
    // CHECK-MESSAGES: :[[@LINE-1]]:5: warning: terminating 'continue' [misc-terminating-continue]
  } while(__null);


  do {
    int x = 1;
    if (x > 0) continue;
    // CHECK-MESSAGES: :[[@LINE-1]]:16: warning: terminating 'continue' [misc-terminating-continue]
  } while (false);
}

void g() {
  do {
    do {
      continue;
      int x = 1;
    } while (1 == 1);
  } while (false);

  do {
    for (int i = 0; i < 1; ++i) {
      continue;
      int x = 1;
    }
  } while (false);

  do {
    while (true) {
      continue;
      int x = 1;
    }
  } while (false);

  int v[] = {1,2,3,34};
  do {
    for (int n : v) {
      if (n>2) continue;
    }
  } while (false);
}