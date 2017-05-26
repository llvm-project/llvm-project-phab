target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

define void @tinkywinky() {
  tail call void @patatino()
  ret void
}

declare void @patatino()
