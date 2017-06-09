; RUN: llvm-dis -o - %s.bc | FileCheck %s

target triple = "arm-apple-ios"

define void @foo() #0 {
  ret void
}
define void @zar() #1 {
  ret void
}
define void @bar() {
  ret void
}

attributes #0 = { "target-features"="+thumb-mode" }
attributes #1 = { "target-features"="+thumb-mode,+vfp3" }

; CHECK: void @foo() [[ThumbModeAttr1:#[0-7]]]
; CHECK: void @zar() [[ThumbModeAttr2:#[0-7]]]
; CHECK: void @bar() [[NoThumbModeAttr:#[0-7]]]

; CHECK: attributes [[ThumbModeAttr1]] = { "target-features"="+thumb-mode" }
; CHECK: attributes [[ThumbModeAttr2]] = { "target-features"="+thumb-mode,+vfp3" }
; CHECK: attributes [[NoThumbModeAttr]] = { "target-features"="-thumb-mode" }
