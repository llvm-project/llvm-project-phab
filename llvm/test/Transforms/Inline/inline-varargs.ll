; RUN: opt -inline -S %s | FileCheck %s

; Check that the side effects of unused arguments happen.
; CHECK: %call{{.*}}callThis
; CHECK: %call{{.*}}callThat

; Verify that calleeY gets inlined. Call should not exist.
; CHECK-NOT: call{{.*}}calleeY

; Verify that calleeN* do not get inlined. Call should exist.
; CHECK: call{{.*}}calleeNMT
; CHECK: call{{.*}}calleeNMTAI
; CHECK: call{{.*}}calleeN
; CHECK: call{{.*}}calleeNAI

; ModuleID = 'inl.cpp'
;source_filename = "inl.cpp"
;target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
;target triple = "x86_64-scei-ps4"
%struct.__va_list_tag = type { i32, i32, i8*, i8* }
$_Z7calleeYiPKcz = comdat any
$_Z9calleeNMTiPKcz = comdat any
$_Z11calleeNMTAIiPKcz = comdat any
$_Z7calleeNiPKcz = comdat any
$_Z9calleeNAIiPKcz = comdat any
@.str = private unnamed_addr constant [4 x i8] c"abc\00", align 1

; Function Attrs: nounwind
define i32 @_Z6calleriPKcz(i32 %p, i8* %x, ...) #0 {
entry:
  %p.addr = alloca i32, align 4
  %x.addr = alloca i8*, align 8
  %r = alloca i32, align 4
  %q = alloca i32, align 4
  store i32 %p, i32* %p.addr, align 4
  store i8* %x, i8** %x.addr, align 8
  %call = call i32 @_Z8callThisv()
  %call1 = call i32 @_Z8callThatv()
  %call2 = call i32 (i32, i8*, ...) @_Z7calleeYiPKcz(i32 12, i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.str, i32 0, i32 0), i32* %p.addr, i32 %call, i32 2, i32 %call1)
  store i32 %call2, i32* %r, align 4
  %call3 = call i32 (i32, i8*, ...) @_Z9calleeNMTiPKcz(i32 12, i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.str, i32 0, i32 0), i32* %p.addr, i32 2, i32 3)
  %0 = load i32, i32* %r, align 4
  %add = add nsw i32 %0, %call3
  store i32 %add, i32* %r, align 4
  %call4 = call i32 (i32, i8*, ...) @_Z11calleeNMTAIiPKcz(i32 12, i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.str, i32 0, i32 0), i32* %p.addr, i32 2, i32 3)
  %1 = load i32, i32* %r, align 4
  %add5 = add nsw i32 %1, %call4
  store i32 %add5, i32* %r, align 4
  %2 = load i32, i32* %p.addr, align 4
  %inc = add nsw i32 %2, 1
  store i32 %inc, i32* %p.addr, align 4
  %call6 = call i32 (i32, i8*, ...) @_Z7calleeNiPKcz(i32 142, i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.str, i32 0, i32 0), i32* %p.addr, i32 2, i32 3)
  store i32 %call6, i32* %q, align 4
  %call7 = call i32 (i32, i8*, ...) @_Z9calleeNAIiPKcz(i32 122, i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.str, i32 0, i32 0), i32* %p.addr, i32 5, i32 3)
  %3 = load i32, i32* %q, align 4
  %add8 = add nsw i32 %3, %call7
  store i32 %add8, i32* %q, align 4
  %4 = load i32, i32* %r, align 4
  %5 = load i32, i32* %q, align 4
  %add9 = add nsw i32 %4, %5
  %6 = load i8*, i8** %x.addr, align 8
  %call10 = musttail call i32 (i32, i8*, ...) @_Z5addMTiPKcz(i32 %add9, i8* %6, ...)
  ret i32 %call10
}

; Function Attrs: inlinehint nounwind
define linkonce_odr i32 @_Z7calleeYiPKcz(i32 %a, i8* %m, ...) #1 comdat {
entry:
  %a.addr = alloca i32, align 4
  %m.addr = alloca i8*, align 8
  store i32 %a, i32* %a.addr, align 4
  store i8* %m, i8** %m.addr, align 8
  %0 = load i32, i32* %a.addr, align 4
  %add = add nsw i32 %0, 1
  %call = call i32 @_Z4add2i(i32 %add)
  ret i32 %call
}

declare i32 @_Z8callThisv() #2

declare i32 @_Z8callThatv() #2

