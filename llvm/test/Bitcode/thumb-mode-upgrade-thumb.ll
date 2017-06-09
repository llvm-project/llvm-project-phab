; RUN: llvm-dis -o - %s.bc | FileCheck %s

target triple = "thumb-apple-ios"

define void @foo() #0 {
  ret void
}
define void @zar() #1 {
  ret void
}
define void @bar() {
  ret void
}

attributes #0 = { "target-features"="-thumb-mode" }
attributes #1 = { "target-features"="-thumb-mode,+vfp3" }

; CHECK: void @foo() [[NoThumbModeAttr1:#[0-7]]]
; CHECK: void @zar() [[NoThumbModeAttr2:#[0-7]]]
; CHECK: void @bar() [[ThumbModeAttr:#[0-7]]]

; CHECK: attributes [[NoThumbModeAttr1]] = { "target-features"="-thumb-mode" }
; CHECK: attributes [[NoThumbModeAttr2]] = { "target-features"="-thumb-mode,+vfp3" }
; CHECK: attributes [[ThumbModeAttr]] = { "target-features"="+thumb-mode" }
