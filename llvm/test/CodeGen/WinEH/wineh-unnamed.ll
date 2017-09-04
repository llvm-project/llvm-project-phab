; Check we can compilte wineh file with unnamed function without colision on 
; funclet symbole name.
; We can improve this test and run it to see that generated program crashed but
; it will only work on windows targets
; RUN: llc < %s

; ModuleID = 'test.cpp'
source_filename = "test.cpp"
target datalayout = "e-m:w-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-windows-msvc19.0.24215"

%rtti.TypeDescriptor5 = type { i8**, i8*, [6 x i8] }
%eh.CatchableType = type { i32, i32, i32, i32, i32, i32, i32 }
%eh.CatchableTypeArray.2 = type { i32, [2 x i32] }
%eh.ThrowInfo = type { i32, i32, i32, i32 }

$"\01??_C@_03JIDPIAJI@err?$AA@" = comdat any

$"\01??_R0PEAD@8" = comdat any

$"_CT??_R0PEAD@88" = comdat any

$"\01??_R0PEAX@8" = comdat any

$"_CT??_R0PEAX@88" = comdat any

$_CTA2PEAD = comdat any

$_TIC2PEAD = comdat any

@"\01??_C@_03JIDPIAJI@err?$AA@" = linkonce_odr unnamed_addr constant [4 x i8] c"err\00", comdat, align 1
@"\01??_7type_info@@6B@" = external constant i8*
@"\01??_R0PEAD@8" = linkonce_odr global %rtti.TypeDescriptor5 { i8** @"\01??_7type_info@@6B@", i8* null, [6 x i8] c".PEAD\00" }, comdat
@__ImageBase = external constant i8
@"_CT??_R0PEAD@88" = linkonce_odr unnamed_addr constant %eh.CatchableType { i32 1, i32 trunc (i64 sub nuw nsw (i64 ptrtoint (%rtti.TypeDescriptor5* @"\01??_R0PEAD@8" to i64), i64 ptrtoint (i8* @__ImageBase to i64)) to i32), i32 0, i32 -1, i32 0, i32 8, i32 0 }, section ".xdata", comdat
@"\01??_R0PEAX@8" = linkonce_odr global %rtti.TypeDescriptor5 { i8** @"\01??_7type_info@@6B@", i8* null, [6 x i8] c".PEAX\00" }, comdat
@"_CT??_R0PEAX@88" = linkonce_odr unnamed_addr constant %eh.CatchableType { i32 1, i32 trunc (i64 sub nuw nsw (i64 ptrtoint (%rtti.TypeDescriptor5* @"\01??_R0PEAX@8" to i64), i64 ptrtoint (i8* @__ImageBase to i64)) to i32), i32 0, i32 -1, i32 0, i32 8, i32 0 }, section ".xdata", comdat
@_CTA2PEAD = linkonce_odr unnamed_addr constant %eh.CatchableTypeArray.2 { i32 2, [2 x i32] [i32 trunc (i64 sub nuw nsw (i64 ptrtoint (%eh.CatchableType* @"_CT??_R0PEAD@88" to i64), i64 ptrtoint (i8* @__ImageBase to i64)) to i32), i32 trunc (i64 sub nuw nsw (i64 ptrtoint (%eh.CatchableType* @"_CT??_R0PEAX@88" to i64), i64 ptrtoint (i8* @__ImageBase to i64)) to i32)] }, section ".xdata", comdat
@_TIC2PEAD = linkonce_odr unnamed_addr constant %eh.ThrowInfo { i32 1, i32 0, i32 0, i32 trunc (i64 sub nuw nsw (i64 ptrtoint (%eh.CatchableTypeArray.2* @_CTA2PEAD to i64), i64 ptrtoint (i8* @__ImageBase to i64)) to i32) }, section ".xdata", comdat

