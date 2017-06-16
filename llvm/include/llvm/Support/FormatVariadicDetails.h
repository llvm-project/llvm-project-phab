//===- FormatVariadicDetails.h - Helpers for FormatVariadic.h ----*- C++-*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_SUPPORT_FORMATVARIADIC_DETAILS_H
#define LLVM_SUPPORT_FORMATVARIADIC_DETAILS_H

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"

#include <type_traits>

namespace llvm {
template <typename T, typename Enable = void> struct format_provider {};

namespace detail {
class format_adapter {
protected:
  virtual ~format_adapter() {}

public:
  virtual void format(raw_ostream &S, StringRef Options) = 0;
};

template <typename T> class value_holder : public format_adapter {
  T Item;
public:
  explicit value_holder(T &&Item) : Item(Item) {}
  virtual void format(raw_ostream &S, StringRef Options) {
    format_one_item(std::forward<T>(Item), S, Options);
  }
};

template <typename T> class missing_format_adapter;

// Test if format_provider<T> is defined on T and contains a member function
// with the signature:
//   static void format(const T&, raw_stream &, StringRef);
//
template <class T> class has_FormatProvider {
public:
  using Decayed = typename std::decay<T>::type;
  typedef void (*Signature_format)(const Decayed &, llvm::raw_ostream &,
                                   StringRef);

  template <typename U>
  static char test(SameType<Signature_format, &U::format> *);

  template <typename U> static double test(...);

  static bool const value =
      (sizeof(test<llvm::format_provider<Decayed>>(nullptr)) == 1);
};

// Simple template that decides whether a type T should use the member-function
// based format() invocation.
template <typename T>
struct uses_format_member
    : public std::integral_constant<
          bool,
          std::is_base_of<format_adapter,
                          typename std::remove_reference<T>::type>::value> {};

// Simple template that decides whether a type T should use the format_provider
// based format() invocation.  The member function takes priority, so this test
// will only be true if there is not ALSO a format member.
template <typename T>
struct uses_format_provider
    : public std::integral_constant<
          bool, !uses_format_member<T>::value && has_FormatProvider<T>::value> {
};
}

template <typename T>
typename std::enable_if<detail::uses_format_member<T>::value, void>::type
format_one_item(T &&Item, raw_ostream &S, StringRef Options) {
  Item.format(S, Options);
}

template <typename T>
typename std::enable_if<detail::uses_format_provider<T>::value, void>::type
format_one_item(T &&Item, raw_ostream &S, StringRef Options) {
  format_provider<typename std::decay<T>::type>::format(std::forward<T>(Item),
                                                        S, Options);
}
}

#endif
