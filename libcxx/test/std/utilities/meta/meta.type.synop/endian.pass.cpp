//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++98, c++03, c++14, c++17

// enum class endian;

#include <type_traits>
#include <cassert>

#include "test_macros.h"

int main() {
    typedef std::endian E;
    static_assert(std::is_enum<std::endian>::value, "");

// Check that E is a scoped enum by checking for conversions.
    typedef std::underlying_type<std::endian>::type UT;
    static_assert(!std::is_convertible<std::endian, UT>::value, "");

// test that the enumeration values exist
    static_assert( std::endian::little == std::endian::little );
    static_assert( std::endian::big    == std::endian::big );
    static_assert( std::endian::native == std::endian::native );
    static_assert( std::endian::little != std::endian::big );

//  Technically not required, but true on all existing machines
    static_assert( std::endian::native == std::endian::little || 
                   std::endian::native == std::endian::big );
    
//  Try to check at runtime
    {
    union {
        uint32_t i;
        char c[4];
    } u = {0x01020304};

    assert ((u.c[0] == 1) == (std::endian::native == std::endian::big));
    }
}
