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
