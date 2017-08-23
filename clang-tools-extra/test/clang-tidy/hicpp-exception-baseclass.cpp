// RUN: %check_clang_tidy %s hicpp-exception-baseclass %t -- -- -fcxx-exceptions

namespace std {
class exception {};
} // namespace std

class derived_exception : public std::exception {};
class deep_hierarchy : public derived_exception {};
class non_derived_exception {};

void problematic() {
  try {
    throw int(42); // Built in is not allowed
    // CHECK-MESSAGES: [[@LINE-1]]:5: warning: throwing an exception whose type is not derived from 'std::exception'
  } catch (int e) {
  }
  throw int(42); // Bad
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: throwing an exception whose type is not derived from 'std::exception'

  try {
    throw non_derived_exception(); // Some class is not allowed
    // CHECK-MESSAGES: [[@LINE-1]]:5: warning: throwing an exception whose type is not derived from 'std::exception'
    // CHECK-MESSAGES: 9:1: note: type defined here
  } catch (non_derived_exception &e) {
  }
  throw non_derived_exception(); // Bad
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: throwing an exception whose type is not derived from 'std::exception'
  // CHECK-MESSAGES: 9:1: note: type defined here
}

void allowed_throws() {
  try {
    throw std::exception(); // Ok
  } catch (std::exception &e) { // Ok
  }
  throw std::exception();

  try {
    throw derived_exception(); // Ok
  } catch (derived_exception &e) { // Ok
  }
  throw derived_exception(); // Ok

  try {
    throw deep_hierarchy(); // Ok, multiple levels of inheritance
  } catch (deep_hierarchy &e) { // Ok
  }
  throw deep_hierarchy(); // Ok
}

// Templated function that throws exception based on template type
template <typename T>
void ThrowException() { throw T(); }
// CHECK-MESSAGES: [[@LINE-1]]:25: warning: throwing an exception whose type is not derived from 'std::exception'
// CHECK-MESSAGES: [[@LINE-3]]:11: note: type defined here
#define THROW_EXCEPTION(CLASS) ThrowException<CLASS>()
#define THROW_BAD_EXCEPTION throw int(42);
#define THROW_GOOD_EXCEPTION throw std::exception();
#define THROW_DERIVED_EXCEPTION throw deep_hierarchy();

template <typename T>
class generic_exception : std::exception {};

template <typename T>
class bad_generic_exception {};

template <typename T>
class exotic_exception : public T {};

void generic_exceptions() {
  THROW_EXCEPTION(int); // Bad
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: throwing an exception whose type is not derived from 'std::exception'
  THROW_EXCEPTION(non_derived_exception); // Bad
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: throwing an exception whose type is not derived from 'std::exception'
  THROW_EXCEPTION(std::exception); // Ok
  THROW_EXCEPTION(derived_exception); // Ok
  THROW_EXCEPTION(deep_hierarchy); // Ok

  THROW_BAD_EXCEPTION;
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: throwing an exception whose type is not derived from 'std::exception'
  // CHECK-MESSAGES: [[@LINE-16]]:29: note: expanded from macro 'THROW_BAD_EXCEPTION'
  THROW_GOOD_EXCEPTION;
  THROW_DERIVED_EXCEPTION;

  throw generic_exception<int>(); // Ok,
  THROW_EXCEPTION(generic_exception<float>); // Ok

  throw bad_generic_exception<int>(); // Bad, not derived
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: throwing an exception whose type is not derived from 'std::exception'
  throw bad_generic_exception<std::exception>(); // Bad as well, since still not derived
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: throwing an exception whose type is not derived from 'std::exception'
  THROW_EXCEPTION(bad_generic_exception<int>); // Bad
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: throwing an exception whose type is not derived from 'std::exception'
  THROW_EXCEPTION(bad_generic_exception<std::exception>); // Bad
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: throwing an exception whose type is not derived from 'std::exception'

  throw exotic_exception<non_derived_exception>(); // Bad
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: throwing an exception whose type is not derived from 'std::exception'
  THROW_EXCEPTION(exotic_exception<non_derived_exception>); // Bad
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: throwing an exception whose type is not derived from 'std::exception'

  throw exotic_exception<derived_exception>(); // Ok
  THROW_EXCEPTION(exotic_exception<derived_exception>); // Ok
}

// Test for typedefed exception types
typedef int TypedefedBad;
typedef derived_exception TypedefedGood;
using UsingBad = int;
using UsingGood = deep_hierarchy;

void typedefed() {
  throw TypedefedBad(); // Bad
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: throwing an exception whose type is not derived from 'std::exception'
  throw TypedefedGood(); // Ok

  throw UsingBad(); // Bad
  // CHECK-MESSAGES: [[@LINE-1]]:3: warning: throwing an exception whose type is not derived from 'std::exception'
  throw UsingGood(); // Ok
}
