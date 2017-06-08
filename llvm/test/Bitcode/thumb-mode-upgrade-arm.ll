; RUN: llvm-dis -o - %s.bc | FileCheck %s

target triple = "arm-apple-ios"

define void @foo() #0 {
  ret void
}
define void @bar() {
  ret void
}

attributes #0 = { "target-features"="+thumb-mode" }

; CHECK: void @foo() [[ThumbModeAttr:#[0-7]]]
; CHECK: void @bar() [[NoThumbModeAttr:#[0-7]]]

; CHECK: attributes [[ThumbModeAttr]] = { "target-features"="+thumb-mode" }
; CHECK: attributes [[NoThumbModeAttr]] = { "target-features"="-thumb-mode" }
