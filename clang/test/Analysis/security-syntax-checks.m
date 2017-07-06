// RUN: %clang_cc1 -triple i386-apple-darwin10 -analyze -analyzer-checker=security.insecureAPI,security.FloatLoopCounter %s -verify
// RUN: %clang_cc1 -triple i386-apple-darwin10 -analyze -DUSE_BUILTINS -analyzer-checker=security.insecureAPI,security.FloatLoopCounter %s -verify
// RUN: %clang_cc1 -triple i386-apple-darwin10 -analyze -DVARIANT -analyzer-checker=security.insecureAPI,security.FloatLoopCounter %s -verify
// RUN: %clang_cc1 -triple i386-apple-darwin10 -analyze -DUSE_BUILTINS -DVARIANT -analyzer-checker=security.insecureAPI,security.FloatLoopCounter %s -verify
// RUN: %clang_cc1 -triple x86_64-unknown-cloudabi -analyze -analyzer-checker=security.insecureAPI,security.FloatLoopCounter %s -verify
// RUN: %clang_cc1 -triple x86_64-unknown-cloudabi -analyze -DUSE_BUILTINS -analyzer-checker=security.insecureAPI,security.FloatLoopCounter %s -verify
// RUN: %clang_cc1 -triple x86_64-unknown-cloudabi -analyze -DVARIANT -analyzer-checker=security.insecureAPI,security.FloatLoopCounter %s -verify
// RUN: %clang_cc1 -triple x86_64-unknown-cloudabi -analyze -DUSE_BUILTINS -DVARIANT -analyzer-checker=security.insecureAPI,security.FloatLoopCounter %s -verify

#ifdef USE_BUILTINS
# define BUILTIN(f) __builtin_ ## f
#else /* USE_BUILTINS */
# define BUILTIN(f) f
#endif /* USE_BUILTINS */

#include "Inputs/system-header-simulator-for-valist.h"
#include "Inputs/system-header-simulator-for-simple-stream.h"

typedef typeof(sizeof(int)) size_t;


// <rdar://problem/6336718> rule request: floating point used as loop 
//  condition (FLP30-C, FLP-30-CPP)
//
// For reference: https://www.securecoding.cert.org/confluence/display/seccode/FLP30-C.+Do+not+use+floating+point+variables+as+loop+counters
//
void test_float_condition() {
  for (float x = 0.1f; x <= 1.0f; x += 0.1f) {} // expected-warning{{Variable 'x' with floating point type 'float'}}
  for (float x = 100000001.0f; x <= 100000010.0f; x += 1.0f) {} // expected-warning{{Variable 'x' with floating point type 'float'}}
  for (float x = 100000001.0f; x <= 100000010.0f; x++ ) {} // expected-warning{{Variable 'x' with floating point type 'float'}}
  for (double x = 100000001.0; x <= 100000010.0; x++ ) {} // expected-warning{{Variable 'x' with floating point type 'double'}}
  for (double x = 100000001.0; ((x)) <= 100000010.0; ((x))++ ) {} // expected-warning{{Variable 'x' with floating point type 'double'}}
  
  for (double x = 100000001.0; 100000010.0 >= x; x = x + 1.0 ) {} // expected-warning{{Variable 'x' with floating point type 'double'}}
  
  int i = 0;
  for (double x = 100000001.0; ((x)) <= 100000010.0; ((x))++, ++i ) {} // expected-warning{{Variable 'x' with floating point type 'double'}}
  
  typedef float FooType;
  for (FooType x = 100000001.0f; x <= 100000010.0f; x++ ) {} // expected-warning{{Variable 'x' with floating point type 'FooType'}}
}

// <rdar://problem/6335715> rule request: gets() buffer overflow
// Part of recommendation: 300-BSI (buildsecurityin.us-cert.gov)
char* gets(char *buf);

void test_gets() {
  char buff[1024];
  gets(buff); // expected-warning{{Call to function 'gets' is extremely insecure as it can always result in a buffer overflow}}
}

int getpw(unsigned int uid, char *buf);

