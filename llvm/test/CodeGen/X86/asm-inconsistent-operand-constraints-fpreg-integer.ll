; RUN: llc -O=0 < %s 2>&1 | FileCheck %s --check-prefix=CHECK

; void test54(short s, int i, long l, long long ll, float f, double d, long double ld) {
;    int b = 0;
;  __asm__("" : "=r"(b) : "f"(f));
;  __asm__("" : "=r"(b) : "f"(d));
;  __asm__("" : "=r"(b) : "f"(ld));
;  __asm__("" : "=r"(b) : "f"(s));
;  __asm__("" : "=r"(b) : "f"(i));
;  __asm__("" : "=r"(b) : "f"(l));
;  __asm__("" : "=r"(b) : "f"(ll));
;}


; ModuleID = 'asm-inconsistent-operand-constraints-fpreg-integer.c'
source_filename = "asm-inconsistent-operand-constraints-fpreg-integer.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; CHECK:	error: Inconsistent operand constraints, in an __asm__ at line {{[0-9]*}}
; CHECK-NEXT:	error: Inconsistent operand constraints, in an __asm__ at line {{[0-9]*}}
; CHECK-NEXT:	error: Inconsistent operand constraints, in an __asm__ at line {{[0-9]*}}
; CHECK-NEXT:	error: Inconsistent operand constraints, in an __asm__ at line {{[0-9]*}}

; Function Attrs: noinline nounwind optnone uwtable
define void @test54(i16 signext %s, i32 %i, i64 %l, i64 %ll, float %f, double %d, x86_fp80 %ld) #0 {
entry:
  %s.addr = alloca i16, align 2
  %i.addr = alloca i32, align 4
  %l.addr = alloca i64, align 8
  %ll.addr = alloca i64, align 8
  %f.addr = alloca float, align 4
  %d.addr = alloca double, align 8
  %ld.addr = alloca x86_fp80, align 16
  %b = alloca i32, align 4
  store i16 %s, i16* %s.addr, align 2
  store i32 %i, i32* %i.addr, align 4
  store i64 %l, i64* %l.addr, align 8
  store i64 %ll, i64* %ll.addr, align 8
  store float %f, float* %f.addr, align 4
  store double %d, double* %d.addr, align 8
  store x86_fp80 %ld, x86_fp80* %ld.addr, align 16
  store i32 0, i32* %b, align 4
  %0 = load float, float* %f.addr, align 4
  %1 = call i32 asm "", "=r,f,~{dirflag},~{fpsr},~{flags}"(float %0) #1, !srcloc !2
  store i32 %1, i32* %b, align 4
  %2 = load double, double* %d.addr, align 8
  %3 = call i32 asm "", "=r,f,~{dirflag},~{fpsr},~{flags}"(double %2) #1, !srcloc !3
  store i32 %3, i32* %b, align 4
  %4 = load x86_fp80, x86_fp80* %ld.addr, align 16
  %5 = call i32 asm "", "=r,f,~{dirflag},~{fpsr},~{flags}"(x86_fp80 %4) #1, !srcloc !4
  store i32 %5, i32* %b, align 4
  %6 = load i16, i16* %s.addr, align 2
  %7 = call i32 asm "", "=r,f,~{dirflag},~{fpsr},~{flags}"(i16 %6) #1, !srcloc !5
  store i32 %7, i32* %b, align 4
  %8 = load i32, i32* %i.addr, align 4
  %9 = call i32 asm "", "=r,f,~{dirflag},~{fpsr},~{flags}"(i32 %8) #1, !srcloc !6
  store i32 %9, i32* %b, align 4
  %10 = load i64, i64* %l.addr, align 8
  %11 = call i32 asm "", "=r,f,~{dirflag},~{fpsr},~{flags}"(i64 %10) #1, !srcloc !7
  store i32 %11, i32* %b, align 4
  %12 = load i64, i64* %ll.addr, align 8
  %13 = call i32 asm "", "=r,f,~{dirflag},~{fpsr},~{flags}"(i64 %12) #1, !srcloc !8
  store i32 %13, i32* %b, align 4
  ret void
}

attributes #0 = { noinline nounwind optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind readnone }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 5.0.0 (trunk 306467)"}
!2 = !{i32 113}
!3 = !{i32 147}
!4 = !{i32 181}
!5 = !{i32 217}
!6 = !{i32 251}
!7 = !{i32 285}
!8 = !{i32 319}
