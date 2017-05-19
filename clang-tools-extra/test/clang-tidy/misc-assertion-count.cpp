// RUN: %check_clang_tidy %s misc-assertion-count %t \
// RUN:   -- -config="{CheckOptions: [ \
// RUN:         {key: misc-assertion-count.LineThreshold, value: 0}, \
// RUN:         {key: misc-assertion-count.AssertsThreshold, value: 2}, \
// RUN:         {key: misc-assertion-count.LinesStep, value: 0}, \
// RUN:         {key: misc-assertion-count.AssertsStep, value: 0}, \
// RUN:         {key: misc-assertion-count.AssertNames, value: 'assert,assert2,my_assert,convoluted_assert,msvc_assert,convoluted_assert_wrapper,convoluted_assert_wrapper2,dummydefine,external_assert,external_system_assert,external_ignored_assert,external_ignored_system_assert,ThrowException,ThrowExceptionHelper,ThrowIE,justSomeFunction,justSomeOtherFunction,tracked_static_macro,tracked_lambda,tracked_lambda_macro'} \
// RUN:       ]}" \
// RUN:      -line-filter='[{"name":"%t.cpp","lines":[[97,9999]]}]' \
// RUN:   -- -std=c++11 -fexceptions \
// RUN:      -I %S/Inputs/misc-assertion-count -isystem %S/Inputs/misc-assertion-count

#include "normal-include.h"
#include <normal-system-include.h>

// clang-format off

//===--- assert definition block ------------------------------------------===//
int abort() { return 0; }

#ifdef NDEBUG
#define assert(x) 1
#else
#define assert(x)                                                              \
  if (!(x))                                                                    \
  (void)abort()
#endif

void print(...);
#define assert2(e) (__builtin_expect(!(e), 0) ?                                \
                       print (#e, __FILE__, __LINE__) : (void)0)

#ifdef NDEBUG
#define my_assert(x) 1
#else
#define my_assert(x)                                                           \
  ((void)((x) ? 1 : abort()))
#endif

#ifdef NDEBUG
#define not_my_assert(x) 1
#else
#define not_my_assert(x)                                                       \
  if (!(x))                                                                    \
  (void)abort()
#endif

#define real_assert(x) ((void)((x) ? 1 : abort()))
#define wrap1(x) real_assert(x)
#define wrap2(x) wrap1(x)
#define convoluted_assert(x) wrap2(x)

#define msvc_assert(expression) (void)(                                        \
            (!!(expression)) ||                                                \
            (abort(), 0)                                                       \
        )
#define convoluted_assert_wrapper2(x) convoluted_assert_wrapper(x)

#define convoluted_assert_wrapper(x) convoluted_assert(x)

#define dummydefine(...)

#define static_macro(x) static_assert(sizeof(x) > 0, "foo")

#define tracked_static_macro(x) static_macro(x)

// FIXME: how to detect lambda calls?
auto tracked_lambda = [](int X) {
  assert(X);
};

#define tracked_lambda_macro(x) tracked_lambda(x)

//===----------------------------------------------------------------------===//

//===--- function definition block ----------------------------------------===//
template <typename T>
void ThrowException(int file, int line) {
  print (file, line);
  throw T();
}

#define ThrowExceptionHelper(CLASS) ThrowException<CLASS>('f', __LINE__)

#define ThrowIE()           \
  do {                         \
    ThrowExceptionHelper(int); \
    __builtin_unreachable();   \
  } while (false)

void justSomeFunction(int a);
void justSomeOtherFunction(int b, int c){};


//===----------------------------------------------------------------------===//

int X = 0;

void foo0() {}
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: function 'foo0' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-2]]:6: note: 0 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-3]]:6: note: 0 assertions (recommended 2)

void foo1() {
// one line
}
// CHECK-MESSAGES: :[[@LINE-3]]:6: warning: function 'foo1' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-4]]:6: note: 2 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-5]]:6: note: 0 assertions (recommended 2)

void foo2() {assert(X);}
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: function 'foo2' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-2]]:6: note: 0 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-3]]:6: note: 1 assertions (recommended 2)

void foo3() {
  assert(X);
}
// CHECK-MESSAGES: :[[@LINE-3]]:6: warning: function 'foo3' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-4]]:6: note: 2 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-5]]:6: note: 1 assertions (recommended 2)

void foo4() { // OK
  assert(X);
  assert(X);
}

