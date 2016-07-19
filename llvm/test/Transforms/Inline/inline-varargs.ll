; RUN: opt -inline -S %s | FileCheck %s

; Check that the side effects of unused arguments happen.
; CHECK: %call{{.*}}callThis

; Verify that calleeY gets inlined. Call should not exist.
; CHECK-NOT: %call{{.*}}calleeY

; Verify that calleeN does not get inlined. Call should exist.
; CHECK: %call{{.*}}calleeN

; ModuleID = 'inl.cpp'
source_filename = "inl.cpp"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.__va_list_tag = type { i32, i32, i8*, i8* }

$_Z7calleeYiiPKcz = comdat any

$_Z7calleeNiiPKcz = comdat any

@.str = private unnamed_addr constant [4 x i8] c"abc\00", align 1

; Function Attrs: nounwind
define i32 @_Z6calleri(i32 %p) #0 {
entry:
  %p.addr = alloca i32, align 4
  %r = alloca i32, align 4
  %q = alloca i32, align 4
  store i32 %p, i32* %p.addr, align 4
  %call = call i32 @_Z8callThisv()
  %call1 = call i32 (i32, i32, i8*, ...) @_Z7calleeYiiPKcz(i32 12, i32 34, i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.str, i32 0, i32 0), i32* %p.addr, i32 %call, i32 2, i32 3)
  store i32 %call1, i32* %r, align 4
  %0 = load i32, i32* %p.addr, align 4
  %inc = add nsw i32 %0, 1
  store i32 %inc, i32* %p.addr, align 4
  %call2 = call i32 (i32, i32, i8*, ...) @_Z7calleeNiiPKcz(i32 112, i32 134, i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.str, i32 0, i32 0), i32* %p.addr, i32 2, i32 3)
  store i32 %call2, i32* %q, align 4
  %1 = load i32, i32* %r, align 4
  %2 = load i32, i32* %q, align 4
  %add = add nsw i32 %1, %2
  ret i32 %add
}

; Function Attrs: inlinehint nounwind
define linkonce_odr i32 @_Z7calleeYiiPKcz(i32 %a, i32 %b, i8* %m, ...) #1 comdat {
entry:
  %a.addr = alloca i32, align 4
  %b.addr = alloca i32, align 4
  %m.addr = alloca i8*, align 8
  store i32 %a, i32* %a.addr, align 4
  store i32 %b, i32* %b.addr, align 4
  store i8* %m, i8** %m.addr, align 8
  %0 = load i32, i32* %a.addr, align 4
  %1 = load i32, i32* %b.addr, align 4
  %add = add nsw i32 %0, %1
  ret i32 %add
}

declare i32 @_Z8callThisv() #2

; Function Attrs: inlinehint nounwind
define linkonce_odr i32 @_Z7calleeNiiPKcz(i32 %a, i32 %b, i8* %m, ...) #3 comdat {
entry:
  %a.addr = alloca i32, align 4
  %b.addr = alloca i32, align 4
  %m.addr = alloca i8*, align 8
  %ap = alloca [1 x %struct.__va_list_tag], align 16
  store i32 %a, i32* %a.addr, align 4
  store i32 %b, i32* %b.addr, align 4
  store i8* %m, i8** %m.addr, align 8
  %arraydecay = getelementptr inbounds [1 x %struct.__va_list_tag], [1 x %struct.__va_list_tag]* %ap, i32 0, i32 0
  %arraydecay1 = bitcast %struct.__va_list_tag* %arraydecay to i8*
  call void @llvm.va_start(i8* %arraydecay1)
  %0 = load i32, i32* %a.addr, align 4
  %1 = load i32, i32* %b.addr, align 4
  %add = add nsw i32 %0, %1
  %arraydecay2 = getelementptr inbounds [1 x %struct.__va_list_tag], [1 x %struct.__va_list_tag]* %ap, i32 0, i32 0
  %gp_offset_p = getelementptr inbounds %struct.__va_list_tag, %struct.__va_list_tag* %arraydecay2, i32 0, i32 0
  %gp_offset = load i32, i32* %gp_offset_p, align 16
  %fits_in_gp = icmp ule i32 %gp_offset, 40
  br i1 %fits_in_gp, label %vaarg.in_reg, label %vaarg.in_mem

vaarg.in_reg:                                     ; preds = %entry
  %2 = getelementptr inbounds %struct.__va_list_tag, %struct.__va_list_tag* %arraydecay2, i32 0, i32 3
  %reg_save_area = load i8*, i8** %2, align 16
  %3 = getelementptr i8, i8* %reg_save_area, i32 %gp_offset
  %4 = bitcast i8* %3 to i32*
  %5 = add i32 %gp_offset, 8
  store i32 %5, i32* %gp_offset_p, align 16
  br label %vaarg.end

vaarg.in_mem:                                     ; preds = %entry
  %overflow_arg_area_p = getelementptr inbounds %struct.__va_list_tag, %struct.__va_list_tag* %arraydecay2, i32 0, i32 2
  %overflow_arg_area = load i8*, i8** %overflow_arg_area_p, align 8
  %6 = bitcast i8* %overflow_arg_area to i32*
  %overflow_arg_area.next = getelementptr i8, i8* %overflow_arg_area, i32 8
  store i8* %overflow_arg_area.next, i8** %overflow_arg_area_p, align 8
  br label %vaarg.end

vaarg.end:                                        ; preds = %vaarg.in_mem, %vaarg.in_reg
  %vaarg.addr = phi i32* [ %4, %vaarg.in_reg ], [ %6, %vaarg.in_mem ]
  %7 = load i32, i32* %vaarg.addr, align 4
  %add3 = add nsw i32 %add, %7
  ret i32 %add3
}

; Function Attrs: nounwind
declare void @llvm.va_start(i8*) #4

attributes #0 = { nounwind "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-features"="+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { inlinehint nounwind "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-features"="+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-features"="+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { inlinehint nounwind "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-features"="+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #4 = { nounwind }

!llvm.ident = !{!0}

!0 = !{!"clang version 3.9.0 (trunk 275529)"}
; Generated by 'clang -cc1 -S -inl.cpp -emit-llvm' with following file.
; #include <stdarg.h>
; inline int calleeY(int a, int b, const char *m, ...) {return a+b;}
; inline int calleeN(int a, int b, const char *m, ...) {
;   va_list ap;
;   va_start(ap, m);
;   return a+b + va_arg(ap, int);
; }
; int callThis();
; int caller(int p)
; {
;   int r = calleeY(12, 34, "abc", &p, callThis(), 2, 3);
;   p++;
;   int q = calleeN(112, 134, "abc", &p, 2, 3);
;   return r + q;
; }

