;RUN: opt -mergesimilarfunc -mergesimilarfunc-level=all -S < %s | FileCheck %s
;
; Test whether mergefunc merges allocas of different sizes correctly
;

target datalayout = "e-m:e-p:32:32-i1:32-i64:64-a:0-v32:32-n16:32"

%struct.A = type { i32, i32 }
%struct.B = type { i32, i32, i32 }

; Function Attrs: nounwind optsize
define void @f1() #0 {
; CHECK-LABEL: @f1__merged(
; CHECK: alloca %struct.A
; CHECK: alloca %struct.B
entry:
  %a = alloca %struct.A, align 4
  %0 = bitcast %struct.A* %a to i8*
  call void @externalFun(i8* %0) #2
  call void @externalFun(i8* %0) #2
  call void @externalFun(i8* %0) #2
  call void @externalFun(i8* %0) #2
  call void @externalFun(i8* %0) #2
  call void @externalFun(i8* %0) #2
  call void @externalFun(i8* %0) #2
  call void @externalFun(i8* %0) #2
  call void @externalFun(i8* %0) #2
  call void @externalFun(i8* %0) #2
  call void @externalFun(i8* %0) #2
  call void @externalFun(i8* %0) #2
  call void @externalFun(i8* %0) #2
  call void @externalFun(i8* %0) #2
  call void @externalFun(i8* %0) #2
  call void @externalFun(i8* %0) #2
  call void @externalFun(i8* %0) #2
  ret void
}

; Function Attrs: optsize
declare void @externalFun(i8*) #1

; Function Attrs: nounwind optsize
define void @f2() #0 {
entry:
  %a = alloca %struct.B, align 4
  %0 = bitcast %struct.B* %a to i8*
  call void @externalFun(i8* %0) #2
  call void @externalFun(i8* %0) #2
  call void @externalFun(i8* %0) #2
  call void @externalFun(i8* %0) #2
  call void @externalFun(i8* %0) #2
  call void @externalFun(i8* %0) #2
  call void @externalFun(i8* %0) #2
  call void @externalFun(i8* %0) #2
  call void @externalFun(i8* %0) #2
  call void @externalFun(i8* %0) #2
  call void @externalFun(i8* %0) #2
  call void @externalFun(i8* %0) #2
  call void @externalFun(i8* %0) #2
  call void @externalFun(i8* %0) #2
  call void @externalFun(i8* %0) #2
  call void @externalFun(i8* %0) #2
  call void @externalFun(i8* %0) #2
  ret void
}

; Function Attrs: nounwind optsize
define void @f3() #0 {
entry:
  %a = alloca i8, align 1
  call void @externalFun(i8* %a) #2
  call void @externalFun(i8* %a) #2
  call void @externalFun(i8* %a) #2
  call void @externalFun(i8* %a) #2
  call void @externalFun(i8* %a) #2
  call void @externalFun(i8* %a) #2
  call void @externalFun(i8* %a) #2
  call void @externalFun(i8* %a) #2
  call void @externalFun(i8* %a) #2
  call void @externalFun(i8* %a) #2
  call void @externalFun(i8* %a) #2
  call void @externalFun(i8* %a) #2
  call void @externalFun(i8* %a) #2
  call void @externalFun(i8* %a) #2
  call void @externalFun(i8* %a) #2
  call void @externalFun(i8* %a) #2
  call void @externalFun(i8* %a) #2
  ret void
}

attributes #0 = { nounwind optsize "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { optsize "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nounwind optsize }