void test_getpw() {
  char buff[1024];
  getpw(2, buff); // expected-warning{{The getpw() function is dangerous as it may overflow the provided buffer. It is obsoleted by getpwuid()}}
}

// <rdar://problem/6337132> CWE-273: Failure to Check Whether Privileges Were
//  Dropped Successfully
typedef unsigned int __uint32_t;
typedef __uint32_t __darwin_uid_t;
typedef __uint32_t __darwin_gid_t;
typedef __darwin_uid_t uid_t;
typedef __darwin_gid_t gid_t;
int setuid(uid_t);
int setregid(gid_t, gid_t);
int setreuid(uid_t, uid_t);
extern void check(int);
void abort(void);

void test_setuid() 
{
  setuid(2); // expected-warning{{The return value from the call to 'setuid' is not checked.  If an error occurs in 'setuid', the following code may execute with unexpected privileges}}
  setuid(0); // expected-warning{{The return value from the call to 'setuid' is not checked.  If an error occurs in 'setuid', the following code may execute with unexpected privileges}}
  if (setuid (2) != 0)
    abort();

  // Currently the 'setuid' check is not flow-sensitive, and only looks
  // at whether the function was called in a compound statement.  This
  // will lead to false negatives, but there should be no false positives.
  int t = setuid(2);  // no-warning
  (void)setuid (2); // no-warning

  check(setuid (2)); // no-warning

  setreuid(2,2); // expected-warning{{The return value from the call to 'setreuid' is not checked.  If an error occurs in 'setreuid', the following code may execute with unexpected privileges}}
  setregid(2,2); // expected-warning{{The return value from the call to 'setregid' is not checked.  If an error occurs in 'setregid', the following code may execute with unexpected privileges}}
}

// <rdar://problem/6337100> CWE-338: Use of cryptographically weak prng
typedef  unsigned short *ushort_ptr_t;  // Test that sugar doesn't confuse the warning.
int      rand(void);
double   drand48(void);
double   erand48(unsigned short[3]);
long     jrand48(ushort_ptr_t);
void     lcong48(unsigned short[7]);
long     lrand48(void);
long     mrand48(void);
long     nrand48(unsigned short[3]);
long     random(void);
int      rand_r(unsigned *);

void test_rand()
{
  unsigned short a[7];
  unsigned b;
  
  rand();	// expected-warning{{Function 'rand' is obsolete because it implements a poor random number generator.  Use 'arc4random' instead}}
  drand48();	// expected-warning{{Function 'drand48' is obsolete because it implements a poor random number generator.  Use 'arc4random' instead}}
  erand48(a);	// expected-warning{{Function 'erand48' is obsolete because it implements a poor random number generator.  Use 'arc4random' instead}}
  jrand48(a);	// expected-warning{{Function 'jrand48' is obsolete because it implements a poor random number generator.  Use 'arc4random' instead}}
  lcong48(a);	// expected-warning{{Function 'lcong48' is obsolete because it implements a poor random number generator.  Use 'arc4random' instead}}
  lrand48();	// expected-warning{{Function 'lrand48' is obsolete because it implements a poor random number generator.  Use 'arc4random' instead}}
  mrand48();	// expected-warning{{Function 'mrand48' is obsolete because it implements a poor random number generator.  Use 'arc4random' instead}}
  nrand48(a);	// expected-warning{{Function 'nrand48' is obsolete because it implements a poor random number generator.  Use 'arc4random' instead}}
  rand_r(&b);	// expected-warning{{Function 'rand_r' is obsolete because it implements a poor random number generator.  Use 'arc4random' instead}}
  random();	// expected-warning{{The 'random' function produces a sequence of values that an adversary may be able to predict.  Use 'arc4random' instead}}
}

char *mktemp(char *buf);

void test_mktemp() {
  char *x = mktemp("/tmp/zxcv"); // expected-warning{{Call to function 'mktemp' is insecure as it always creates or uses insecure temporary file}}
}


//===----------------------------------------------------------------------===
// strcpy()
//===----------------------------------------------------------------------===
#ifdef VARIANT

