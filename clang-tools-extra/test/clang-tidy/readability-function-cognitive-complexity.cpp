// RUN: %check_clang_tidy %s readability-function-cognitive-complexity %t -- -config='{CheckOptions: [{key: readability-function-cognitive-complexity.Threshold, value: 0}]}' -- -std=c++11

// any function should be checked.

extern int ext_func(int x = 0);

int some_func(int x = 0);

static int some_other_func(int x = 0) {}

template<typename T> void some_templ_func(T x = 0) {}

class SomeClass {
public:
  int *begin(int x = 0);
  int *end(int x = 0);
  static int func(int x = 0);
  template<typename T> void some_templ_func(T x = 0) {}
};

// nothing ever decreases cognitive complexity, so we can check all the things
// in one go. none of the following should increase cognitive complexity:
void unittest_false() {
  {};
  ext_func();
  some_func();
  some_other_func();
  some_templ_func<int>();
  some_templ_func<bool>();
  SomeClass::func();
  SomeClass C;
  C.some_templ_func<int>();
  C.some_templ_func<bool>();
  C.func();
  C.end();
  int i = some_func();
  i = i;
  i++;
  --i;
  i < 0;
  int j = 0 ?: 1;
  auto k = new int;
  delete k;
  throw i;
end:
  return;
}

//----------------------------------------------------------------------------//
//------------------------------ B1. Increments ------------------------------//
//----------------------------------------------------------------------------//
// Check that every thing listed in B1 of the specification does indeed       //
// recieve the base increment, and that not-body does not increase nesting    //
//----------------------------------------------------------------------------//

// break does not increase cognitive complexity.
// only  break LABEL  does, but it is unavaliable in C or C++

// continue does not increase cognitive complexity.
// only  continue LABEL  does, but it is unavaliable in C or C++

void unittest_b1_00() {
// CHECK-NOTES: :[[@LINE-1]]:6: warning: function 'unittest_b1_00' has cognitive complexity of 5 (threshold 0) [readability-function-cognitive-complexity]
  if (1 ? 1 : 0) {
// CHECK-NOTES: :[[@LINE-1]]:3: note: +1, including nesting penalty of 0, nesting level increased to 1{{$}}
// CHECK-NOTES: :[[@LINE-2]]:7: note: +1, including nesting penalty of 0, nesting level increased to 1{{$}}
  } else if (1 ? 1 : 0) {
// CHECK-NOTES: :[[@LINE-1]]:10: note: +1, nesting level increased to 1{{$}}
// CHECK-NOTES: :[[@LINE-2]]:14: note: +1, including nesting penalty of 0, nesting level increased to 1{{$}}
  } else {
// CHECK-NOTES: :[[@LINE-1]]:5: note: +1, nesting level increased to 1{{$}}
  }
}

void unittest_b1_01() {
// CHECK-NOTES: :[[@LINE-1]]:6: warning: function 'unittest_b1_01' has cognitive complexity of 2 (threshold 0) [readability-function-cognitive-complexity]
  int i = (1 ? 1 : 0) ? 1 : 0;
// CHECK-NOTES: :[[@LINE-1]]:11: note: +1, including nesting penalty of 0, nesting level increased to 1{{$}}
// CHECK-NOTES: :[[@LINE-2]]:12: note: +1, including nesting penalty of 0, nesting level increased to 1{{$}}
// FIXME: would be nice to point at the '?' symbol. does not seem to be possible
}

