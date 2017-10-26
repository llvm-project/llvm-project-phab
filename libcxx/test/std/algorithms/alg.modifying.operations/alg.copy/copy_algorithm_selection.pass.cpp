//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// <algorithm>

// template<InputIterator InIter, OutputIterator<auto, InIter::reference> OutIter>
//   OutIter
//   copy(InIter first, InIter last, OutIter result);

#include <algorithm>
#include <cassert>
#include <iterator>

#include "test_iterators.h"

namespace {

struct debug
{
    static constexpr bool __is_debug = true;
};

struct release
{
    static constexpr bool __is_debug = false;
};

template <class Mode>
struct default_test_traits : Mode
{
    template <bool, class InIter, class OutIter>
    static OutIter __memmove(InIter, InIter, OutIter o) {
        assert(false);
        return o;
    }

    template <bool, class InIter, class OutIter>
    static OutIter __loop(InIter, InIter, OutIter o) {
        assert(false);
        return o;
    }
};

template <class Mode, bool should_be_backward>
struct memmove_traits : default_test_traits<Mode>
{
    template <bool is_backward, class InIter, class OutIter>
    static OutIter __memmove(InIter, InIter, OutIter o)
    {
        assert(is_backward == should_be_backward);
        return o;
    }
};

template <class Mode, bool should_be_backward>
struct loop_traits : default_test_traits<Mode>
{
    template <bool is_backward, class InIter, class OutIter>
    static OutIter __loop(InIter, InIter, OutIter o)
    {
        assert(is_backward == should_be_backward);
        return o;
    }
};

template <class Mode>
using should_mmove_fwd = memmove_traits<Mode, false>;

template <class Mode>
using should_mmove_backward = memmove_traits<Mode, true>;

template <class Mode>
using should_loop_fwd = loop_traits<Mode, false>;

template <class Mode>
using should_loop_backward = loop_traits<Mode, true>;

template <bool is_backward, class Traits, class Ip, class Op>
void test()
{
  Op res = _VSTD::__do_copy<is_backward, Traits>(Ip(), Ip(), Op());
  (void)res;
}

}  // namespace

int main()
{
    constexpr bool fwd = false;
    constexpr bool backward = true;

    {
        typedef int* I;
        typedef int* O;

        test<fwd, should_mmove_fwd<debug>, I, O>();
        test<fwd, should_mmove_fwd<release>, I, O>();
        test<backward, should_mmove_backward<debug>, I, O>();
        test<backward, should_mmove_backward<release>, I, O>();
    }

    {
        typedef const int* I;
        typedef int* O;

        test<fwd, should_mmove_fwd<debug>, I, O>();
        test<fwd, should_mmove_fwd<release>, I, O>();
        test<backward, should_mmove_backward<debug>, I, O>();
        test<backward, should_mmove_backward<release>, I, O>();
    }

    {
        typedef std::move_iterator<int*> I;
        typedef int* O;

        test<fwd, should_mmove_fwd<debug>, I, O>();
        test<fwd, should_mmove_fwd<release>, I, O>();
        test<backward, should_mmove_backward<debug>, I, O>();
        test<backward, should_mmove_backward<release>, I, O>();
    }

    {
        typedef std::move_iterator<const int*> I;
        typedef int* O;

        test<fwd, should_mmove_fwd<debug>, I, O>();
        test<fwd, should_mmove_fwd<release>, I, O>();
        test<backward, should_mmove_backward<debug>, I, O>();
        test<backward, should_mmove_backward<release>, I, O>();
    }

    {
        typedef std::move_iterator<std::move_iterator<const int*>> I;
        typedef int* O;

        test<fwd, should_mmove_fwd<debug>, I, O>();
        test<fwd, should_mmove_fwd<release>, I, O>();
        test<backward, should_mmove_backward<debug>, I, O>();
        test<backward, should_mmove_backward<release>, I, O>();
    }

    {
        typedef int* I;
        typedef _VSTD::__wrap_iter<int*> O;

        test<fwd, should_loop_fwd<debug>, I, O>();
        test<fwd, should_mmove_fwd<release>, I, O>();
        test<backward, should_loop_backward<debug>, I, O>();
        test<backward, should_mmove_backward<release>, I, O>();
    }

    {
        typedef _VSTD::__wrap_iter<int*> I;
        typedef _VSTD::__wrap_iter<int*> O;

        test<fwd, should_loop_fwd<debug>, I, O>();
        test<fwd, should_mmove_fwd<release>, I, O>();
        test<backward, should_loop_backward<debug>, I, O>();
        test<backward, should_mmove_backward<release>, I, O>();
    }

    {
        typedef std::move_iterator<_VSTD::__wrap_iter<int*>> I;
        typedef _VSTD::__wrap_iter<int*> O;

        test<fwd, should_loop_fwd<debug>, I, O>();
        test<fwd, should_mmove_fwd<release>, I, O>();
        test<backward, should_loop_backward<debug>, I, O>();
        test<backward, should_mmove_backward<release>, I, O>();
    }

    {
        typedef std::reverse_iterator<int*> I;
        typedef int* O;

        test<fwd, should_loop_fwd<debug>, I, O>();
        test<fwd, should_loop_fwd<release>, I, O>();
        test<backward, should_loop_backward<debug>, I, O>();
        test<backward, should_loop_backward<release>, I, O>();
    }

    {
        typedef std::reverse_iterator<bidirectional_iterator<int*>> I;
        typedef std::reverse_iterator<int*> O;

        test<fwd, should_loop_backward<debug>, I, O>();
        test<fwd, should_loop_backward<release>, I, O>();
        test<backward, should_loop_fwd<debug>, I, O>();
        test<backward, should_loop_fwd<release>, I, O>();
    }

    {
        typedef std::reverse_iterator<int*> I;
        typedef std::reverse_iterator<int*> O;

        test<fwd, should_mmove_backward<debug>, I, O>();
        test<fwd, should_mmove_backward<release>, I, O>();
        test<backward, should_mmove_fwd<debug>, I, O>();
        test<backward, should_mmove_fwd<release>, I, O>();
    }

    {
        typedef std::reverse_iterator<std::move_iterator<std::reverse_iterator<int*>>> I;
        typedef std::reverse_iterator<std::reverse_iterator<int*>> O;

        test<fwd, should_mmove_fwd<debug>, I, O>();
        test<fwd, should_mmove_fwd<release>, I, O>();
        test<backward, should_mmove_backward<debug>, I, O>();
        test<backward, should_mmove_backward<release>, I, O>();
    }

    {
        typedef std::move_iterator<std::reverse_iterator<_VSTD::__wrap_iter<int*>>> I;
        typedef std::reverse_iterator<_VSTD::__wrap_iter<int*>> O;

        test<fwd, should_loop_backward<debug>, I, O>();
        test<fwd, should_mmove_backward<release>, I, O>();
        test<backward, should_loop_fwd<debug>, I, O>();
        test<backward, should_mmove_fwd<release>, I, O>();
    }
}