#define __strcpy_chk BUILTIN(__strcpy_chk)
char *__strcpy_chk(char *restrict s1, const char *restrict s2, size_t destlen);

#define strcpy(a,b) __strcpy_chk(a,b,(size_t)-1)

#else /* VARIANT */

#define strcpy BUILTIN(strcpy)
char *strcpy(char *restrict s1, const char *restrict s2);

#endif /* VARIANT */

void test_strcpy() {
  char x[4];
  char *y;

  strcpy(x, y); //expected-warning{{Call to function 'strcpy' is insecure as it does not provide bounding of the memory buffer. Replace unbounded copy functions with analogous functions that support length arguments such as 'strlcpy'. CWE-119}}
}

//===----------------------------------------------------------------------===
// strcat()
//===----------------------------------------------------------------------===
#ifdef VARIANT

#define __strcat_chk BUILTIN(__strcat_chk)
char *__strcat_chk(char *restrict s1, const char *restrict s2, size_t destlen);

#define strcat(a,b) __strcat_chk(a,b,(size_t)-1)

#else /* VARIANT */

#define strcat BUILTIN(strcat)
char *strcat(char *restrict s1, const char *restrict s2);

#endif /* VARIANT */

void test_strcat() {
  char x[4];
  char *y;

  strcat(x, y); //expected-warning{{Call to function 'strcat' is insecure as it does not provide bounding of the memory buffer. Replace unbounded copy functions with analogous functions that support length arguments such as 'strlcat'. CWE-119}}
}

//===----------------------------------------------------------------------===
// vfork()
//===----------------------------------------------------------------------===
typedef int __int32_t;
typedef __int32_t pid_t;
pid_t vfork(void);

void test_vfork() {
  vfork(); //expected-warning{{Call to function 'vfork' is insecure as it can lead to denial of service situations in the parent process}}
}

//===----------------------------------------------------------------------===
// mkstemp()
//===----------------------------------------------------------------------===

char *mkdtemp(char *template);
int mkstemps(char *template, int suffixlen);
int mkstemp(char *template);
char *mktemp(char *template);

void test_mkstemp() {
  mkstemp("XX"); // expected-warning {{Call to 'mkstemp' should have at least 6 'X's in the format string to be secure (2 'X's seen)}}
  mkstemp("XXXXXX");
  mkstemp("XXXXXXX");
  mkstemps("XXXXXX", 0);
  mkstemps("XXXXXX", 1); // expected-warning {{5 'X's seen}}
  mkstemps("XXXXXX", 2); // expected-warning {{Call to 'mkstemps' should have at least 6 'X's in the format string to be secure (4 'X's seen, 2 characters used as a suffix)}}
  mkdtemp("XX"); // expected-warning {{2 'X's seen}}
  mkstemp("X"); // expected-warning {{Call to 'mkstemp' should have at least 6 'X's in the format string to be secure (1 'X' seen)}}
  mkdtemp("XXXXXX");
}


//===----------------------------------------------------------------------===
// deprecated or unsafe buffer handling
//===----------------------------------------------------------------------===
typedef int wchar_t;

int sprintf(char *str, const char *format, ...);
//int vsprintf (char *s, const char *format, va_list arg);
int scanf(const char *format, ...);
int wscanf(const wchar_t *format, ...);
int fscanf(FILE *stream, const char *format, ...);
int fwscanf(FILE *stream, const wchar_t *format, ...);
int vscanf(const char *format, va_list arg);
int vwscanf(const wchar_t *format, va_list arg);
int vfscanf(FILE *stream, const char *format, va_list arg);
int vfwscanf(FILE *stream, const wchar_t *format, va_list arg);
int sscanf(const char *s, const char *format, ...);
int swscanf(const wchar_t *ws, const wchar_t *format, ...);
int vsscanf(const char *s, const char *format, va_list arg);
int vswscanf(const wchar_t *ws, const wchar_t *format, va_list arg);
int swprintf(wchar_t *ws, size_t len, const wchar_t *format, ...);
int snprintf(char *s, size_t n, const char *format, ...);
int vswprintf(wchar_t *ws, size_t len, const wchar_t *format, va_list arg);
int vsnprintf(char *s, size_t n, const char *format, va_list arg);
void *memcpy(void *destination, const void *source, size_t num);
void *memmove(void *destination, const void *source, size_t num);
char *strncpy(char *destination, const char *source, size_t num);
char *strncat(char *destination, const char *source, size_t num);
void *memset(void *ptr, int value, size_t num);

