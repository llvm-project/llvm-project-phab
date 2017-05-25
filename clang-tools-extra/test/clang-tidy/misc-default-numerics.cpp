// RUN: %check_clang_tidy %s misc-default-numerics %t

namespace std {

template <typename T>
struct numeric_limit {
  static T min() { return T(); }
  static T max() { return T(); }
};

template <>
struct numeric_limit<int> {
  static int min() { return -1; }
  static int max() { return 1; }
};

} // namespace std

class MyType {};
template <typename T>
class MyTemplate {};

void test() {
  auto x = std::numeric_limit<int>::min();

  auto y = std::numeric_limit<MyType>::min();
  // CHECK-MESSAGES: [[@LINE-1]]:12: warning: 'std::numeric_limits::min' called with type 'MyType'; no such specialization exists, so the default value for that type is returned

  auto z = std::numeric_limit<MyTemplate<int>>::max();
  // CHECK-MESSAGES: [[@LINE-1]]:12: warning: 'std::numeric_limits::max' called with type 'MyTemplate<int>';
}

template <typename T>
void fun() {
  auto x = std::numeric_limit<T>::min();
}

void testTemplate() {
  fun<int>();
  // FIXME: This should generate warning with backtrace.
  fun<MyType>;
}
