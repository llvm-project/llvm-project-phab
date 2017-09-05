// RUN: %check_clang_tidy %s readability-suspicious-call-argument %t -- -- -std=c++11

void foo_1(int aaaaaa, int bbbbbb) {}

void foo_2(int source, int aaaaaa) {}

void foo_3(int valToRet, int aaaaaa) {}

void foo_4(int pointer, int aaaaaa) {}

void foo_5(int aaaaaa, int bbbbbb, int cccccc, ...) {}

void foo_6(const int dddddd, bool &eeeeee) {}

void foo_7(int aaaaaa, int bbbbbb, int cccccc, int ffffff = 7) {}

// Test functions for convertible argument--parameter types.
void fun(const int &m);
void fun2() {
  int m = 3;
  fun(m);
}

// Test cases for parameters of const reference and value.
void value_const_reference(int llllll, const int &kkkkkk);

void const_ref_value_swapped() {
  const int &kkkkkk = 42;
  const int &llllll = 42;
  value_const_reference(kkkkkk, llllll);
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: kkkkkk (llllll) is swapped with llllll (kkkkkk). [readability-suspicious-call-argument]
}

// Const, non const references.
void const_nonconst_parameters(const int &mmmmmm, int &nnnnnn);

void const_nonconst_swap1() {
  const int& nnnnnn = 42;
  int mmmmmm;
  // Do not check, because non-const reference parameter cannot bind to const reference argument.
  const_nonconst_parameters(nnnnnn, mmmmmm);
}

void const_nonconst_swap3() {
  const int nnnnnn = 42;
  int m = 42;
  int &mmmmmm = m;
  // Do not check, const int does not bind to non const reference.
  const_nonconst_parameters(nnnnnn, mmmmmm);
}

void const_nonconst_swap2() {
  int nnnnnn;
  int mmmmmm;
  // Check for swapped arguments. (Both arguments are non-const.)
  const_nonconst_parameters(
      nnnnnn,
      mmmmmm);
  // CHECK-MESSAGES: :[[@LINE-3]]:3: warning: nnnnnn (mmmmmm) is swapped with mmmmmm (nnnnnn). [readability-suspicious-call-argument]
}

void const_nonconst_pointers(const int* mmmmmm, int* nnnnnn);
void const_nonconst_pointers2(const int* mmmmmm, const int* nnnnnn);

void const_nonconst_pointers_swapped(){
    int* mmmmmm;
    const int* nnnnnn;
    const_nonconst_pointers(nnnnnn, mmmmmm);
}

void const_nonconst_pointers_swapped2() {
  const int *mmmmmm;
  int *nnnnnn;
  const_nonconst_pointers2(nnnnnn, mmmmmm);
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: nnnnnn (mmmmmm) is swapped with mmmmmm (nnnnnn). [readability-suspicious-call-argument]
}

// Test cases for pointers and arrays.
void pointer_array_parameters(
    int *pppppp, int qqqqqq[4]);

void pointer_array_swap() {
  int qqqqqq[5];
  int *pppppp;
  // Check for swapped arguments. An array implicitly converts to a pointer.
  pointer_array_parameters(qqqqqq, pppppp);
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: qqqqqq (pppppp) is swapped with pppppp (qqqqqq). [readability-suspicious-call-argument]
}

// Test cases for multilevel pointers.
void multilevel_pointer_parameters(int *const **pppppp,
                                   const int *const *volatile const *qqqqqq);
void multilevel_pointer_parameters2(
    char *****nnnnnn, char *volatile *const *const *const *const &mmmmmm);

typedef float T;
typedef T * S;
typedef S *const volatile R;
typedef R * Q;
typedef Q * P;
typedef P * O;
void multilevel_pointer_parameters3(float **const volatile ***rrrrrr, O &ssssss);