void foo5() {
  assert2(X);
}
// CHECK-MESSAGES: :[[@LINE-3]]:6: warning: function 'foo5' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-4]]:6: note: 2 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-5]]:6: note: 1 assertions (recommended 2)

void foo6() {
  my_assert(X);
}
// CHECK-MESSAGES: :[[@LINE-3]]:6: warning: function 'foo6' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-4]]:6: note: 2 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-5]]:6: note: 1 assertions (recommended 2)

void foo7() {
  wrap2(X);
  convoluted_assert(X);
}
// CHECK-MESSAGES: :[[@LINE-4]]:6: warning: function 'foo7' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-5]]:6: note: 3 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-6]]:6: note: 1 assertions (recommended 2)

void foo8() {
  msvc_assert(X);
}
// CHECK-MESSAGES: :[[@LINE-3]]:6: warning: function 'foo8' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-4]]:6: note: 2 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-5]]:6: note: 1 assertions (recommended 2)

void foo9() {
  convoluted_assert_wrapper(X);
}
// CHECK-MESSAGES: :[[@LINE-3]]:6: warning: function 'foo9' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-4]]:6: note: 2 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-5]]:6: note: 1 assertions (recommended 2)

void foo10() {
  convoluted_assert_wrapper2(X);
}
// CHECK-MESSAGES: :[[@LINE-3]]:6: warning: function 'foo10' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-4]]:6: note: 2 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-5]]:6: note: 1 assertions (recommended 2)

void foo11() {
  dummydefine(X);
}
// CHECK-MESSAGES: :[[@LINE-3]]:6: warning: function 'foo11' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-4]]:6: note: 2 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-5]]:6: note: 1 assertions (recommended 2)

void foo12() {
  external_assert(X);
}
// CHECK-MESSAGES: :[[@LINE-3]]:6: warning: function 'foo12' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-4]]:6: note: 2 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-5]]:6: note: 1 assertions (recommended 2)

void foo13() {
  external_system_assert(X);
}
// CHECK-MESSAGES: :[[@LINE-3]]:6: warning: function 'foo13' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-4]]:6: note: 2 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-5]]:6: note: 1 assertions (recommended 2)

inline void foo14() { // OK
  assert(X);
  msvc_assert(X);
}

inline void foo15() { // OK
  assert(X);
  msvc_assert(X);
  convoluted_assert(X);
}

inline void foo16() { // OK
  assert(X);

  if(false)
    msvc_assert(X);
}

void foo17() {
  foo14();
  foo15();
  foo16();
}
// CHECK-MESSAGES: :[[@LINE-5]]:6: warning: function 'foo17' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-6]]:6: note: 4 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-7]]:6: note: 0 assertions (recommended 2)

void foo18() { assert(X); } void foo19() { assert(X); }
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: function 'foo18' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-2]]:6: note: 0 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-3]]:6: note: 1 assertions (recommended 2)
// CHECK-MESSAGES: :[[@LINE-4]]:34: warning: function 'foo19' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-5]]:34: note: 0 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-6]]:34: note: 1 assertions (recommended 2)

void foo20() { assert(X); assert(X); } void foo21() { assert(X); }
// CHECK-MESSAGES: :[[@LINE-1]]:45: warning: function 'foo21' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-2]]:45: note: 0 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-3]]:45: note: 1 assertions (recommended 2)

void foo22() { assert(X); } void foo23() { assert(X); assert(X); }
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: function 'foo22' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-2]]:6: note: 0 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-3]]:6: note: 1 assertions (recommended 2)

void foo24() { assert(X); assert(X); } void foo25() { assert(X); assert(X); }

void foo26() {
  assert(X); } void foo27() { assert(X); }
// CHECK-MESSAGES: :[[@LINE-2]]:6: warning: function 'foo26' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-3]]:6: note: 1 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-4]]:6: note: 1 assertions (recommended 2)
// CHECK-MESSAGES: :[[@LINE-4]]:21: warning: function 'foo27' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-5]]:21: note: 0 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-6]]:21: note: 1 assertions (recommended 2)

void foo28() { assert(X);
} void foo29() { assert(X); }
// CHECK-MESSAGES: :[[@LINE-2]]:6: warning: function 'foo28' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-3]]:6: note: 1 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-4]]:6: note: 1 assertions (recommended 2)
// CHECK-MESSAGES: :[[@LINE-4]]:8: warning: function 'foo29' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-5]]:8: note: 0 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-6]]:8: note: 1 assertions (recommended 2)

