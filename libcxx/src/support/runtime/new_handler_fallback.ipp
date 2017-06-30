// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

namespace std {

_LIBCPP_SAFE_STATIC static std::new_handler __new_handler;

new_handler
set_new_handler(new_handler handler) _NOEXCEPT
{
#ifdef _LIBCPP_HAS_NO_THREADS
    auto old = __new_handler;
    __new_handler = handler;
    return old;
#else
    return __sync_lock_test_and_set(&__new_handler, handler);
#endif
}

new_handler
get_new_handler() _NOEXCEPT
{
#ifdef _LIBCPP_HAS_NO_THREADS
    return __new_handler;
#else
    return __sync_fetch_and_add(&__new_handler, nullptr);
#endif
}

} // namespace std
