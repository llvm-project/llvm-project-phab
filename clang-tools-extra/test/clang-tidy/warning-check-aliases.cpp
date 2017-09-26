// RUN: clang-tidy %s -- 2>&1 | FileCheck -implicit-check-not='{{warning:|error:}}' %s
// RUN: clang-tidy %s -checks='-*,cert-err54-cpp' -- 2>&1 | FileCheck -implicit-check-not='{{warning:|error:}}' -check-prefix=CHECK2 %s
// RUN: clang-tidy %s -checks='-*,cert-err54-cpp' -- -Wno-exceptions 2>&1 | FileCheck -implicit-check-not='{{warning:|error:}}' -check-prefix=CHECK2 %s

class B {};
class D : public B {};

void f() {
  try {

  } catch (B &X) {

  } catch (D &Y) {
  }
}

//CHECK: :13:12: warning: exception of type 'D &' will be caught by earlier handler [clang-diagnostic-exceptions]
//CHECK: :11:12: note: for type 'B &'

//CHECK2: :13:12: warning: exception of type 'D &' will be caught by earlier handler [cert-err54-cpp]
//CHECK2: :11:12: note: for type 'B &'