void multilevel_pointer_swap() {
  int *const ** qqqqqq;
  int *const ** pppppp;
  multilevel_pointer_parameters(qqqqqq, pppppp);
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: qqqqqq (pppppp) is swapped with pppppp (qqqqqq). [readability-suspicious-call-argument]

  char *****mmmmmm;
  char *****nnnnnn;
  multilevel_pointer_parameters2(mmmmmm, nnnnnn);
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: mmmmmm (nnnnnn) is swapped with nnnnnn (mmmmmm). [readability-suspicious-call-argument]

  float **const volatile *** rrrrrr;
  float **const volatile *** ssssss;
  multilevel_pointer_parameters3(ssssss, rrrrrr);
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: ssssss (rrrrrr) is swapped with rrrrrr (ssssss). [readability-suspicious-call-argument]
}

void multilevel_pointer_parameters4(char ****pppppp,
                                    char *const volatile **const *qqqqqq);
void multilevel_pointer_parameters5(
    bool *****nnnnnn, bool *volatile *const *const *const *&mmmmmm);
void multilevel_pointer_parameters6(double **llllll, char **&kkkkkk);
void multilevel_pointer_parameters7(const volatile int ***iiiiii,
                                    const int *const *const *jjjjjj);

void multilevel_pointer_swap3() {
  char ****qqqqqq;
  char *const volatile **const *pppppp;
  // Do not check.
  multilevel_pointer_parameters4(qqqqqq, pppppp);

  bool *****mmmmmm;
  bool *volatile *const *const *const *nnnnnn;
  // Do not check.
  multilevel_pointer_parameters5(mmmmmm, nnnnnn);

  double **kkkkkk;
  char **llllll;
  multilevel_pointer_parameters6(kkkkkk, llllll);

  const volatile int ***jjjjjj;
  const int *const *const *iiiiii;
  multilevel_pointer_parameters7(jjjjjj, iiiiii);
}

// Test cases for multidimesional arrays.
void multilevel_array_parameters(int pppppp [2][2][2], const int qqqqqq[][2][2]);

void multilevel_array_parameters2(int (*mmmmmm)[2][2], int nnnnnn[9][2][23]);

void multilevel_array_parameters3(int (*eeeeee)[2][2], int (&ffffff)[1][2][2]);

void multilevel_array_parameters4(int (*llllll)[2][2], int kkkkkk[2][2]);

void multilevel_array_parameters5(int iiiiii[2][2], char jjjjjj[2][2]);

void multilevel_array_parameters6(int (*bbbbbb)[2][2], int cccccc[1][2][2]);

void multilevel_array_swap() {
  int qqqqqq[1][2][2];
  int pppppp[][2][2] = {{{1, 2}, {1, 2}}, {{1, 2}, {1, 2}}}; // int [2][2][2]
  multilevel_array_parameters(qqqqqq, pppppp);
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: qqqqqq (pppppp) is swapped with pppppp (qqqqqq). [readability-suspicious-call-argument]

  int(*nnnnnn)[2][2];
  int mmmmmm[9][2][23];
  // Do not check, array sizes has to match in every dimension, except the first.
  multilevel_array_parameters2(nnnnnn, mmmmmm);

  int ffffff[][2][2] = {{{1, 2}, {1, 2}}, {{1, 2}, {1, 2}}}; // int [2][2][2]
  int eeeeee[1][2][2] = {{{1, 2}, {1, 2}}}; // int [1][2][2]
  // Do not check, for array references, size has to match in every dimension.
  multilevel_array_parameters3(ffffff, eeeeee);

  int kkkkkk[2][2][2];
  int (*llllll)[2];
  // Do not check, argument dimensions differ.
  multilevel_array_parameters4(kkkkkk, llllll);

  int jjjjjj[2][2];
  char iiiiii[2][2];
  // Do not check, array element types differ.
  multilevel_array_parameters5(jjjjjj, iiiiii);

  int t[][2][2] = {{{1, 2}, {1, 2}}, {{1, 2}, {1, 2}}}; // int [2][2][2]
  int (*cccccc)[2][2] = t; // int (*)[2][2]
  int bbbbbb[][2][2] =  {{{1, 2}, {1, 2}}}; // int [1][2][2]
  multilevel_array_parameters6(cccccc, bbbbbb);
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: cccccc (bbbbbb) is swapped with bbbbbb (cccccc). [readability-suspicious-call-argument]
}

