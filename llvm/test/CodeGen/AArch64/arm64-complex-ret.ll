; RUN: llc -mtriple=arm64-eabi -o - %s | FileCheck %s

define { i192, i192, i21, i192 } @foo(i192) {
; CHECK-LABEL: foo:
; CHECK-DAG: stp xzr, xzr, [x8, #8]
; CHECK-DAG: str xzr, [x8]
  ret { i192, i192, i21, i192 } {i192 0, i192 1, i21 2, i192 3}
}
