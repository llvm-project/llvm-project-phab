//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// <memory>

// element_type* pointer_traits<T*>::to_address(pointer p) noexcept;

#include <memory>
#include <cassert>

int main()
{
    int i = 0;
    assert(std::pointer_traits<int*>::to_address(&i) == &i);
    assert(std::pointer_traits<const int*>::to_address(&i) == &i);
    assert(std::pointer_traits<void*>::to_address(&i) == &i);
}
