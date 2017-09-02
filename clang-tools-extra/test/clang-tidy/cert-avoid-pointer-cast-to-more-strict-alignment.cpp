// RUN: %check_clang_tidy %s cert-exp36-c %t

void function(void) {
  char c = 'x';
  int *ip = reinterpret_cast<int *>(&c);
  // CHECK-MESSAGES: :[[@LINE-1]]:13: warning: do not cast pointers into more strictly aligned pointer types [cert-exp36-c] 
}
