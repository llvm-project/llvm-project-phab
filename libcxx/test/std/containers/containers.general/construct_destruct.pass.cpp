//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <array>
#include <cassert>
#include <deque>
#include <forward_list>
#include <list>
#include <map>
#include <queue>
#include <set>
#include <stack>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// Holder<ns::T> is a do-nothing wrapper around a value of the given type.
//
// Since unqualified name lookup and operator overload lookup participate in
// ADL, the dependent type ns::T contributes to the overload set considered by
// ADL. In other words, ADL will consider all associated namespaces: this
// includes not only N, but also any inline friend functions declared by ns::T.
//
// The purpose of this test is to ensure that simply constructing or destroying
// a container does not invoke ADL which would be problematic for unknown types.
// By using the type 'Holder<ns::T> *' as a container value, we can avoid the
// obvious problems with unknown types: a pointer is trivial, and its size is
// known. However, any ADL which includes ns::T as an associated namesapce will
// fail.
//
// For example:
//
//   namespace ns {
//     class T {
//       // 13.5.7 [over.inc]:
//       friend std::list<T *>::const_iterator
//       operator++(std::list<T *>::const_iterator it) {
//         return /* ... */;
//       }
//     };
//   }
//
// The C++ standard stipulates that some functions, such as std::advance, use
// overloaded operators (in C++14: 24.4.4 [iterator.operations]). The
// implication is that calls to such a function are dependent on knowing the
// overload set of operators in all associated namespaces; under ADL, this
// includes the private friend function in the example above.
//
// However, for some operations, such as construction and destruction, no such
// ADL is required. This can be important, for example, when using the Pimpl
// pattern:
//
//   // Defined in a header file:
//   class InterfaceList {
//     // Defined in a .cpp file:
//     class Impl;
//     vector<Impl*> impls;
//   public:
//     ~InterfaceList();
//   };
//
template <class T>
class Holder { T value; };

namespace ns { class Fwd; }

// TestSequencePtr and TestMappingPtr ensure that a given container type can be
// default-constructed and destroyed with an incomplete value pointer type.
template <template <class...> class Container>
void TestSequencePtr() {
  using X = Container<Holder<ns::Fwd>*>;
  {
    X u;
    assert(u.empty());
  }
  {
    auto u = X();
    assert(u.empty());
  }
}

template <template <class...> class Container>
void TestMappingPtr() {
  {
    using X = Container<int, Holder<ns::Fwd>*>;
    X u1;
    assert(u1.empty());
    auto u2 = X();
    assert(u2.empty());
  }
  {
    using X = Container<Holder<ns::Fwd>*, int>;
    X u1;
    assert(u1.empty());
    auto u2 = X();
    assert(u2.empty());
  }
}

int main()
{
  // Sequence containers:
  {
    std::array<Holder<ns::Fwd>*, 1> a;
    (void)a;
  }
  TestSequencePtr<std::deque>();
  TestSequencePtr<std::forward_list>();
  TestSequencePtr<std::list>();
  TestSequencePtr<std::vector>();

  // Associative containers, non-mapping:
  TestSequencePtr<std::set>();
  TestSequencePtr<std::unordered_set>();

  // Associative containers, mapping:
  TestMappingPtr<std::map>();
  TestMappingPtr<std::unordered_map>();

  // Container adapters:
  TestSequencePtr<std::queue>();
  TestSequencePtr<std::stack>();
}