void unittest_b1_02(int x) {
// CHECK-NOTES: :[[@LINE-1]]:6: warning: function 'unittest_b1_02' has cognitive complexity of 5 (threshold 0) [readability-function-cognitive-complexity]
  switch (1 ? 1 : 0) {
// CHECK-NOTES: :[[@LINE-1]]:3: note: +1, including nesting penalty of 0, nesting level increased to 1{{$}}
// CHECK-NOTES: :[[@LINE-2]]:11: note: +1, including nesting penalty of 0, nesting level increased to 1{{$}}
  case -1:
    return;
  case 1 ? 1 : 0:
// CHECK-NOTES: :[[@LINE-1]]:8: note: +1, including nesting penalty of 0, nesting level increased to 1{{$}}
    return;
  case (1 ? 2 : 0) ... (1 ? 3 : 0):
// CHECK-NOTES: :[[@LINE-1]]:9: note: +1, including nesting penalty of 0, nesting level increased to 1{{$}}
// CHECK-NOTES: :[[@LINE-2]]:25: note: +1, including nesting penalty of 0, nesting level increased to 1{{$}}
    return;
  default:
    break;
  }
}

void unittest_b1_03(int x) {
// CHECK-NOTES: :[[@LINE-1]]:6: warning: function 'unittest_b1_03' has cognitive complexity of 4 (threshold 0) [readability-function-cognitive-complexity]
  for (x = 1 ? 1 : 0; x < (1 ? 1 : 0); x += 1 ? 1 : 0) {
// CHECK-NOTES: :[[@LINE-1]]:3: note: +1, including nesting penalty of 0, nesting level increased to 1{{$}}
// CHECK-NOTES: :[[@LINE-2]]:12: note: +1, including nesting penalty of 0, nesting level increased to 1{{$}}
// CHECK-NOTES: :[[@LINE-3]]:28: note: +1, including nesting penalty of 0, nesting level increased to 1{{$}}
// CHECK-NOTES: :[[@LINE-4]]:45: note: +1, including nesting penalty of 0, nesting level increased to 1{{$}}
    break;
    continue;
  }
}

void unittest_b1_04() {
// CHECK-NOTES: :[[@LINE-1]]:6: warning: function 'unittest_b1_04' has cognitive complexity of 2 (threshold 0) [readability-function-cognitive-complexity]
  SomeClass C;
  for (int i : (1 ? C : C)) {
// CHECK-NOTES: :[[@LINE-1]]:3: note: +1, including nesting penalty of 0, nesting level increased to 1{{$}}
// CHECK-NOTES: :[[@LINE-2]]:17: note: +1, including nesting penalty of 0, nesting level increased to 1{{$}}
    break;
    continue;
  }
}

void unittest_b1_05() {
// CHECK-NOTES: :[[@LINE-1]]:6: warning: function 'unittest_b1_05' has cognitive complexity of 2 (threshold 0) [readability-function-cognitive-complexity]
  while (1 ? 1 : 0) {
// CHECK-NOTES: :[[@LINE-1]]:3: note: +1, including nesting penalty of 0, nesting level increased to 1{{$}}
// CHECK-NOTES: :[[@LINE-2]]:10: note: +1, including nesting penalty of 0, nesting level increased to 1{{$}}
    break;
    continue;
  }
}

void unittest_b1_06() {
// CHECK-NOTES: :[[@LINE-1]]:6: warning: function 'unittest_b1_06' has cognitive complexity of 2 (threshold 0) [readability-function-cognitive-complexity]
  do {
// CHECK-NOTES: :[[@LINE-1]]:3: note: +1, including nesting penalty of 0, nesting level increased to 1{{$}}
    break;
    continue;
  } while (1 ? 1 : 0);
// CHECK-NOTES: :[[@LINE-1]]:12: note: +1, including nesting penalty of 0, nesting level increased to 1{{$}}
}

void unittest_b1_07() {
// CHECK-NOTES: :[[@LINE-1]]:6: warning: function 'unittest_b1_07' has cognitive complexity of 1 (threshold 0) [readability-function-cognitive-complexity]
  try {
  } catch (...) {
// CHECK-NOTES: :[[@LINE-1]]:5: note: +1, including nesting penalty of 0, nesting level increased to 1{{$}}
  }
}