void multilevel_array_swap2(){
 int qqqqqq[2][2][2];
 const int pppppp[][2][2] = {{{1, 2}, {1, 2}}, {{1, 2}, {1, 2}}};
 // Do not check, pppppp is const and cannot bind to an array with nonconst elements.
 multilevel_array_parameters(qqqqqq, pppppp);
}

// Complex test case.
void multilevel_pointer_array_parameters(const int (*const (*volatile const (*const  (*const (*const &aaaaaa)[1])[32])[4])[3][2][2]), const int (*const (*volatile const (*const  (*const (* &bbbbbb)[1])[32])[4])[3][2][2]));

void multilevel_pointer_array_swap() {
  const int(
      *const(*volatile const(*const(*const(* aaaaaa)[1])[32])[4])[3][2][2]);
  const int(
      *const(*volatile const(*const(*const(* bbbbbb)[1])[32])[4])[3][2][2]);
  multilevel_pointer_array_parameters(bbbbbb, aaaaaa);
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: bbbbbb (aaaaaa) is swapped with aaaaaa (bbbbbb). [readability-suspicious-call-argument]
}

enum class numbers_scoped { one, two};

// Test cases for arithmetic types.
void arithmetic_type_parameters(float vvvvvv, int wwwwww);
void arithmetic_type_parameters2(numbers_scoped vvvvvv, int wwwwww);

void arithmetic_types_swap1() {
  bool wwwwww;
  float vvvvvv;
  arithmetic_type_parameters(wwwwww, vvvvvv);
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: wwwwww (vvvvvv) is swapped with vvvvvv (wwwwww). [readability-suspicious-call-argument]
}

void arithmetic_types_swap3() {
  char wwwwww;
  unsigned long long int vvvvvv;
  arithmetic_type_parameters(wwwwww, vvvvvv);
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: wwwwww (vvvvvv) is swapped with vvvvvv (wwwwww). [readability-suspicious-call-argument]
}

void arithmetic_types_swap4() {
  enum numbers { one, two};
  numbers wwwwww = numbers::one;
  int vvvvvv;
  arithmetic_type_parameters(wwwwww, vvvvvv);
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: wwwwww (vvvvvv) is swapped with vvvvvv (wwwwww). [readability-suspicious-call-argument]
}

void arithmetic_types_swap5() {
  wchar_t vvvvvv;
  float wwwwww;
  arithmetic_type_parameters(wwwwww, vvvvvv);
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: wwwwww (vvvvvv) is swapped with vvvvvv (wwwwww). [readability-suspicious-call-argument]
}

void arithmetic_types_swap6() {
  wchar_t vvvvvv;
  numbers_scoped wwwwww = numbers_scoped::one;
  // Do not check, numers is a scoped enum type.
  arithmetic_type_parameters2(wwwwww, vvvvvv);
}

// Base, derived
class TestClass {
public:
  void thisFunction(int integerParam, int thisIsPARAM) {}
};

class DerivedTestClass : public TestClass {};

void base_derived_pointer_parameters(TestClass *aaaaaa,
                                     DerivedTestClass *bbbbbb);

void base_derived_swap1() {
  TestClass *bbbbbb;
  DerivedTestClass *aaaaaa;
  // Do not check, because TestClass does not convert implicitly to DerivedTestClass.
  base_derived_pointer_parameters(bbbbbb, aaaaaa);
}

void base_derived_swap2() {
  DerivedTestClass *bbbbbb, *aaaaaa;
  // Check for swapped arguments, DerivedTestClass converts to TestClass implicitly.
  base_derived_pointer_parameters(bbbbbb, aaaaaa);
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: bbbbbb (aaaaaa) is swapped with aaaaaa (bbbbbb). [readability-suspicious-call-argument]
}

// Multilevel inheritance
class DerivedOfDerivedTestClass : public DerivedTestClass {};

void multi_level_inheritance_swap() {
  DerivedOfDerivedTestClass *aaaaaa, *bbbbbb;
  // Check for swapped arguments. Derived classes implicitly convert to their base.
  base_derived_pointer_parameters(
      bbbbbb, aaaaaa);
  // CHECK-MESSAGES: :[[@LINE-2]]:3: warning: bbbbbb (aaaaaa) is swapped with aaaaaa (bbbbbb). [readability-suspicious-call-argument]
}