void foo30() { assert(X); }
void foo31() { assert(X); }
// CHECK-MESSAGES: :[[@LINE-2]]:6: warning: function 'foo30' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-3]]:6: note: 0 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-4]]:6: note: 1 assertions (recommended 2)
// CHECK-MESSAGES: :[[@LINE-4]]:6: warning: function 'foo31' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-5]]:6: note: 0 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-6]]:6: note: 1 assertions (recommended 2)

void foo32() { assert(X); } void foo33() {
  assert(X); }
// CHECK-MESSAGES: :[[@LINE-2]]:6: warning: function 'foo32' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-3]]:6: note: 0 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-4]]:6: note: 1 assertions (recommended 2)
// CHECK-MESSAGES: :[[@LINE-5]]:34: warning: function 'foo33' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-6]]:34: note: 1 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-7]]:34: note: 1 assertions (recommended 2)

void foo34() { assert(X); } void foo35() { assert(X);
}
// CHECK-MESSAGES: :[[@LINE-2]]:6: warning: function 'foo34' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-3]]:6: note: 0 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-4]]:6: note: 1 assertions (recommended 2)
// CHECK-MESSAGES: :[[@LINE-5]]:34: warning: function 'foo35' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-6]]:34: note: 1 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-7]]:34: note: 1 assertions (recommended 2)

void foo36() {
  assert(X); } void foo37() {
  assert(X); }
// CHECK-MESSAGES: :[[@LINE-3]]:6: warning: function 'foo36' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-4]]:6: note: 1 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-5]]:6: note: 1 assertions (recommended 2)
// CHECK-MESSAGES: :[[@LINE-5]]:21: warning: function 'foo37' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-6]]:21: note: 1 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-7]]:21: note: 1 assertions (recommended 2)

// FIXME
void foo38() {
  tracked_lambda(X);
}
// CHECK-MESSAGES: :[[@LINE-3]]:6: warning: function 'foo38' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-4]]:6: note: 2 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-5]]:6: note: 0 assertions (recommended 2)

void foo39() {
  tracked_lambda_macro(X);
}
// CHECK-MESSAGES: :[[@LINE-3]]:6: warning: function 'foo39' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-4]]:6: note: 2 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-5]]:6: note: 1 assertions (recommended 2)

//===----------------------------------------------------------------------===//

void bar0() {
  abort();
}
// CHECK-MESSAGES: :[[@LINE-3]]:6: warning: function 'bar0' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-4]]:6: note: 2 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-5]]:6: note: 0 assertions (recommended 2)

void bar1() {
  ThrowException<int>(0, 0);
}
// CHECK-MESSAGES: :[[@LINE-3]]:6: warning: function 'bar1' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-4]]:6: note: 2 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-5]]:6: note: 1 assertions (recommended 2)

void bar2() {
  ThrowExceptionHelper(int);
}
// CHECK-MESSAGES: :[[@LINE-3]]:6: warning: function 'bar2' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-4]]:6: note: 2 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-5]]:6: note: 1 assertions (recommended 2)

void bar3() {
  ThrowIE();
}
// CHECK-MESSAGES: :[[@LINE-3]]:6: warning: function 'bar3' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-4]]:6: note: 2 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-5]]:6: note: 1 assertions (recommended 2)

void bar4() {
  justSomeFunction(X);
}
// CHECK-MESSAGES: :[[@LINE-3]]:6: warning: function 'bar4' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-4]]:6: note: 2 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-5]]:6: note: 1 assertions (recommended 2)

void bar5() {
  justSomeOtherFunction(X, X - X);
}
// CHECK-MESSAGES: :[[@LINE-3]]:6: warning: function 'bar5' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-4]]:6: note: 2 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-5]]:6: note: 1 assertions (recommended 2)

inline void bar6() { // OK
  justSomeFunction(X);
  justSomeOtherFunction(X, X * X);
}

inline void bar7() { // OK
  assert(X);
  justSomeOtherFunction(X, X + 4);
}

struct QQQ {
inline void bar8() { // OK
  {
    int Y = 1;
    do {
      for(;;);
      while(0) {
        assert(X);
      }
      if(false) {
        justSomeOtherFunction(X, Y);
      }
    } while(false);
  }
}
};