void unittest_b1_08() {
// CHECK-NOTES: :[[@LINE-1]]:6: warning: function 'unittest_b1_08' has cognitive complexity of 1 (threshold 0) [readability-function-cognitive-complexity]
  goto end;
// CHECK-NOTES: :[[@LINE-1]]:3: note: +1{{$}}
end:
  return;
}

void unittest_b1_09_00() {
// CHECK-NOTES: :[[@LINE-1]]:6: warning: function 'unittest_b1_09_00' has cognitive complexity of 34 (threshold 0) [readability-function-cognitive-complexity]
  if(1 && 1) {
// CHECK-NOTES: :[[@LINE-1]]:3: note: +1, including nesting penalty of 0, nesting level increased to 1{{$}}
// CHECK-NOTES: :[[@LINE-2]]:8: note: +1{{$}}
  }
  if(1 && 1 && 1) {
// CHECK-NOTES: :[[@LINE-1]]:3: note: +1, including nesting penalty of 0, nesting level increased to 1{{$}}
// CHECK-NOTES: :[[@LINE-2]]:13: note: +1{{$}}
  }
  if((1 && 1) && 1) {
// CHECK-NOTES: :[[@LINE-1]]:3: note: +1, including nesting penalty of 0, nesting level increased to 1{{$}}
// CHECK-NOTES: :[[@LINE-2]]:15: note: +1{{$}}
  }
  if(1 && (1 && 1)) {
// CHECK-NOTES: :[[@LINE-1]]:3: note: +1, including nesting penalty of 0, nesting level increased to 1{{$}}
// CHECK-NOTES: :[[@LINE-2]]:8: note: +1{{$}}
  }

  if(1 && 1 || 1) {
// CHECK-NOTES: :[[@LINE-1]]:3: note: +1, including nesting penalty of 0, nesting level increased to 1{{$}}
// CHECK-NOTES: :[[@LINE-2]]:13: note: +1{{$}}
// CHECK-NOTES: :[[@LINE-3]]:8: note: +1{{$}}
  }
  if((1 && 1) || 1) {
// CHECK-NOTES: :[[@LINE-1]]:3: note: +1, including nesting penalty of 0, nesting level increased to 1{{$}}
// CHECK-NOTES: :[[@LINE-2]]:15: note: +1{{$}}
// CHECK-NOTES: :[[@LINE-3]]:9: note: +1{{$}}
  }
  if(1 && (1 || 1)) {
// CHECK-NOTES: :[[@LINE-1]]:3: note: +1, including nesting penalty of 0, nesting level increased to 1{{$}}
// CHECK-NOTES: :[[@LINE-2]]:8: note: +1{{$}}
// CHECK-NOTES: :[[@LINE-3]]:14: note: +1{{$}}
  }

  if(1 || 1) {
// CHECK-NOTES: :[[@LINE-1]]:3: note: +1, including nesting penalty of 0, nesting level increased to 1{{$}}
// CHECK-NOTES: :[[@LINE-2]]:8: note: +1{{$}}
  }
  if(1 || 1 || 1) {
// CHECK-NOTES: :[[@LINE-1]]:3: note: +1, including nesting penalty of 0, nesting level increased to 1{{$}}
// CHECK-NOTES: :[[@LINE-2]]:13: note: +1{{$}}
  }
  if((1 || 1) || 1) {
// CHECK-NOTES: :[[@LINE-1]]:3: note: +1, including nesting penalty of 0, nesting level increased to 1{{$}}
// CHECK-NOTES: :[[@LINE-2]]:15: note: +1{{$}}
  }
  if(1 || (1 || 1)) {
// CHECK-NOTES: :[[@LINE-1]]:3: note: +1, including nesting penalty of 0, nesting level increased to 1{{$}}
// CHECK-NOTES: :[[@LINE-2]]:8: note: +1{{$}}
  }

  if(1 || 1 && 1) {
// CHECK-NOTES: :[[@LINE-1]]:3: note: +1, including nesting penalty of 0, nesting level increased to 1{{$}}
// CHECK-NOTES: :[[@LINE-2]]:8: note: +1{{$}}
// CHECK-NOTES: :[[@LINE-3]]:13: note: +1{{$}}
  }
  if((1 || 1) && 1) {
// CHECK-NOTES: :[[@LINE-1]]:3: note: +1, including nesting penalty of 0, nesting level increased to 1{{$}}
// CHECK-NOTES: :[[@LINE-2]]:15: note: +1{{$}}
// CHECK-NOTES: :[[@LINE-3]]:9: note: +1{{$}}
  }
  if(1 || (1 && 1)) {
// CHECK-NOTES: :[[@LINE-1]]:3: note: +1, including nesting penalty of 0, nesting level increased to 1{{$}}
// CHECK-NOTES: :[[@LINE-2]]:8: note: +1{{$}}
// CHECK-NOTES: :[[@LINE-3]]:14: note: +1{{$}}
  }
}