// Tests for function pointer swaps
void funct_ptr_params(double (*ffffff)(int, int), double (*gggggg)(int, int));
void funct_ptr_params(double (*ffffff)(int, int), int (*gggggg)(int, int));

double ffffff(int a, int b) { return 0; }
double gggggg(int a, int b) { return 0; }

void funtionc_ptr_params_swap() {
  funct_ptr_params(gggggg, ffffff);
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: gggggg (ffffff) is swapped with ffffff (gggggg). [readability-suspicious-call-argument]
}

int fffff(int a, int b) { return 0; }

void function_ptr_swap2() {
  // Do not check, because the function `ffffff` cannot convert to a function
  // with prototype: double(int,int).
  funct_ptr_params(gggggg, fffff);
}

int main() {

  // Equality test.
  int aaaaaa, cccccc = 0;
  foo_1(cccccc, aaaaaa); // Should fail.
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: cccccc (aaaaaa) is swapped with aaaaaa (bbbbbb). [readability-suspicious-call-argument]

  // Abbreviation test.
  int src = 0;
  foo_2(aaaaaa, src); // Should fail.
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: aaaaaa (source) is swapped with src (aaaaaa). [readability-suspicious-call-argument]

  // Levenshtein test.
  int aaaabb = 0;
  foo_1(cccccc, aaaabb); // Should fail.
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: cccccc (aaaaaa) is swapped with aaaabb (bbbbbb). [readability-suspicious-call-argument]

  // Prefix test.
  int aaaa = 0;
  foo_1(cccccc, aaaa); // Should fail.
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: cccccc (aaaaaa) is swapped with aaaa (bbbbbb). [readability-suspicious-call-argument]

  // Suffix test.
  int urce = 0;
  foo_2(cccccc, urce); // Should fail.
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: cccccc (source) is swapped with urce (aaaaaa). [readability-suspicious-call-argument]

  // Substring test.
  int ourc = 0;
  foo_2(cccccc, ourc); // Should fail.
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: cccccc (source) is swapped with ourc (aaaaaa). [readability-suspicious-call-argument]

  // Jaro-Winkler test.
  int iPonter = 0;
  foo_4(cccccc, iPonter); // Should fail.
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: cccccc (pointer) is swapped with iPonter (aaaaaa). [readability-suspicious-call-argument]

  // Dice test.
  int aaabaa = 0;
  foo_1(cccccc, aaabaa); // Should fail.
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: cccccc (aaaaaa) is swapped with aaabaa (bbbbbb). [readability-suspicious-call-argument]

  // Variadic function test.
  int bbbbbb = 0;
  foo_5(src, bbbbbb, cccccc, aaaaaa); // Should pass.
  foo_5(cccccc, bbbbbb, aaaaaa, src); // Should fail.
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: cccccc (aaaaaa) is swapped with aaaaaa (cccccc). [readability-suspicious-call-argument]

  // Test function with default argument.
  foo_7(src, bbbbbb, cccccc, aaaaaa); // Should fail.
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: src (aaaaaa) is swapped with aaaaaa (ffffff). [readability-suspicious-call-argument]

  foo_7(cccccc, bbbbbb, aaaaaa, src); // Should fail.
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: cccccc (aaaaaa) is swapped with aaaaaa (cccccc). [readability-suspicious-call-argument]

  int ffffff = 0;
  foo_7(ffffff, bbbbbb, cccccc); // Should fail.

  // Type match
  bool dddddd = false;
  int eeeeee = 0;
  auto szam = 0;
  foo_6(eeeeee, dddddd); // Should fail.
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: eeeeee (dddddd) is swapped with dddddd (eeeeee). [readability-suspicious-call-argument]
  foo_1(szam, aaaaaa);   // Should fail.
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: szam (aaaaaa) is swapped with aaaaaa (bbbbbb). [readability-suspicious-call-argument]

  // Test lambda.
  auto testMethod = [&](int method, int randomParam) { return 0; };
  int method = 0;
  testMethod(method, 0); // Should pass.

  // Member function test
  TestClass test;
  int integ, thisIsAnArg = 0;
  test.thisFunction(integ, thisIsAnArg); // Should pass.

  return 0;
}