void bar9() {
  bar6();
  bar7();
}
// CHECK-MESSAGES: :[[@LINE-4]]:6: warning: function 'bar9' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-5]]:6: note: 3 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-6]]:6: note: 0 assertions (recommended 2)

void bar10() { justSomeFunction(X); } void bar11() { justSomeFunction(X); }
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: function 'bar10' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-2]]:6: note: 0 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-3]]:6: note: 1 assertions (recommended 2)
// CHECK-MESSAGES: :[[@LINE-4]]:44: warning: function 'bar11' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-5]]:44: note: 0 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-6]]:44: note: 1 assertions (recommended 2)

void bar12() { assert(X); justSomeFunction(X); } void bar13() { justSomeFunction(X); }
// CHECK-MESSAGES: :[[@LINE-1]]:55: warning: function 'bar13' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-2]]:55: note: 0 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-3]]:55: note: 1 assertions (recommended 2)

auto bar13andhalf = []() {
};
// CHECK-MESSAGES: :[[@LINE-2]]:21: warning: function 'operator()' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-3]]:21: note: 1 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-4]]:21: note: 0 assertions (recommended 2)

void bar14() {
  auto bar15 = []() {
  };
}
// CHECK-MESSAGES: :[[@LINE-4]]:6: warning: function 'bar14' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-5]]:6: note: 3 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-6]]:6: note: 0 assertions (recommended 2)

void bar16() {
  auto bar17 = []() {
    auto bar18 = []() {
    };
  };
}
// CHECK-MESSAGES: :[[@LINE-6]]:6: warning: function 'bar16' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-7]]:6: note: 5 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-8]]:6: note: 0 assertions (recommended 2)

void bar19() {
  auto bar20 = []() {
  };
  auto bar21 = []() {
  };
}
// CHECK-MESSAGES: :[[@LINE-6]]:6: warning: function 'bar19' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-7]]:6: note: 5 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-8]]:6: note: 0 assertions (recommended 2)

void bar22() {
  assert(X);
  auto bar23 = []() {
  };
}
// CHECK-MESSAGES: :[[@LINE-5]]:6: warning: function 'bar22' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-6]]:6: note: 4 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-7]]:6: note: 1 assertions (recommended 2)

void bar24() {
  auto bar25 = []() {
    assert(X);
  };
}
// CHECK-MESSAGES: :[[@LINE-5]]:6: warning: function 'bar24' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-6]]:6: note: 4 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-7]]:6: note: 0 assertions (recommended 2)

void bar26() {
  assert(X);
  auto bar27 = []() {
    assert(X);
  };
}
// CHECK-MESSAGES: :[[@LINE-6]]:6: warning: function 'bar26' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-7]]:6: note: 5 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-8]]:6: note: 1 assertions (recommended 2)

void bar28() {
  auto bar29 = []() {
    assert(X);
    auto bar30 = []() {
    };
  };
}
// CHECK-MESSAGES: :[[@LINE-7]]:6: warning: function 'bar28' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-8]]:6: note: 6 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-9]]:6: note: 0 assertions (recommended 2)

void bar31() {
  auto bar32 = []() {
    auto bar33 = []() {
      assert(X);
    };
  };
}
// CHECK-MESSAGES: :[[@LINE-7]]:6: warning: function 'bar31' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-8]]:6: note: 6 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-9]]:6: note: 0 assertions (recommended 2)

void bar34() {
  auto bar35 = []() {
    assert(X);
    auto bar36 = []() {
      assert(X);
    };
  };
}
// CHECK-MESSAGES: :[[@LINE-8]]:6: warning: function 'bar34' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-9]]:6: note: 7 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-10]]:6: note: 0 assertions (recommended 2)

void bar37() {
  assert(X);
  auto bar38 = []() {
    assert(X);
    auto bar39 = []() {
      assert(X);
    };
  };
}
// CHECK-MESSAGES: :[[@LINE-9]]:6: warning: function 'bar37' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-10]]:6: note: 8 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-11]]:6: note: 1 assertions (recommended 2)

void bar40() {
  justSomeFunction(X);
  auto bar41 = []() {
    assert(X);
  };
}
// CHECK-MESSAGES: :[[@LINE-6]]:6: warning: function 'bar40' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-7]]:6: note: 5 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-8]]:6: note: 1 assertions (recommended 2)