void unittest_b1_09_01() {
// CHECK-NOTES: :[[@LINE-1]]:6: warning: function 'unittest_b1_09_01' has cognitive complexity of 12 (threshold 0) [readability-function-cognitive-complexity]
  if(1 && some_func(1 && 1)) {
// CHECK-NOTES: :[[@LINE-1]]:3: note: +1, including nesting penalty of 0, nesting level increased to 1{{$}}
// CHECK-NOTES: :[[@LINE-2]]:8: note: +1{{$}}
// CHECK-NOTES: :[[@LINE-3]]:23: note: +1{{$}}
  }
  if(1 && some_func(1 || 1)) {
// CHECK-NOTES: :[[@LINE-1]]:3: note: +1, including nesting penalty of 0, nesting level increased to 1{{$}}
// CHECK-NOTES: :[[@LINE-2]]:8: note: +1{{$}}
// CHECK-NOTES: :[[@LINE-3]]:23: note: +1{{$}}
  }
  if(1 || some_func(1 || 1)) {
// CHECK-NOTES: :[[@LINE-1]]:3: note: +1, including nesting penalty of 0, nesting level increased to 1{{$}}
// CHECK-NOTES: :[[@LINE-2]]:8: note: +1{{$}}
// CHECK-NOTES: :[[@LINE-3]]:23: note: +1{{$}}
  }
  if(1 || some_func(1 && 1)) {
// CHECK-NOTES: :[[@LINE-1]]:3: note: +1, including nesting penalty of 0, nesting level increased to 1{{$}}
// CHECK-NOTES: :[[@LINE-2]]:8: note: +1{{$}}
// CHECK-NOTES: :[[@LINE-3]]:23: note: +1{{$}}
  }
}

// FIXME: each method in a recursion cycle

//----------------------------------------------------------------------------//
//---------------------------- B2. Nesting lebel -----------------------------//
//----------------------------------------------------------------------------//
// Check that every thing listed in B2 of the specification does indeed       //
// increase the nesting level                                                 //
//----------------------------------------------------------------------------//

void unittest_b2_00() {
// CHECK-NOTES: :[[@LINE-1]]:6: warning: function 'unittest_b2_00' has cognitive complexity of 9 (threshold 0) [readability-function-cognitive-complexity]
  if (true) {
// CHECK-NOTES: :[[@LINE-1]]:3: note: +1, including nesting penalty of 0, nesting level increased to 1{{$}}
    if(true) {
// CHECK-NOTES: :[[@LINE-1]]:5: note: +2, including nesting penalty of 1, nesting level increased to 2{{$}}
    }
  } else if (true) {
// CHECK-NOTES: :[[@LINE-1]]:10: note: +1, nesting level increased to 1{{$}}
    if(true) {
// CHECK-NOTES: :[[@LINE-1]]:5: note: +2, including nesting penalty of 1, nesting level increased to 2{{$}}
    }
  } else {
// CHECK-NOTES: :[[@LINE-1]]:5: note: +1, nesting level increased to 1{{$}}
    if(true) {
// CHECK-NOTES: :[[@LINE-1]]:5: note: +2, including nesting penalty of 1, nesting level increased to 2{{$}}
    }
  }
}

