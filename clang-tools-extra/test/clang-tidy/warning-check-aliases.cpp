// RUN: clang-tidy %s -- 2>&1 | FileCheck -implicit-check-not='{{warning:|error:}}' %s
// RUN: clang-tidy %s -checks='-*,cert-err54-cpp' -- 2>&1 | FileCheck -implicit-check-not='{{warning:|error:}}' -check-prefix=CHECK2 %s

class B {};
class D : public B {};

void f() {
  try {

  } catch (B &X) {

  } catch (D &Y) {
  }
}

//CHECK: :12:12: warning: exception of type 'D &' will be caught by earlier handler [clang-diagnostic-exceptions]
//CHECK: :10:12: note: for type 'B &'

//CHECK2: :12:12: warning: exception of type 'D &' will be caught by earlier handler [cert-err54-cpp]
//CHECK2: :10:12: note: for type 'B &'