void test_deprecated_or_unsafe_buffer_handling_1() {
  char buf [5];
  wchar_t wbuf [5];
  int a;
  FILE *file;
  sprintf(buf, "a"); // expected-warning{{Using 'sprintf' is depracated as it does not provide bounding of the memory buffer or security checks introduced in the C11 standard. Replace with analogous functions introduced in C11 standard that supports length arguments or provides boundary checks such as 'sprintf_s'}}
  scanf("%d", &a); // expected-warning{{Using 'scanf' is depracated as it does not provide bounding of the memory buffer or security checks introduced in the C11 standard. Replace with analogous functions introduced in C11 standard that supports length arguments or provides boundary checks such as 'scanf_s'}}
  scanf("%s", buf); // expected-warning{{Call to function 'scanf' is insecure as it does not provide bounding of the memory buffer. Replace with analogous functions that support length arguments or provides boundary checks such as 'scanf_s' in case of C11}} expected-warning{{Using 'scanf' is depracated as it does not provide bounding of the memory buffer or security checks introduced in the C11 standard. Replace with analogous functions introduced in C11 standard that supports length arguments or provides boundary checks such as 'scanf_s'}}
  scanf("%4s", buf); // expected-warning{{Using 'scanf' is depracated as it does not provide bounding of the memory buffer or security checks introduced in the C11 standard. Replace with analogous functions introduced in C11 standard that supports length arguments or provides boundary checks such as 'scanf_s'}}
  wscanf((const wchar_t*) L"%s", buf); // expected-warning{{Call to function 'wscanf' is insecure as it does not provide bounding of the memory buffer. Replace with analogous functions that support length arguments or provides boundary checks such as 'wscanf_s' in case of C11}} expected-warning{{Using 'wscanf' is depracated as it does not provide bounding of the memory buffer or security checks introduced in the C11 standard. Replace with analogous functions introduced in C11 standard that supports length arguments or provides boundary checks such as 'wscanf_s'}}
  fscanf(file, "%d", &a); // expected-warning{{Using 'fscanf' is depracated as it does not provide bounding of the memory buffer or security checks introduced in the C11 standard. Replace with analogous functions introduced in C11 standard that supports length arguments or provides boundary checks such as 'fscanf_s'}}
  fscanf(file, "%s", buf); // expected-warning{{Call to function 'fscanf' is insecure as it does not provide bounding of the memory buffer. Replace with analogous functions that support length arguments or provides boundary checks such as 'fscanf_s' in case of C11}} expected-warning{{Using 'fscanf' is depracated as it does not provide bounding of the memory buffer or security checks introduced in the C11 standard. Replace with analogous functions introduced in C11 standard that supports length arguments or provides boundary checks such as 'fscanf_s'}}
  fscanf(file, "%4s", buf); // expected-warning{{Using 'fscanf' is depracated as it does not provide bounding of the memory buffer or security checks introduced in the C11 standard. Replace with analogous functions introduced in C11 standard that supports length arguments or provides boundary checks such as 'fscanf_s'}}
  fwscanf(file, (const wchar_t*) L"%s", wbuf); // expected-warning{{Call to function 'fwscanf' is insecure as it does not provide bounding of the memory buffer. Replace with analogous functions that support length arguments or provides boundary checks such as 'fwscanf_s' in case of C11}} expected-warning{{Using 'fwscanf' is depracated as it does not provide bounding of the memory buffer or security checks introduced in the C11 standard. Replace with analogous functions introduced in C11 standard that supports length arguments or provides boundary checks such as 'fwscanf_s'}}
  sscanf("5", "%d", &a); // expected-warning{{Using 'sscanf' is depracated as it does not provide bounding of the memory buffer or security checks introduced in the C11 standard. Replace with analogous functions introduced in C11 standard that supports length arguments or provides boundary checks such as 'sscanf_s'}}
  sscanf("5", "%s", buf); // expected-warning{{Call to function 'sscanf' is insecure as it does not provide bounding of the memory buffer. Replace with analogous functions that support length arguments or provides boundary checks such as 'sscanf_s' in case of C11}} expected-warning{{Using 'sscanf' is depracated as it does not provide bounding of the memory buffer or security checks introduced in the C11 standard. Replace with analogous functions introduced in C11 standard that supports length arguments or provides boundary checks such as 'sscanf_s'}}
  sscanf("5", "%4s", buf); // expected-warning{{Using 'sscanf' is depracated as it does not provide bounding of the memory buffer or security checks introduced in the C11 standard. Replace with analogous functions introduced in C11 standard that supports length arguments or provides boundary checks such as 'sscanf_s'}}
  swscanf(L"5", (const wchar_t*) L"%s", wbuf); // expected-warning{{Call to function 'swscanf' is insecure as it does not provide bounding of the memory buffer. Replace with analogous functions that support length arguments or provides boundary checks such as 'swscanf_s' in case of C11}} expected-warning{{Using 'swscanf' is depracated as it does not provide bounding of the memory buffer or security checks introduced in the C11 standard. Replace with analogous functions introduced in C11 standard that supports length arguments or provides boundary checks such as 'swscanf_s'}}
  swprintf(L"5", 1, (const wchar_t*) L"%s", wbuf); // expected-warning{{Using 'swprintf' is depracated as it does not provide bounding of the memory buffer or security checks introduced in the C11 standard. Replace with analogous functions introduced in C11 standard that supports length arguments or provides boundary checks such as 'swprintf_s'}}
  snprintf("5", 1, "%s", buf); // expected-warning{{Using 'snprintf' is depracated as it does not provide bounding of the memory buffer or security checks introduced in the C11 standard. Replace with analogous functions introduced in C11 standard that supports length arguments or provides boundary checks such as 'snprintf_s'}}
  memcpy(buf, wbuf, 1); // expected-warning{{Using 'memcpy' is depracated as it does not provide bounding of the memory buffer or security checks introduced in the C11 standard. Replace with analogous functions introduced in C11 standard that supports length arguments or provides boundary checks such as 'memcpy_s'}}
  memmove(buf, wbuf, 1); // expected-warning{{Using 'memmove' is depracated as it does not provide bounding of the memory buffer or security checks introduced in the C11 standard. Replace with analogous functions introduced in C11 standard that supports length arguments or provides boundary checks such as 'memmove_s'}}
  strncpy(buf, "a", 1); // expected-warning{{Using 'strncpy' is depracated as it does not provide bounding of the memory buffer or security checks introduced in the C11 standard. Replace with analogous functions introduced in C11 standard that supports length arguments or provides boundary checks such as 'strncpy_s'}}
  strncat(buf, "a", 1); // expected-warning{{Using 'strncat' is depracated as it does not provide bounding of the memory buffer or security checks introduced in the C11 standard. Replace with analogous functions introduced in C11 standard that supports length arguments or provides boundary checks such as 'strncat_s'}}
  memset(buf, 'a', 1); // expected-warning{{Using 'memset' is depracated as it does not provide bounding of the memory buffer or security checks introduced in the C11 standard. Replace with analogous functions introduced in C11 standard that supports length arguments or provides boundary checks such as 'memset_s'}}
}