void unittest_b2_01() {
// CHECK-NOTES: :[[@LINE-1]]:6: warning: function 'unittest_b2_01' has cognitive complexity of 5 (threshold 0) [readability-function-cognitive-complexity]
  int i = 1 ? (1 ? 1 : 0) : (1 ? 1 : 0);
// CHECK-NOTES: :[[@LINE-1]]:11: note: +1, including nesting penalty of 0, nesting level increased to 1{{$}}
// CHECK-NOTES: :[[@LINE-2]]:16: note: +2, including nesting penalty of 1, nesting level increased to 2{{$}}
// CHECK-NOTES: :[[@LINE-3]]:30: note: +2, including nesting penalty of 1, nesting level increased to 2{{$}}
// FIXME: would be nice to point at the '?' symbol. does not seem to be possible
}

void unittest_b2_02(int x) {
// CHECK-NOTES: :[[@LINE-1]]:6: warning: function 'unittest_b2_02' has cognitive complexity of 3 (threshold 0) [readability-function-cognitive-complexity]
  switch (x) {
// CHECK-NOTES: :[[@LINE-1]]:3: note: +1, including nesting penalty of 0, nesting level increased to 1{{$}}
  case -1:
    if(true) {
// CHECK-NOTES: :[[@LINE-1]]:5: note: +2, including nesting penalty of 1, nesting level increased to 2{{$}}
    }
    return;
  default:
    return;
  }
}

void unittest_b2_03() {
// CHECK-NOTES: :[[@LINE-1]]:6: warning: function 'unittest_b2_03' has cognitive complexity of 3 (threshold 0) [readability-function-cognitive-complexity]
  for (;;) {
// CHECK-NOTES: :[[@LINE-1]]:3: note: +1, including nesting penalty of 0, nesting level increased to 1{{$}}
    if(true) {
// CHECK-NOTES: :[[@LINE-1]]:5: note: +2, including nesting penalty of 1, nesting level increased to 2{{$}}
    }
  }
}

void unittest_b2_04() {
// CHECK-NOTES: :[[@LINE-1]]:6: warning: function 'unittest_b2_04' has cognitive complexity of 3 (threshold 0) [readability-function-cognitive-complexity]
  SomeClass C;
  for (int i : C) {
// CHECK-NOTES: :[[@LINE-1]]:3: note: +1, including nesting penalty of 0, nesting level increased to 1{{$}}
    if(true) {
// CHECK-NOTES: :[[@LINE-1]]:5: note: +2, including nesting penalty of 1, nesting level increased to 2{{$}}
    }
  }
}

void unittest_b2_05() {
// CHECK-NOTES: :[[@LINE-1]]:6: warning: function 'unittest_b2_05' has cognitive complexity of 3 (threshold 0) [readability-function-cognitive-complexity]
  while (true) {
// CHECK-NOTES: :[[@LINE-1]]:3: note: +1, including nesting penalty of 0, nesting level increased to 1{{$}}
    if(true) {
// CHECK-NOTES: :[[@LINE-1]]:5: note: +2, including nesting penalty of 1, nesting level increased to 2{{$}}
    }
  }
}

void unittest_b2_06() {
// CHECK-NOTES: :[[@LINE-1]]:6: warning: function 'unittest_b2_06' has cognitive complexity of 3 (threshold 0) [readability-function-cognitive-complexity]
  do {
// CHECK-NOTES: :[[@LINE-1]]:3: note: +1, including nesting penalty of 0, nesting level increased to 1{{$}}
    if(true) {
// CHECK-NOTES: :[[@LINE-1]]:5: note: +2, including nesting penalty of 1, nesting level increased to 2{{$}}
    }
  } while (true);
}

