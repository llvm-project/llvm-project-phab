; RUN: llvm-dis -o - %s.bc | FileCheck %s

target triple = "thumb-apple-ios"

define void @foo() #0 {
  ret void
}
define void @bar() {
  ret void
}

attributes #0 = { "target-features"="-thumb-mode" }

; CHECK: void @foo() [[NoThumbModeAttr:#[0-7]]]
; CHECK: void @bar() [[ThumbModeAttr:#[0-7]]]

; CHECK: attributes [[NoThumbModeAttr]] = { "target-features"="-thumb-mode" }
; CHECK: attributes [[ThumbModeAttr]] = { "target-features"="+thumb-mode" }