; Function Attrs: noinline uwtable
define void @"\01?raise@@YAXH@Z"(i32 %i) #0 {
entry:
  %i.addr = alloca i32, align 4
  %tmp = alloca i8*, align 8
  store i32 %i, i32* %i.addr, align 4
  %0 = load i32, i32* %i.addr, align 4
  %cmp = icmp eq i32 %0, 1
  br i1 %cmp, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  store i8* getelementptr inbounds ([4 x i8], [4 x i8]* @"\01??_C@_03JIDPIAJI@err?$AA@", i32 0, i32 0), i8** %tmp, align 8
  %1 = bitcast i8** %tmp to i8*
  call void @_CxxThrowException(i8* %1, %eh.ThrowInfo* @_TIC2PEAD) #3
  unreachable

if.end:                                           ; preds = %entry
  ret void
}

declare dllimport void @_CxxThrowException(i8*, %eh.ThrowInfo*)

; Function Attrs: noinline uwtable
define i32 @0(i32 %i) #0 personality i8* bitcast (i32 (...)* @__CxxFrameHandler3 to i8*) {
entry:
  %retval = alloca i32, align 4
  %i.addr = alloca i32, align 4
  store i32 %i, i32* %i.addr, align 4
  %0 = load i32, i32* %i.addr, align 4
  invoke void @"\01?raise@@YAXH@Z"(i32 %0)
          to label %invoke.cont unwind label %catch.dispatch

catch.dispatch:                                   ; preds = %entry
  %1 = catchswitch within none [label %catch] unwind to caller

catch:                                            ; preds = %catch.dispatch
  %2 = catchpad within %1 [i8* null, i32 64, i8* null]
  store i32 1, i32* %retval, align 4
  catchret from %2 to label %catchret.dest

invoke.cont:                                      ; preds = %entry
  store i32 0, i32* %retval, align 4
  br label %return

catchret.dest:                                    ; preds = %catch
  br label %return

try.cont:                                         ; No predecessors!
  call void @llvm.trap()
  unreachable

return:                                           ; preds = %catchret.dest, %invoke.cont
  %3 = load i32, i32* %retval, align 4
  ret i32 %3
}

declare i32 @__CxxFrameHandler3(...)

; Function Attrs: noreturn nounwind
declare void @llvm.trap() #1

; Function Attrs: noinline uwtable
define i32 @1(i32 %i) #0 personality i8* bitcast (i32 (...)* @__CxxFrameHandler3 to i8*) {
entry:
  %retval = alloca i32, align 4
  %i.addr = alloca i32, align 4
  store i32 %i, i32* %i.addr, align 4
  %0 = load i32, i32* %i.addr, align 4
  %add = sub nsw i32 %0, 1
  invoke void @"\01?raise@@YAXH@Z"(i32 %add)
          to label %invoke.cont unwind label %catch.dispatch

catch.dispatch:                                   ; preds = %entry
  %1 = catchswitch within none [label %catch] unwind to caller

catch:                                            ; preds = %catch.dispatch
  %2 = catchpad within %1 [i8* null, i32 64, i8* null]
  store i32 4, i32* %retval, align 4
  catchret from %2 to label %catchret.dest

invoke.cont:                                      ; preds = %entry
  store i32 2, i32* %retval, align 4
  br label %return

catchret.dest:                                    ; preds = %catch
  br label %return

try.cont:                                         ; No predecessors!
  call void @llvm.trap()
  unreachable

return:                                           ; preds = %catchret.dest, %invoke.cont
  %3 = load i32, i32* %retval, align 4
  ret i32 %3
}

; Function Attrs: noinline norecurse uwtable
define i32 @main(i32 %argc) #2 {
entry:
  %retval = alloca i32, align 4
  %argc.addr = alloca i32, align 4
  store i32 0, i32* %retval, align 4
  store i32 %argc, i32* %argc.addr, align 4
  %0 = load i32, i32* %argc.addr, align 4
  %call = call i32 @0(i32 %0)
  %1 = load i32, i32* %argc.addr, align 4
  %call1 = call i32 @1(i32 %1)
  %add = add nsw i32 %call, %call1
  ret i32 %add
}

attributes #0 = { noinline uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { noreturn nounwind }
attributes #2 = { noinline norecurse uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { noreturn }

!llvm.module.flags = !{!0}

!0 = !{i32 1, !"PIC Level", i32 2}
