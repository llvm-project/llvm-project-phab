// RUN: %check_clang_tidy %s cert-dcl51-cpp %t -- -- -I %S/Inputs/Headers

#include "dcl51.h"

#define _MACRO_NAME_
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: do not use global names that start with an underscore [cert-dcl51-cpp]      
// CHECK-MESSAGES: :[[@LINE-2]]:9: warning: identifiers that begin with an underscore followed by an uppercase letter are reserved to the implementation [cert-dcl51-cpp]
#define switch
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: do not use a macro name that is identical to a keyword or an attribute [cert-dcl51-cpp]
#define else__if
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: identifiers containing double underscores are reserved to the implementation [cert-dcl51-cpp]

int _global_variable;
// CHECK-MESSAGES: :[[@LINE-1]]:5: warning: do not use global names that start with an underscore [cert-dcl51-cpp]      
int _Upper;
// CHECK-MESSAGES: :[[@LINE-1]]:5: warning: do not use global names that start with an underscore [cert-dcl51-cpp]
// CHECK-MESSAGES: :[[@LINE-2]]:5: warning: identifiers that begin with an underscore followed by an uppercase letter are reserved to the implementation [cert-dcl51-cpp]       
int __variable;
// CHECK-MESSAGES: :[[@LINE-1]]:5: warning: do not use global names that start with an underscore [cert-dcl51-cpp]
// CHECK-MESSAGES: :[[@LINE-2]]:5: warning: identifiers containing double underscores are reserved to the implementation [cert-dcl51-cpp]
int some__variable;
// CHECK-MESSAGES: :[[@LINE-1]]:5: warning: identifiers containing double underscores are reserved to the implementation [cert-dcl51-cpp]
void fun__ction() {
  int local__variable;
}
// CHECK-MESSAGES: :[[@LINE-3]]:6: warning: identifiers containing double underscores are reserved to the implementation [cert-dcl51-cpp]
// CHECK-MESSAGES: :[[@LINE-3]]:7: warning: identifiers containing double underscores are reserved to the implementation [cert-dcl51-cpp]
void _Function() {
  int _Case;
}
// CHECK-MESSAGES: :[[@LINE-3]]:6: warning: do not use global names that start with an underscore [cert-dcl51-cpp]
// CHECK-MESSAGES: :[[@LINE-4]]:6: warning: identifiers that begin with an underscore followed by an uppercase letter are reserved to the implementation [cert-dcl51-cpp]
// CHECK-MESSAGES: :[[@LINE-4]]:7: warning: identifiers that begin with an underscore followed by an uppercase letter are reserved to the implementation [cert-dcl51-cpp]    

typedef int _Integer;
// CHECK-MESSAGES: :[[@LINE-1]]:13: warning: do not use global names that start with an underscore [cert-dcl51-cpp]
// CHECK-MESSAGES: :[[@LINE-2]]:13: warning: identifiers that begin with an underscore followed by an uppercase letter are reserved to the implementation [cert-dcl51-cpp]

using _Char = char;
// CHECK-MESSAGES: :[[@LINE-1]]:7: warning: do not use global names that start with an underscore [cert-dcl51-cpp]
// CHECK-MESSAGES: :[[@LINE-2]]:7: warning: identifiers that begin with an underscore followed by an uppercase letter are reserved to the implementation [cert-dcl51-cpp]

template<int _Number> void _fun__ction();
// CHECK-MESSAGES: :[[@LINE-1]]:14: warning: identifiers that begin with an underscore followed by an uppercase letter are reserved to the implementation [cert-dcl51-cpp]
// CHECK-MESSAGES: :[[@LINE-2]]:28: warning: do not use global names that start with an underscore [cert-dcl51-cpp]
// CHECK-MESSAGES: :[[@LINE-3]]:28: warning: identifiers containing double underscores are reserved to the implementation [cert-dcl51-cpp]

enum _global_names {
  _Variable = 1,
  second__variable = 2
};
// CHECK-MESSAGES: :[[@LINE-4]]:6: warning: do not use global names that start with an underscore [cert-dcl51-cpp]
// CHECK-MESSAGES: :[[@LINE-4]]:3: warning: identifiers that begin with an underscore followed by an uppercase letter are reserved to the implementation [cert-dcl51-cpp]
// CHECK-MESSAGES: :[[@LINE-4]]:3: warning: identifiers containing double underscores are reserved to the implementation [cert-dcl51-cpp]

void __Function() {
  _label__:
  if (true) {
    goto _label__;
  }
}
// CHECK-MESSAGES: :[[@LINE-6]]:6: warning: do not use global names that start with an underscore [cert-dcl51-cpp]
// CHECK-MESSAGES: :[[@LINE-7]]:6: warning: identifiers containing double underscores are reserved to the implementation [cert-dcl51-cpp]
// CHECK-MESSAGES: :[[@LINE-7]]:3: warning: identifiers containing double underscores are reserved to the implementation [cert-dcl51-cpp]

namespace _reserved_namespace_ {
  int _Variable;
  struct _Struct_name {
    void Fun__ction();
  };
}
// CHECK-MESSAGES: :[[@LINE-6]]:11: warning: do not use global names that start with an underscore [cert-dcl51-cpp]
// CHECK-MESSAGES: :[[@LINE-6]]:7: warning: identifiers that begin with an underscore followed by an uppercase letter are reserved to the implementation [cert-dcl51-cpp]
// CHECK-MESSAGES: :[[@LINE-6]]:10: warning: identifiers that begin with an underscore followed by an uppercase letter are reserved to the implementation [cert-dcl51-cpp]
// CHECK-MESSAGES: :[[@LINE-6]]:10: warning: identifiers containing double underscores are reserved to the implementation [cert-dcl51-cpp]

namespace {
  int _variable;
  class _Classname {
    void _Function();
  };
  namespace {
    int _still_global;
  }
}
// CHECK-MESSAGES: :[[@LINE-8]]:7: warning: do not use global names that start with an underscore [cert-dcl51-cpp]
// CHECK-MESSAGES: :[[@LINE-8]]:9: warning: do not use global names that start with an underscore [cert-dcl51-cpp]
// CHECK-MESSAGES: :[[@LINE-9]]:9: warning: identifiers that begin with an underscore followed by an uppercase letter are reserved to the implementation [cert-dcl51-cpp]
// CHECK-MESSAGES: :[[@LINE-9]]:10: warning: identifiers that begin with an underscore followed by an uppercase letter are reserved to the implementation [cert-dcl51-cpp]
// CHECK-MESSAGES: :[[@LINE-7]]:9: warning: do not use global names that start with an underscore [cert-dcl51-cpp]


// Something that doesn't trigger the check:
struct X {
  int *begin();
  int *end();
};

void function_for() {
  X x;
  for(int i : x) {
    // Intentionally left empty.
  }
}

#define MACRO_NAME
#define override
#define else_if

#define const const
#define inline __inline

int some_variable;

void fun_ction() {
  int _local_lower_variable;
}

typedef int Integer;
using Char = char;

template<int Number> void function();

enum my_enum {
  variable = 1,
  other_variable = 2
};

void Function() {
  _label_:
  if (true) {
    goto _label_;
  }
}

namespace my_space {
  int _variable;
  struct _struct_name_ {
    void _function();
  };
  namespace {
    int _my_variable;
  }
}

namespace {
  class Classname {
    void _my_function();
  };
  bool operator==(Classname a, double b);
}
