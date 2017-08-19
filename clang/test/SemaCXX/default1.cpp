// RUN: %clang_cc1 -fsyntax-only -verify %s
// RUN: %clang_cc1 -fsyntax-only -verify -std=c++98 %s
// RUN: %clang_cc1 -fsyntax-only -verify -std=c++11 %s

void f(int i);
void f(int i = 0); // expected-note {{previous definition is here}}
void f(int i = 17); // expected-error {{redefinition of default argument}}


void g(int i, int j, int k = 3);
void g(int i, int j = 2, int k);
void g(int i = 1, int j, int k);

void h(int i, int j = 2, int k = 3, 
       int l, // expected-error {{missing default argument on parameter 'l'}}
       int,   // expected-error {{missing default argument on parameter}}
       int n);// expected-error {{missing default argument on parameter 'n'}}

struct S { } s;
void i(int = s) { } // expected-error {{no viable conversion}} \
// expected-note{{passing argument to parameter here}}

struct X { 
  X(int);
};

void j(X x = 17); // expected-note{{'::j' declared here}}

struct Y { // expected-note 2{{candidate constructor (the implicit copy constructor) not viable}}
#if __cplusplus >= 201103L // C++11 or later
// expected-note@-2 2 {{candidate constructor (the implicit move constructor) not viable}}
#endif

  explicit Y(int);
};

void k(Y y = 17); // expected-error{{no viable conversion}} \
// expected-note{{passing argument to parameter 'y' here}}

void kk(Y = 17); // expected-error{{no viable conversion}} \
// expected-note{{passing argument to parameter here}}

int l () {
  int m(int i, int j, int k = 3);
  if (1)
  {
    int m(int i, int j = 2, int k = 4);
    m(8);
  }
  return 0;
}

int i () {
  void j (int f = 4);
  {
    void j (int f);
    j(); // expected-error{{too few arguments to function call, expected 1, have 0; did you mean '::j'?}}
  }
  void jj (int f = 4);
  {
    void jj (int f); // expected-note{{'jj' declared here}}
    jj(); // expected-error{{too few arguments to function call, single argument 'f' was not specified}}
  }
}

int i2() {
  void j(int f = 4); // expected-note{{'j' declared here}}
  {
    j(2, 3); // expected-error{{too many arguments to function call, expected at most single argument 'f', have 2}}
  }
}

int pr20055_f(int x = 0, int y = UNDEFINED); // expected-error{{use of undeclared identifier}}
int pr20055_v = pr20055_f(0);

void PR20769() { void PR20769(int = 1); }
void PR20769(int = 2);

void PR20769_b(int = 1);
void PR20769_b() { void PR20769_b(int = 2); }

#if __cplusplus >= 201103L // C++11 or later
struct S2 {
  template<class T>
  S2(T&&) {}
};

template<class T>
void func0(int a0, S2 a1 = [](){ (void)&a0; }); // expected-error {{default argument references parameter 'a0'}}

template<class T>
void func1(T a0, int a1, S2 a2 = _Generic((a0), default: [](){ (void)&a1; }, int: 0)); // expected-error {{default argument references parameter 'a1'}}

template<class T>
void func2(S2 a0 = [](){
  int t; [&t](){ (void)&t;}();
});

template<class T>
void func3(int a0, S2 a1 = [](){
  [=](){ (void)&a0;}(); // expected-error {{default argument references parameter 'a0'}}
});

double d;

void test1() {
  int i;
  float f;
  void foo0(int a0 = _Generic((f), double: d, float: f)); // expected-error {{default argument references local variable 'f' of enclosing function}}
  void foo1(int a0 = _Generic((d), double: d, float: f));
  void foo2(int a0 = _Generic((i), int: d, float: f));
  void foo3(int a0 = _Generic((i), default: d, float: f));

  void foo4(S2 a0 = [&](){ (void)&i; }); // expected-error {{lambda expression in default argument cannot capture any entity}}
  void foo5(S2 a0 = [](){
    // No warning about capture list of a lambda expression defined in a block scope.
    int t; [&t](){ (void)&t;}();
  });
  void foo6(int a0, S2 a1 = [](){
    // No warning about local variables or parameters referenced by an
    // unevaluated expression.
    int t = sizeof({i, a0;});
  });
  void foo6(S2 a0 = [](){
    int i; // expected-note {{'i' declared here}}
    void foo7(int a0, // expected-note {{'a0' declared here}}
              S2 a1 = [](){ (void)&a0; }); // expected-error {{variable 'a0' cannot be implicitly captured in a lambda with no capture-default specified}} expected-error {{default argument references parameter 'a0'}} expected-note {{lambda expression begins here}}
    void foo8(S2 a0 = [](){ (void)&i; }); // expected-error {{variable 'i' cannot be implicitly captured in a lambda with no capture-default specified}} expected-error {{default argument references local variable 'i' of enclosing function}} expected-note {{lambda expression begins here}}
  });

  func0<int>(1); // expected-error {{no matching function for call to 'func0'}}
  func1<int>(1); // expected-error {{no matching function for call to 'func1'}}
  func2<int>();
  func3<int>(1); // expected-error {{no matching function for call to 'func3'}}
}

#endif