void unittest_b2_07() {
// CHECK-NOTES: :[[@LINE-1]]:6: warning: function 'unittest_b2_07' has cognitive complexity of 3 (threshold 0) [readability-function-cognitive-complexity]
  try {
  } catch (...) {
// CHECK-NOTES: :[[@LINE-1]]:5: note: +1, including nesting penalty of 0, nesting level increased to 1{{$}}
    if(true) {
// CHECK-NOTES: :[[@LINE-1]]:5: note: +2, including nesting penalty of 1, nesting level increased to 2{{$}}
    }
  }
}

void unittest_b2_08_00() {
// CHECK-NOTES: :[[@LINE-1]]:6: warning: function 'unittest_b2_08_00' has cognitive complexity of 8 (threshold 0) [readability-function-cognitive-complexity]
  class X {
    X() {
// CHECK-NOTES: :[[@LINE-1]]:5: note: nesting level increased to 1{{$}}
      if (true) {
// CHECK-NOTES: :[[@LINE-1]]:7: note: +2, including nesting penalty of 1, nesting level increased to 2{{$}}
      }
    }

    ~X() {
// CHECK-NOTES: :[[@LINE-1]]:5: note: nesting level increased to 1{{$}}
      if (true) {
// CHECK-NOTES: :[[@LINE-1]]:7: note: +2, including nesting penalty of 1, nesting level increased to 2{{$}}
      }
    }

    void Y() {
// CHECK-NOTES: :[[@LINE-1]]:5: note: nesting level increased to 1{{$}}
      if (true) {
// CHECK-NOTES: :[[@LINE-1]]:7: note: +2, including nesting penalty of 1, nesting level increased to 2{{$}}
      }
    }

    static void Z() {
// CHECK-NOTES: :[[@LINE-1]]:5: note: nesting level increased to 1{{$}}
      if (true) {
// CHECK-NOTES: :[[@LINE-1]]:7: note: +2, including nesting penalty of 1, nesting level increased to 2{{$}}
      }
    }

// CHECK-NOTES: :[[@LINE-28]]:5: warning: function 'X' has cognitive complexity of 1 (threshold 0) [readability-function-cognitive-complexity]
// CHECK-NOTES: :[[@LINE-27]]:7: note: +1, including nesting penalty of 0, nesting level increased to 1{{$}}

// CHECK-NOTES: :[[@LINE-24]]:5: warning: function '~X' has cognitive complexity of 1 (threshold 0) [readability-function-cognitive-complexity]
// CHECK-NOTES: :[[@LINE-23]]:7: note: +1, including nesting penalty of 0, nesting level increased to 1{{$}}

// CHECK-NOTES: :[[@LINE-20]]:10: warning: function 'Y' has cognitive complexity of 1 (threshold 0) [readability-function-cognitive-complexity]
// CHECK-NOTES: :[[@LINE-19]]:7: note: +1, including nesting penalty of 0, nesting level increased to 1{{$}}

// CHECK-NOTES: :[[@LINE-16]]:17: warning: function 'Z' has cognitive complexity of 1 (threshold 0) [readability-function-cognitive-complexity]
// CHECK-NOTES: :[[@LINE-15]]:7: note: +1, including nesting penalty of 0, nesting level increased to 1{{$}}
  };
}

