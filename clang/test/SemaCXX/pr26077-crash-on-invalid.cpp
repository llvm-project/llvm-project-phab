// RUN: %clang_cc1 -fsyntax-only -verify -std=c++11 %s
// Don't crash (PR26077).

class DB {};

template <typename T> class RemovePolicy : public T {};

template <typename T, typename Policy = RemovePolicy<T>>
  class Crash : T, Policy {};

template <typename Policy>
class Crash<DB> : public DB, public Policy { // expected-error {{partial specialization of 'Crash' does not use any of its template parameters}}
public:
  Crash(){}
};
