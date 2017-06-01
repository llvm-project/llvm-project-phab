//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// <ostream>

// template <class charT, class traits = char_traits<charT> >
//   class basic_ostream;

// operator<<(nullptr_t);

#include <ostream>
#include <cstddef>
#include <cassert>

template <class CharT>
class testbuf
    : public std::basic_streambuf<CharT>
{
    typedef std::basic_streambuf<CharT> base;
    std::basic_string<CharT> str_;
public:
    testbuf()
    {
    }

    std::basic_string<CharT> str() const
        {return std::basic_string<CharT>(base::pbase(), base::pptr());}

protected:

    virtual typename base::int_type
        overflow(typename base::int_type __c = base::traits_type::eof())
        {
            if (__c != base::traits_type::eof())
            {
                int n = static_cast<int>(str_.size());
                str_.push_back(static_cast<CharT>(__c));
                str_.resize(str_.capacity());
                base::setp(const_cast<CharT*>(str_.data()),
                           const_cast<CharT*>(str_.data() + str_.size()));
                base::pbump(n+1);
            }
            return __c;
        }
};

int main()
{
    {
        std::ostream os((std::streambuf*)0);
        std::nullptr_t n = nullptr;
        os << n;
        assert(os.bad());
        assert(os.fail());
    }
    {
        testbuf<char> sb;
        std::ostream os(&sb);
        std::nullptr_t n = nullptr;
        os << n;
        assert(os.good());
        std::string s(sb.str());

        // Implementation defined. Instead of validating the output,
        // at least ensure that it does not generate an empty string.
        assert(!s.empty());
    }
    {
        testbuf<char> sb;
        std::ostream os(&sb);
        std::nullptr_t const n = nullptr;
        os << n;
        assert(os.good());
    }
}
