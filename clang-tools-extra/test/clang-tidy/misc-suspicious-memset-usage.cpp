// RUN: %check_clang_tidy %s misc-suspicious-memset-usage %t

void *memset(void *, int, __SIZE_TYPE__);

template <typename T>
void mtempl() {
  memset(0, '0', sizeof(T));
  // CHECK-MESSAGES: :[[@LINE-1]]:13: warning: memset fill value is char '0', potentially mistaken for int 0 [misc-suspicious-memset-usage]
  // CHECK-FIXES: memset(0, 0, sizeof(T));
  memset(0, 256, sizeof(T));
  // CHECK-MESSAGES: :[[@LINE-1]]:13: warning: memset fill value is out of unsigned character range, gets truncated [misc-suspicious-memset-usage]
}

void BadFillValue() {

  int i[5] = {1, 2, 3, 4, 5};
  int *p = i;
  int l = 5;
  char z = '1';
  char *c = &z;

  memset(p, '0', l);
  // CHECK-MESSAGES: :[[@LINE-1]]:13: warning: memset fill value is char '0', potentially mistaken for int 0 [misc-suspicious-memset-usage]
  // CHECK-FIXES: memset(p, 0, l);
#define M memset(p, '0', l);
  M
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: memset fill value is char '0', potentially mistaken for int 0 [misc-suspicious-memset-usage]

  // Valid expressions:
  memset(p, '2', l);
  memset(p, 0, l);
  memset(c, '0', 1);

  memset(p, 0xabcd, l);
  // CHECK-MESSAGES: :[[@LINE-1]]:13: warning: memset fill value is out of unsigned character range, gets truncated [misc-suspicious-memset-usage]

  // Valid expressions:
  memset(p, 0x00, l);
  mtempl<int>();
}

class A {
  virtual void a1() {}
  void a2() {
    memset(this, 0, sizeof(*this));
    // CHECK-MESSAGES: :[[@LINE-1]]:12: warning: memset used on this pointer potentially corrupts virtual method table [misc-suspicious-memset-usage]
  }
  A() {
    memset(this, 0, sizeof(*this));
    // CHECK-MESSAGES: :[[@LINE-1]]:12: warning: memset used on this pointer potentially corrupts virtual method table [misc-suspicious-memset-usage]
  }
};

class B : public A {
  void b1() {}
  void b2() {
    memset(this, 0, sizeof(*this));
    // CHECK-MESSAGES: :[[@LINE-1]]:12: warning: memset used on this pointer potentially corrupts virtual method table [misc-suspicious-memset-usage]
  }
};

// Valid expressions:
class C {
  void c1() {}
  void c2() {
    memset(this, 0, sizeof(*this));
  }
  C() {
    memset(this, 0, sizeof(*this));
  }
};

class D {
  virtual void d1() {}
  void d2() {
    [this] { memset(this, 0, sizeof(*this)); };
    // CHECK-MESSAGES: :[[@LINE-1]]:21: warning: memset used on this pointer potentially corrupts virtual method table [misc-suspicious-memset-usage]
  }
  D() {
    [this] { memset(this, 0, sizeof(*this)); };
    // CHECK-MESSAGES: :[[@LINE-1]]:21: warning: memset used on this pointer potentially corrupts virtual method table [misc-suspicious-memset-usage]
  }
};

// Valid expressions:
class E {
  virtual void e1() {}
  E() {
    struct S1 {
      void s1() { memset(this, 0, sizeof(*this)); }
    };
  }
  struct S2 {
    void s2() { memset(this, 0, sizeof(*this)); }
  };
  void e2() {
    struct S3 {
      void s3() { memset(this, 0, sizeof(*this)); }
    };
  }
};
