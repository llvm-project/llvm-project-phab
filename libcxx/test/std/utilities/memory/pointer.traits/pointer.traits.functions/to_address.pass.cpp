//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// <memory>

// element_type* pointer_traits::to_address(pointer p) noexcept;

#include <memory>
#include <cassert>

struct P1
{
  typedef int element_type;
  int* value;
  explicit P1(int* ptr) noexcept : value(ptr) { }
  int* operator->() const noexcept { return value; }
};

struct P2
{
  typedef P1::element_type element_type;
  P1 value;
  explicit P2(P1 ptr) noexcept : value(ptr) { }
  P1 operator->() const noexcept { return value; }
};

int main()
{
    int i = 0;
    P1 p1(&i);
    assert(std::pointer_traits<P1>::to_address(p1) == &i);
    P2 p2(p1);
    assert(std::pointer_traits<P2>::to_address(p2) == &i);
}
