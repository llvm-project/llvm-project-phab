namespace src {

void foo() {
  int x = 321;
}

void main() { foo(); };

const char *a = "foo";

typedef unsigned int nat;

int p = 1 * 2 * 3 * 4;
int squared = p * p;

class X {
  const char *foo(int i) {
    if (i == 0)
      return "foo";
    return 0;
  }

public:
  X(){};

  int id(int i) { return i; }
};
}

void m() { int x = 0 + 0 + 0; }
int um = 1 * 2 + 3;

void f1() {{ (void) __func__;;; }}

#define M1 return 1 + 1
#define M2 return 1 + 2
#define F(a, b) return a + b;

int f2() {
  M1;
  M1;
  F(1, 1);
}

template <class T, class U = void>
U visit(T &t) {
  int x = t;
  return U();
}

void tmp() {
  int x;
  visit<int>(x);
}

int x = 1;