void bar42() {
  assert(X);
  auto bar43 = []() {
    justSomeFunction(X);
  };
}
// CHECK-MESSAGES: :[[@LINE-6]]:6: warning: function 'bar42' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-7]]:6: note: 5 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-8]]:6: note: 1 assertions (recommended 2)

//===----------------------------------------------------------------------===//

// do not warn on implicit declarations.
class C {
  void memfunc() {}
};
C S;
// CHECK-MESSAGES: :[[@LINE-3]]:8: warning: function 'memfunc' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-4]]:8: note: 0 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-5]]:8: note: 0 assertions (recommended 2)

void bas0() {
  class C {
    void bas1() {}
  };
}
// CHECK-MESSAGES: :[[@LINE-5]]:6: warning: function 'bas0' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-6]]:6: note: 4 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-7]]:6: note: 0 assertions (recommended 2)
// CHECK-MESSAGES: :[[@LINE-6]]:10: warning: function 'bas1' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-7]]:10: note: 0 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-8]]:10: note: 0 assertions (recommended 2)

void bas2() {
  assert(X);
  class C {
    void bas3() {}
  };
}
// CHECK-MESSAGES: :[[@LINE-6]]:6: warning: function 'bas2' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-7]]:6: note: 5 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-8]]:6: note: 1 assertions (recommended 2)
// CHECK-MESSAGES: :[[@LINE-6]]:10: warning: function 'bas3' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-7]]:10: note: 0 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-8]]:10: note: 0 assertions (recommended 2)

void bas4() {
  class C {
    void bas5() {
      assert(X);
    }
  };
}
// CHECK-MESSAGES: :[[@LINE-7]]:6: warning: function 'bas4' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-8]]:6: note: 6 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-9]]:6: note: 0 assertions (recommended 2)
// CHECK-MESSAGES: :[[@LINE-8]]:10: warning: function 'bas5' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-9]]:10: note: 2 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-10]]:10: note: 1 assertions (recommended 2)

void bas6() {
  assert(X);
  class C {
    void bas7() {
      assert(X);
    }
  };
}
// CHECK-MESSAGES: :[[@LINE-8]]:6: warning: function 'bas6' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-9]]:6: note: 7 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-10]]:6: note: 1 assertions (recommended 2)
// CHECK-MESSAGES: :[[@LINE-8]]:10: warning: function 'bas7' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-9]]:10: note: 2 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-10]]:10: note: 1 assertions (recommended 2)

//===----------------------------------------------------------------------===//

void baz0() {
  static_assert(sizeof(X) > 0, "foo");
}
// CHECK-MESSAGES: :[[@LINE-3]]:6: warning: function 'baz0' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-4]]:6: note: 2 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-5]]:6: note: 1 assertions (recommended 2)

void baz1() {
  static_macro(X);
}
// CHECK-MESSAGES: :[[@LINE-3]]:6: warning: function 'baz1' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-4]]:6: note: 2 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-5]]:6: note: 1 assertions (recommended 2)

void baz2() {
  tracked_static_macro(X);
}
// CHECK-MESSAGES: :[[@LINE-3]]:6: warning: function 'baz2' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-4]]:6: note: 2 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-5]]:6: note: 1 assertions (recommended 2)

void baz3() { // OK
  static_assert(sizeof(X) > 0, "foo");
  assert(X);
}

void baz4() { // OK
  static_macro(X);
  justSomeFunction(X);
}

void baz5() {
  static_macro(X);
  auto baz6 = []() {
  };
}
// CHECK-MESSAGES: :[[@LINE-5]]:6: warning: function 'baz5' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-6]]:6: note: 4 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-7]]:6: note: 1 assertions (recommended 2)

void baz7() {
  auto baz8 = []() {
    static_macro(X);
  };
}
// CHECK-MESSAGES: :[[@LINE-5]]:6: warning: function 'baz7' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-6]]:6: note: 4 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-7]]:6: note: 0 assertions (recommended 2)

void baz9() {
  static_macro(X);
  auto baz10 = []() {
    static_macro(X);
  };
}
// CHECK-MESSAGES: :[[@LINE-6]]:6: warning: function 'baz9' falls behind recommended lines-per-assertion thresholds [misc-assertion-count]
// CHECK-MESSAGES: :[[@LINE-7]]:6: note: 5 lines including whitespace and comments (threshold 0)
// CHECK-MESSAGES: :[[@LINE-8]]:6: note: 1 assertions (recommended 2)