void unittest_b2_08_01() {
// CHECK-NOTES: :[[@LINE-1]]:6: warning: function 'unittest_b2_08_01' has cognitive complexity of 8 (threshold 0) [readability-function-cognitive-complexity]
  struct X {
    X() {
// CHECK-NOTES: :[[@LINE-1]]:5: note: nesting level increased to 1{{$}}
      if (true) {
// CHECK-NOTES: :[[@LINE-1]]:7: note: +2, including nesting penalty of 1, nesting level increased to 2{{$}}
      }
    }

    ~X() {
// CHECK-NOTES: :[[@LINE-1]]:5: note: nesting level increased to 1{{$}}
      if (true) {
// CHECK-NOTES: :[[@LINE-1]]:7: note: +2, including nesting penalty of 1, nesting level increased to 2{{$}}
      }
    }

    void Y() {
// CHECK-NOTES: :[[@LINE-1]]:5: note: nesting level increased to 1{{$}}
      if (true) {
// CHECK-NOTES: :[[@LINE-1]]:7: note: +2, including nesting penalty of 1, nesting level increased to 2{{$}}
      }
    }

    static void Z() {
// CHECK-NOTES: :[[@LINE-1]]:5: note: nesting level increased to 1{{$}}
      if (true) {
// CHECK-NOTES: :[[@LINE-1]]:7: note: +2, including nesting penalty of 1, nesting level increased to 2{{$}}
      }
    }

// CHECK-NOTES: :[[@LINE-28]]:5: warning: function 'X' has cognitive complexity of 1 (threshold 0) [readability-function-cognitive-complexity]
// CHECK-NOTES: :[[@LINE-27]]:7: note: +1, including nesting penalty of 0, nesting level increased to 1{{$}}

// CHECK-NOTES: :[[@LINE-24]]:5: warning: function '~X' has cognitive complexity of 1 (threshold 0) [readability-function-cognitive-complexity]
// CHECK-NOTES: :[[@LINE-23]]:7: note: +1, including nesting penalty of 0, nesting level increased to 1{{$}}

// CHECK-NOTES: :[[@LINE-20]]:10: warning: function 'Y' has cognitive complexity of 1 (threshold 0) [readability-function-cognitive-complexity]
// CHECK-NOTES: :[[@LINE-19]]:7: note: +1, including nesting penalty of 0, nesting level increased to 1{{$}}

// CHECK-NOTES: :[[@LINE-16]]:17: warning: function 'Z' has cognitive complexity of 1 (threshold 0) [readability-function-cognitive-complexity]
// CHECK-NOTES: :[[@LINE-15]]:7: note: +1, including nesting penalty of 0, nesting level increased to 1{{$}}
  };
}

void unittest_b2_08_02() {
// CHECK-NOTES: :[[@LINE-1]]:6: warning: function 'unittest_b2_08_02' has cognitive complexity of 2 (threshold 0) [readability-function-cognitive-complexity]
  auto fun = []() {
// CHECK-NOTES: :[[@LINE-1]]:14: note: nesting level increased to 1{{$}}
    if (true) {
// CHECK-NOTES: :[[@LINE-1]]:5: note: +2, including nesting penalty of 1, nesting level increased to 2{{$}}
    }
  };
}

//----------------------------------------------------------------------------//
//-------------------------- B2. Nesting increments --------------------------//
//----------------------------------------------------------------------------//
// Check that every thing listed in B3 of the specification does indeed       //
// recieve the penalty of the current nesting level                           //
//----------------------------------------------------------------------------//

void unittest_b3_00() {
// CHECK-NOTES: :[[@LINE-1]]:6: warning: function 'unittest_b3_00' has cognitive complexity of 3 (threshold 0) [readability-function-cognitive-complexity]
  if (true) {
// CHECK-NOTES: :[[@LINE-1]]:3: note: +1, including nesting penalty of 0, nesting level increased to 1{{$}}
    if (true) {
// CHECK-NOTES: :[[@LINE-1]]:5: note: +2, including nesting penalty of 1, nesting level increased to 2{{$}}
    }
  }
}

