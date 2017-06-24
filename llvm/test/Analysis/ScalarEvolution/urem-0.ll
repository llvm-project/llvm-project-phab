; RUN: opt < %s -scalar-evolution -analyze | FileCheck %s

; CHECK: -->  ((-27 * (%a /u 27)) + %a)

define i8 @foo(i8 %a) {
        %t0 = urem i8 %a, 27
        ret i8 %t0
}