; Function Attrs: inlinehint nounwind
define linkonce_odr i32 @_Z9calleeNMTiPKcz(i32 %a, i8* %m, ...) #1 comdat {
entry:
  %a.addr = alloca i32, align 4
  %m.addr = alloca i8*, align 8
  store i32 %a, i32* %a.addr, align 4
  store i8* %m, i8** %m.addr, align 8
  %0 = load i32, i32* %a.addr, align 4
  %add = add nsw i32 %0, 2
  %1 = load i8*, i8** %m.addr, align 8
  %call = musttail call i32 (i32, i8*, ...) @_Z5addMTiPKcz(i32 %add, i8* %1, i32 5, ...)
  ret i32 %call
}

; Function Attrs: inlinehint nounwind
define linkonce_odr i32 @_Z11calleeNMTAIiPKcz(i32 %a, i8* %m, ...) #3 comdat {
entry:
  %a.addr = alloca i32, align 4
  %m.addr = alloca i8*, align 8
  store i32 %a, i32* %a.addr, align 4
  store i8* %m, i8** %m.addr, align 8
  %0 = load i32, i32* %a.addr, align 4
  %sub = sub nsw i32 %0, 2
  %1 = load i8*, i8** %m.addr, align 8
  %call = musttail call i32 (i32, i8*, ...) @_Z5addMTiPKcz(i32 %sub, i8* %1, i32 6, ...)
  ret i32 %call
}

; Function Attrs: inlinehint nounwind
define linkonce_odr i32 @_Z7calleeNiPKcz(i32 %a, i8* %m, ...) #1 comdat {
entry:
  %a.addr = alloca i32, align 4
  %m.addr = alloca i8*, align 8
  %ap = alloca [1 x %struct.__va_list_tag], align 16
  store i32 %a, i32* %a.addr, align 4
  store i8* %m, i8** %m.addr, align 8
  %arraydecay = getelementptr inbounds [1 x %struct.__va_list_tag], [1 x %struct.__va_list_tag]* %ap, i32 0, i32 0
  %arraydecay1 = bitcast %struct.__va_list_tag* %arraydecay to i8*
  call void @llvm.va_start(i8* %arraydecay1)
  %0 = load i32, i32* %a.addr, align 4
  %add = add nsw i32 %0, 3
  %arraydecay2 = getelementptr inbounds [1 x %struct.__va_list_tag], [1 x %struct.__va_list_tag]* %ap, i32 0, i32 0
  %gp_offset_p = getelementptr inbounds %struct.__va_list_tag, %struct.__va_list_tag* %arraydecay2, i32 0, i32 0
  %gp_offset = load i32, i32* %gp_offset_p, align 16
  %fits_in_gp = icmp ule i32 %gp_offset, 40
  br i1 %fits_in_gp, label %vaarg.in_reg, label %vaarg.in_mem

vaarg.in_reg:                                     ; preds = %entry
  %1 = getelementptr inbounds %struct.__va_list_tag, %struct.__va_list_tag* %arraydecay2, i32 0, i32 3
  %reg_save_area = load i8*, i8** %1, align 16
  %2 = getelementptr i8, i8* %reg_save_area, i32 %gp_offset
  %3 = bitcast i8* %2 to i32*
  %4 = add i32 %gp_offset, 8
  store i32 %4, i32* %gp_offset_p, align 16
  br label %vaarg.end

vaarg.in_mem:                                     ; preds = %entry
  %overflow_arg_area_p = getelementptr inbounds %struct.__va_list_tag, %struct.__va_list_tag* %arraydecay2, i32 0, i32 2
  %overflow_arg_area = load i8*, i8** %overflow_arg_area_p, align 8
  %5 = bitcast i8* %overflow_arg_area to i32*
  %overflow_arg_area.next = getelementptr i8, i8* %overflow_arg_area, i32 8
  store i8* %overflow_arg_area.next, i8** %overflow_arg_area_p, align 8
  br label %vaarg.end

vaarg.end:                                        ; preds = %vaarg.in_mem, %vaarg.in_reg
  %vaarg.addr = phi i32* [ %3, %vaarg.in_reg ], [ %5, %vaarg.in_mem ]
  %6 = load i32, i32* %vaarg.addr, align 4
  %add3 = add nsw i32 %add, %6
  ret i32 %add3
}

