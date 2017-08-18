target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

declare i32 @b()
define i32 @a() readnone {
  %1 = call i32 @b()
  ret i32 %1
}

define i32 @c() norecurse {
  ret i32 4
}
