// RUN: %clang_cc1 -fsyntax-only -verify %s

// This file contains typo correction test which ensures that
// multiple typos in a single member calls chain are correctly
// diagnosed.

class X
{
public:
  void foo() const;  // expected-note {{'foo' declared here}}
};

class Y
{
public:
  const X& getX() const { return m_x; } // expected-note {{'getX' declared here}}
  int getN() const { return m_n; }
private:
  X m_x;
  int m_n;
};

class Z
{
public:
  const Y& getY0() const { return m_y0; } // expected-note {{'getY0' declared here}}
  const Y& getY1() const { return m_y1; }

private:
  Y m_y0;
  Y m_y1;
};

Z z_obj;

void test()
{
  z_obj.getY2(). // expected-error {{no member named 'getY2' in 'Z'; did you mean 'getY0'}}
    getM().      // expected-error {{no member named 'getM' in 'Y'; did you mean 'getX'}}
    foe();       // expected-error {{no member named 'foe' in 'X'; did you mean 'foo'}}
}