void unittest_b3_01() {
// CHECK-NOTES: :[[@LINE-1]]:6: warning: function 'unittest_b3_01' has cognitive complexity of 3 (threshold 0) [readability-function-cognitive-complexity]
  if (true) {
// CHECK-NOTES: :[[@LINE-1]]:3: note: +1, including nesting penalty of 0, nesting level increased to 1{{$}}
    int i = 1 ? 1 : 0;
// CHECK-NOTES: :[[@LINE-1]]:13: note: +2, including nesting penalty of 1, nesting level increased to 2{{$}}
// FIXME: would be nice to point at the '?' symbol. does not seem to be possible
  }
}

void unittest_b3_02(int x) {
// CHECK-NOTES: :[[@LINE-1]]:6: warning: function 'unittest_b3_02' has cognitive complexity of 3 (threshold 0) [readability-function-cognitive-complexity]
  if (true) {
// CHECK-NOTES: :[[@LINE-1]]:3: note: +1, including nesting penalty of 0, nesting level increased to 1{{$}}
    switch (x) {
// CHECK-NOTES: :[[@LINE-1]]:5: note: +2, including nesting penalty of 1, nesting level increased to 2{{$}}
    case -1:
      return;
    default:
      return;
    }
  }
}

void unittest_b3_03() {
// CHECK-NOTES: :[[@LINE-1]]:6: warning: function 'unittest_b3_03' has cognitive complexity of 3 (threshold 0) [readability-function-cognitive-complexity]
  if (true) {
// CHECK-NOTES: :[[@LINE-1]]:3: note: +1, including nesting penalty of 0, nesting level increased to 1{{$}}
    for (;;) {
// CHECK-NOTES: :[[@LINE-1]]:5: note: +2, including nesting penalty of 1, nesting level increased to 2{{$}}
    }
  }
}

void unittest_b3_04() {
// CHECK-NOTES: :[[@LINE-1]]:6: warning: function 'unittest_b3_04' has cognitive complexity of 3 (threshold 0) [readability-function-cognitive-complexity]
  if (true) {
// CHECK-NOTES: :[[@LINE-1]]:3: note: +1, including nesting penalty of 0, nesting level increased to 1{{$}}
    SomeClass C;
    for (int i : C) {
// CHECK-NOTES: :[[@LINE-1]]:5: note: +2, including nesting penalty of 1, nesting level increased to 2{{$}}
    }
  }
}

void unittest_b3_05() {
// CHECK-NOTES: :[[@LINE-1]]:6: warning: function 'unittest_b3_05' has cognitive complexity of 3 (threshold 0) [readability-function-cognitive-complexity]
  if (true) {
// CHECK-NOTES: :[[@LINE-1]]:3: note: +1, including nesting penalty of 0, nesting level increased to 1{{$}}
    while (true) {
// CHECK-NOTES: :[[@LINE-1]]:5: note: +2, including nesting penalty of 1, nesting level increased to 2{{$}}
    }
  }
}

void unittest_b3_06() {
// CHECK-NOTES: :[[@LINE-1]]:6: warning: function 'unittest_b3_06' has cognitive complexity of 3 (threshold 0) [readability-function-cognitive-complexity]
  if (true) {
// CHECK-NOTES: :[[@LINE-1]]:3: note: +1, including nesting penalty of 0, nesting level increased to 1{{$}}
    do {
// CHECK-NOTES: :[[@LINE-1]]:5: note: +2, including nesting penalty of 1, nesting level increased to 2{{$}}
    } while (true);
  }
}

void unittest_b3_07() {
// CHECK-NOTES: :[[@LINE-1]]:6: warning: function 'unittest_b3_07' has cognitive complexity of 3 (threshold 0) [readability-function-cognitive-complexity]
  if (true) {
// CHECK-NOTES: :[[@LINE-1]]:3: note: +1, including nesting penalty of 0, nesting level increased to 1{{$}}
    try {
    } catch (...) {
// CHECK-NOTES: :[[@LINE-1]]:7: note: +2, including nesting penalty of 1, nesting level increased to 2{{$}}
    }
  }
}
