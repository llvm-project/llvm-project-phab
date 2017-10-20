// RUN: %check_clang_tidy %s misc-misplaced-operator-in-strlen-in-alloc %t

typedef __typeof(sizeof(int)) size_t;
void *malloc(size_t);
void *calloc(size_t, size_t);
void *realloc(void *, size_t);

size_t strlen(const char*);

void bad_malloc(char *name) {
  char *new_name = (char*) malloc(strlen(name + 1));
  // CHECK-MESSAGES: :[[@LINE-1]]:28: warning: Binary operator + 1 is inside strlen
  // CHECK-FIXES: {{^  char \*new_name = \(char\*\) malloc\(}}strlen(name) + 1{{\);$}}
}

void bad_calloc(char *name) {
  char *new_names = (char*) calloc(2, strlen(name + 1));
  // CHECK-MESSAGES: :[[@LINE-1]]:29: warning: Binary operator + 1 is inside strlen
  // CHECK-FIXES: {{^  char \*new_names = \(char\*\) calloc\(2, }}strlen(name) + 1{{\);$}}
}

void bad_realloc(char * old_name, char *name) {
  char *new_name = (char*) realloc(old_name, strlen(name + 1));
  // CHECK-MESSAGES: :[[@LINE-1]]:28: warning: Binary operator + 1 is inside strlen
  // CHECK-FIXES: {{^  char \*new_name = \(char\*\) realloc\(old_name, }}strlen(name) + 1{{\);$}}
}
