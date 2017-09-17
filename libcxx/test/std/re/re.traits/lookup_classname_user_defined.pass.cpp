//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// REQUIRES: locale.ja_JP.UTF-8

// <regex>

// template <class charT> struct regex_traits;

// template <class ForwardIterator>
//   char_class_type
//   lookup_classname(ForwardIterator first, ForwardIterator last,
//                    bool icase = false) const;

#include <regex>
#include <cassert>
#include "test_macros.h"
#include "platform_support.h" // locale name macros

struct wctype_traits : std::regex_traits<wchar_t>
{
    using char_class_type = std::wctype_t;
    template<class ForwardIt>
    char_class_type lookup_classname(ForwardIt first, ForwardIt last, bool icase = false ) const {
        (void)icase;
        return std::wctype(std::string(first, last).c_str());
    }
    bool isctype(wchar_t c, char_class_type f) const {
        return std::iswctype(c, f);
    }
};

int main()
{
    std::locale::global(std::locale("ja_JP.utf8"));
    std::wsmatch m;
    std::wstring in = L"風の谷のナウシカ";

    // matches all characters (they are classified as alnum)
    std::wstring re1 = L"([[:alnum:]]+)";
    std::regex_search(in, m, std::wregex(re1));
    assert(m[1] == L"風の谷のナウシカ");

    // matches only the kanji
    std::wstring re2 = L"([[:jkata:]]+)";
    std::regex_search(in, m, std::basic_regex<wchar_t, wctype_traits>(re2));
    assert(m[1] == L"ナウシカ");
}
