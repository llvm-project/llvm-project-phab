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

void f1() {{ (void) __func__;;; }}