void test_deprecated_or_unsafe_buffer_handling_2(const char *format, ...) {
  char buf [5];
  FILE *file;
  va_list args;
  va_start(args, format);
  vsprintf(buf, format, args); // expected-warning{{Using 'vsprintf' is depracated as it does not provide bounding of the memory buffer or security checks introduced in the C11 standard. Replace with analogous functions introduced in C11 standard that supports length arguments or provides boundary checks such as 'vsprintf_s'}} expected-warning{{Call to function 'vsprintf' is insecure as it does not provide bounding of the memory buffer. Replace with analogous functions that support length arguments or provides boundary checks such as 'vsprintf_s' in case of C11}}
  vscanf(format, args); // expected-warning{{Using 'vscanf' is depracated as it does not provide bounding of the memory buffer or security checks introduced in the C11 standard. Replace with analogous functions introduced in C11 standard that supports length arguments or provides boundary checks such as 'vscanf_s'}} expected-warning{{Call to function 'vscanf' is insecure as it does not provide bounding of the memory buffer. Replace with analogous functions that support length arguments or provides boundary checks such as 'vscanf_s' in case of C11}}
  vfscanf(file, format, args); // expected-warning{{Using 'vfscanf' is depracated as it does not provide bounding of the memory buffer or security checks introduced in the C11 standard. Replace with analogous functions introduced in C11 standard that supports length arguments or provides boundary checks such as 'vfscanf_s'}} expected-warning{{Call to function 'vfscanf' is insecure as it does not provide bounding of the memory buffer. Replace with analogous functions that support length arguments or provides boundary checks such as 'vfscanf_s' in case of C11}}
  vsscanf("a", format, args); // expected-warning{{Using 'vsscanf' is depracated as it does not provide bounding of the memory buffer or security checks introduced in the C11 standard. Replace with analogous functions introduced in C11 standard that supports length arguments or provides boundary checks such as 'vsscanf_s'}} expected-warning{{Call to function 'vsscanf' is insecure as it does not provide bounding of the memory buffer. Replace with analogous functions that support length arguments or provides boundary checks such as 'vsscanf_s' in case of C11}}
  vsnprintf("a", 1, format, args); // expected-warning{{Using 'vsnprintf' is depracated as it does not provide bounding of the memory buffer or security checks introduced in the C11 standard. Replace with analogous functions introduced in C11 standard that supports length arguments or provides boundary checks such as 'vsnprintf_s'}}
}

