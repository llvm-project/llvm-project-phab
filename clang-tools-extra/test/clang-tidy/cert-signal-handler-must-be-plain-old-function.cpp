// RUN: %check_clang_tidy %s cert-msc54-cpp %t -- -- -std=c++11

namespace std {
extern "C" using signalhandler = void(int);
int signal(int sig, signalhandler *func);
}

#define SIG_ERR -1
#define SIGTERM 15

static void sig_handler(int sig) {}
// CHECK-MESSAGES: :[[@LINE-1]]:13: warning: use 'external C' prefix for signal handlers [cert-msc54-cpp]
// CHECK-MESSAGES: :[[@LINE+3]]:39: note: given as a signal parameter here

void install_signal_handler() {
  if (SIG_ERR == std::signal(SIGTERM, sig_handler))
    return;
}

extern "C" void cpp_signal_handler(int sig) {
  // CHECK-MESSAGES: :[[@LINE-1]]:17: warning: do not use C++ representations in signal handlers [cert-msc54-cpp]
  // CHECK-MESSAGES: :[[@LINE+1]]:3: note: C++ representation used here
  throw "error message";
}

void install_cpp_signal_handler() {
  if (SIG_ERR == std::signal(SIGTERM, cpp_signal_handler))
    return;
}

void indirect_recursion();

void cpp_like(){
  try {
    cpp_signal_handler(SIG_ERR);
  } catch(...) {
    // handle error
  }
}

void no_body();

void recursive_function() {
  indirect_recursion();
  cpp_like();
  no_body();
  recursive_function();
}

void indirect_recursion() {
  recursive_function();
}

extern "C" void signal_handler_with_cpp_function_call(int sig) {
  // CHECK-MESSAGES: :[[@LINE-1]]:17: warning: do not call functions with C++ representations in signal handlers [cert-msc54-cpp]
  // CHECK-MESSAGES: :[[@LINE-11]]:3: note: function called here
  // CHECK-MESSAGES: :[[@LINE-23]]:3: note: C++ representation used here
  recursive_function();
}

void install_signal_handler_with_cpp_function_call() {
  if (SIG_ERR == std::signal(SIGTERM, signal_handler_with_cpp_function_call))
    return;
}

class TestClass {
public:
  static void static_function() {}
};

extern "C" void signal_handler_with_cpp_static_method_call(int sig) {
  // CHECK-MESSAGES: :[[@LINE-1]]:17: warning: do not call functions with C++ representations in signal handlers [cert-msc54-cpp]
  // CHECK-MESSAGES: :[[@LINE+1]]:3: note: function called here
  TestClass::static_function();
}

void install_signal_handler_with_cpp_static_method_call() {
  if (SIG_ERR == std::signal(SIGTERM, signal_handler_with_cpp_static_method_call))
    return;
}


// Something that does not trigger the check:

extern "C" void c_sig_handler(int sig) {
#define cbrt(X) _Generic((X), long double \
                         : cbrtl, default \
                         : cbrt)(X)
  auto char c = '1';
  extern _Thread_local double _Complex d;
  static const unsigned long int i = sizeof(float);
  register short s;
  void f();
  volatile struct c_struct {
    enum e {};
    union u;
  };
  typedef bool boolean;
label:
  switch (c) {
  case ('1'):
    break;
  default:
    d = 1.2;
  }
  goto label;
  for (; i < 42;) {
    if (d == 1.2)
      continue;
    else
      return;
  }
  do {
    _Atomic int v = _Alignof(char);
    _Static_assert(42 == 42, "True");
  } while (c == 42);
}

void install_c_sig_handler() {
  if (SIG_ERR == std::signal(SIGTERM, c_sig_handler)) {
    // Handle error
  }
}

extern "C" void signal_handler_with_function_pointer(int sig) {
  void (*funp) (void);
  funp = &cpp_like;
  funp();
}

void install_signal_handler_with_function_pointer() {
  if (SIG_ERR == std::signal(SIGTERM, signal_handler_with_function_pointer))
    return;
}