; Function Attrs: alwaysinline inlinehint nounwind
define linkonce_odr i32 @_Z9calleeNAIiPKcz(i32 %a, i8* %m, ...) #3 comdat {
entry:
  %a.addr = alloca i32, align 4
  %m.addr = alloca i8*, align 8
  %ap = alloca [1 x %struct.__va_list_tag], align 16
  store i32 %a, i32* %a.addr, align 4
  store i8* %m, i8** %m.addr, align 8
  %arraydecay = getelementptr inbounds [1 x %struct.__va_list_tag], [1 x %struct.__va_list_tag]* %ap, i32 0, i32 0
  %arraydecay1 = bitcast %struct.__va_list_tag* %arraydecay to i8*
  call void @llvm.va_start(i8* %arraydecay1)
  %0 = load i32, i32* %a.addr, align 4
  %mul = mul nsw i32 %0, 2
  %arraydecay2 = getelementptr inbounds [1 x %struct.__va_list_tag], [1 x %struct.__va_list_tag]* %ap, i32 0, i32 0
  %gp_offset_p = getelementptr inbounds %struct.__va_list_tag, %struct.__va_list_tag* %arraydecay2, i32 0, i32 0
  %gp_offset = load i32, i32* %gp_offset_p, align 16
  %fits_in_gp = icmp ule i32 %gp_offset, 40
  br i1 %fits_in_gp, label %vaarg.in_reg, label %vaarg.in_mem

vaarg.in_reg:                                     ; preds = %entry
  %1 = getelementptr inbounds %struct.__va_list_tag, %struct.__va_list_tag* %arraydecay2, i32 0, i32 3
  %reg_save_area = load i8*, i8** %1, align 16
  %2 = getelementptr i8, i8* %reg_save_area, i32 %gp_offset
  %3 = bitcast i8* %2 to i32*
  %4 = add i32 %gp_offset, 8
  store i32 %4, i32* %gp_offset_p, align 16
  br label %vaarg.end

vaarg.in_mem:                                     ; preds = %entry
  %overflow_arg_area_p = getelementptr inbounds %struct.__va_list_tag, %struct.__va_list_tag* %arraydecay2, i32 0, i32 2
  %overflow_arg_area = load i8*, i8** %overflow_arg_area_p, align 8
  %5 = bitcast i8* %overflow_arg_area to i32*
  %overflow_arg_area.next = getelementptr i8, i8* %overflow_arg_area, i32 8
  store i8* %overflow_arg_area.next, i8** %overflow_arg_area_p, align 8
  br label %vaarg.end

vaarg.end:                                        ; preds = %vaarg.in_mem, %vaarg.in_reg
  %vaarg.addr = phi i32* [ %3, %vaarg.in_reg ], [ %5, %vaarg.in_mem ]
  %6 = load i32, i32* %vaarg.addr, align 4
  %add = add nsw i32 %mul, %6
  ret i32 %add
}

declare i32 @_Z5addMTiPKcz(i32, i8*, ...) #2

declare i32 @_Z4add2i(i32) #2

; Function Attrs: nounwind
declare void @llvm.va_start(i8*) #4

attributes #0 = { nounwind "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-features"="+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { inlinehint nounwind "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-features"="+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-features"="+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { alwaysinline inlinehint nounwind "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-features"="+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #4 = { nounwind }

!llvm.ident = !{!0}

!0 = !{!"clang version 4.0.0 (trunk)"}

;Generated by processing this file with: 
; clang++.exe -cc1 -triple x86_64-scei-ps4 inl.cpp -emit-llvm
;
;#include <stdarg.h>
;int addMT(int p, const char *, ...);
;int add2(int);
;
;inline int calleeY(int a, const char *m, ...) {return add2(a+1);}
;inline int calleeNMT(int a, const char *m, ...) {return addMT(a+2,m, 5);}
;inline int //  __attribute__((always_inline))   // will add always_inline in .ll
;    calleeNMTAI(int a, const char *m, ...) {return addMT(a-2,m, 6);}
;
;// do not inline this
;inline int calleeN(int a, const char *m, ...) {
;  va_list ap;
;  va_start(ap, m);
;  return a+3 + va_arg(ap, int);
;}
;// even if it is always_inline
;inline int __attribute__((always_inline)) calleeNAI(int a, const char *m, ...) {
;  va_list ap;
;  va_start(ap, m);
;  return a*2 + va_arg(ap, int);
;}
;int callThis(); // calls to this must be preserved
;int callThat(); // calls to this must be preserved
;int caller(int p,const char *x, ...)
;{
;  int r = calleeY(12, "abc", &p, callThis(), 2, callThat()); // inline this
;  r += calleeNMT(12, "abc", &p, 2, 3); // No. calleeNMT has mustatil call
;  r += calleeNMTAI(12, "abc", &p, 2, 3); // Not even if it is alway_inline
;  p++;
;  int q = calleeN(142, "abc", &p, 2, 3); // No. calleeN has va_start
;  q +=  calleeNAI(122, "abc", &p, 5, 3); // Not even if it has always_inline
;  return addMT(r + q, x);
;}
;
;// and then  running following SED script on it
;// SED /define.*calleeNMTAI/s/#1/#3/
;// SED /call.*addMT/s/= call/= musttail call/
;// SED /call.*addMT/s/)$/, ...)/