void test_deprecated_or_unsafe_buffer_handling_3(const wchar_t *format, ...) {
  wchar_t wbuf [5];
  FILE *file;
  va_list args;
  va_start(args, format);
  vwscanf(format, args); // expected-warning{{Using 'vwscanf' is depracated as it does not provide bounding of the memory buffer or security checks introduced in the C11 standard. Replace with analogous functions introduced in C11 standard that supports length arguments or provides boundary checks such as 'vwscanf_s'}} expected-warning{{Call to function 'vwscanf' is insecure as it does not provide bounding of the memory buffer. Replace with analogous functions that support length arguments or provides boundary checks such as 'vwscanf_s' in case of C11}}
  vfwscanf(file, format, args); // expected-warning{{Using 'vfwscanf' is depracated as it does not provide bounding of the memory buffer or security checks introduced in the C11 standard. Replace with analogous functions introduced in C11 standard that supports length arguments or provides boundary checks such as 'vfwscanf_s'}} expected-warning{{Call to function 'vfwscanf' is insecure as it does not provide bounding of the memory buffer. Replace with analogous functions that support length arguments or provides boundary checks such as 'vfwscanf_s' in case of C11}}
  vswscanf(L"a", format, args); // expected-warning{{Using 'vswscanf' is depracated as it does not provide bounding of the memory buffer or security checks introduced in the C11 standard. Replace with analogous functions introduced in C11 standard that supports length arguments or provides boundary checks such as 'vswscanf_s'}} expected-warning{{Call to function 'vswscanf' is insecure as it does not provide bounding of the memory buffer. Replace with analogous functions that support length arguments or provides boundary checks such as 'vswscanf_s' in case of C11}}
  vswprintf(L"a", 1, format, args); // expected-warning{{Using 'vswprintf' is depracated as it does not provide bounding of the memory buffer or security checks introduced in the C11 standard. Replace with analogous functions introduced in C11 standard that supports length arguments or provides boundary checks such as 'vswprintf_s'}}
}
