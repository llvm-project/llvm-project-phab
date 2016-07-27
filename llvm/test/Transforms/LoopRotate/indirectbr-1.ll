; RUN: opt -S < %s -loop-rotate -verify-dom-info -verify-loop-info | FileCheck %s
; ModuleID = 'ind.c'
source_filename = "ind.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"
; Check that the loop-latch with indirect branch is not rotated.
; CHECK-NOT: {{.*}}.lr
; CHECK-NOT: {{.*}}.lr.ph

@f.x = internal global [3 x i8*] [i8* blockaddress(@f, %F), i8* blockaddress(@f, %G), i8* blockaddress(@f, %H)], align 16

; Function Attrs: nounwind uwtable
define i32 @f(i32 %i, i32 %j, i32 %k) #0 {
entry:
  %retval = alloca i32, align 4
  %i.addr = alloca i32, align 4
  %j.addr = alloca i32, align 4
  %k.addr = alloca i32, align 4
  store i32 %i, i32* %i.addr, align 4
  store i32 %j, i32* %j.addr, align 4
  store i32 %k, i32* %k.addr, align 4
  br label %F

F:                                                ; preds = %entry, %indirectgoto
  %0 = load i32, i32* %i.addr, align 4
  %1 = load i32, i32* %j.addr, align 4
  %cmp = icmp sgt i32 %0, %1
  br i1 %cmp, label %if.then, label %if.else

if.then:                                          ; preds = %F
  br label %G

if.else:                                          ; preds = %F
  %2 = load i32, i32* %i.addr, align 4
  %3 = load i32, i32* %k.addr, align 4
  %cmp1 = icmp sgt i32 %2, %3
  br i1 %cmp1, label %if.then2, label %if.end

if.then2:                                         ; preds = %if.else
  br label %H

if.end:                                           ; preds = %if.else
  br label %if.end3

if.end3:                                          ; preds = %if.end
  %call = call i32 (...) @z()
  %idxprom = sext i32 %call to i64
  %arrayidx = getelementptr inbounds [3 x i8*], [3 x i8*]* @f.x, i64 0, i64 %idxprom
  %4 = load i8*, i8** %arrayidx, align 8
  br label %indirectgoto

G:                                                ; preds = %if.then, %indirectgoto
  %call4 = call i32 (...) @g()
  store i32 %call4, i32* %retval, align 4
  br label %return

H:                                                ; preds = %if.then2, %indirectgoto
  %call5 = call i32 (...) @h()
  store i32 %call5, i32* %retval, align 4
  br label %return

return:                                           ; preds = %H, %G
  %5 = load i32, i32* %retval, align 4
  ret i32 %5

indirectgoto:                                     ; preds = %if.end3
  %indirect.goto.dest = phi i8* [ %4, %if.end3 ]
  indirectbr i8* %indirect.goto.dest, [label %F, label %G, label %H]
}

declare i32 @z(...) #1

declare i32 @g(...) #1

declare i32 @h(...) #1

attributes #0 = { nounwind uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.ident = !{!0}

!0 = !{!"clang version 3.9.0 "}
