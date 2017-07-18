; RUN: llc -O=0 < %s 2>&1 | FileCheck %s --check-prefix=CHECK

; typedef long long __m128 __attribute__((__vector_size__(16)));
; typedef short int __m64 __attribute__((__vector_size__(8)));
;
; typedef double __m128d __attribute__((__vector_size__(16)));
; typedef long long __m128i __attribute__((__vector_size__(16)));
;
; typedef double __v2df __attribute__ ((__vector_size__ (16)));
; typedef long long __v2di __attribute__ ((__vector_size__ (16)));
; typedef short __v8hi __attribute__((__vector_size__(16)));
; typedef char __v16qi __attribute__((__vector_size__(16)));
; 
; void test51(__m64 m64, __m128i m128, __m128d m128d, __v2df v2df, __v2di v2di, __v8hi v8hi, __v16qi v16qi ) {
;   int b = 0;
;   __asm__("" : "=r"(b) : "f"(m64));
;   __asm__("" : "=r"(b) : "f"(m128));
;   __asm__("" : "=r"(b) : "f"(m128d));
;   __asm__("" : "=r"(b) : "f"(v2df));
;   __asm__("" : "=r"(b) : "f"(v2di));
;   __asm__("" : "=r"(b) : "f"(v8hi));
;   __asm__("" : "=r"(b) : "f"(v16qi));
; }

; ModuleID = 'asm-inconsistent-operand-constraints-fpreg-vector.c'
source_filename = "asm-inconsistent-operand-constraints-fpreg-vector.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; CHECK:	error: Inconsistent operand constraints, in an __asm__ at line {{[0-9]*}}
; CHECK-NEXT:	error: Inconsistent operand constraints, in an __asm__ at line {{[0-9]*}}
; CHECK-NEXT:	error: Inconsistent operand constraints, in an __asm__ at line {{[0-9]*}}
; CHECK-NEXT:	error: Inconsistent operand constraints, in an __asm__ at line {{[0-9]*}}
; CHECK-NEXT:	error: Inconsistent operand constraints, in an __asm__ at line {{[0-9]*}}
; CHECK-NEXT:	error: Inconsistent operand constraints, in an __asm__ at line {{[0-9]*}}
; CHECK-NEXT:	error: Inconsistent operand constraints, in an __asm__ at line {{[0-9]*}}

; Function Attrs: noinline nounwind optnone uwtable
define void @test51(double %m64.coerce, <2 x i64> %m128, <2 x double> %m128d, <2 x double> %v2df, <2 x i64> %v2di, <8 x i16> %v8hi, <16 x i8> %v16qi) #0 {
entry:
  %m64 = alloca <4 x i16>, align 8
  %m64.addr = alloca <4 x i16>, align 8
  %m128.addr = alloca <2 x i64>, align 16
  %m128d.addr = alloca <2 x double>, align 16
  %v2df.addr = alloca <2 x double>, align 16
  %v2di.addr = alloca <2 x i64>, align 16
  %v8hi.addr = alloca <8 x i16>, align 16
  %v16qi.addr = alloca <16 x i8>, align 16
  %b = alloca i32, align 4
  %0 = bitcast <4 x i16>* %m64 to double*
  store double %m64.coerce, double* %0, align 8
  %m641 = load <4 x i16>, <4 x i16>* %m64, align 8
  store <4 x i16> %m641, <4 x i16>* %m64.addr, align 8
  store <2 x i64> %m128, <2 x i64>* %m128.addr, align 16
  store <2 x double> %m128d, <2 x double>* %m128d.addr, align 16
  store <2 x double> %v2df, <2 x double>* %v2df.addr, align 16
  store <2 x i64> %v2di, <2 x i64>* %v2di.addr, align 16
  store <8 x i16> %v8hi, <8 x i16>* %v8hi.addr, align 16
  store <16 x i8> %v16qi, <16 x i8>* %v16qi.addr, align 16
  store i32 0, i32* %b, align 4
  %1 = load <4 x i16>, <4 x i16>* %m64.addr, align 8
  %2 = call i32 asm "", "=r,f,~{dirflag},~{fpsr},~{flags}"(<4 x i16> %1) #1, !srcloc !2
  store i32 %2, i32* %b, align 4
  %3 = load <2 x i64>, <2 x i64>* %m128.addr, align 16
  %4 = call i32 asm "", "=r,f,~{dirflag},~{fpsr},~{flags}"(<2 x i64> %3) #1, !srcloc !3
  store i32 %4, i32* %b, align 4
  %5 = load <2 x double>, <2 x double>* %m128d.addr, align 16
  %6 = call i32 asm "", "=r,f,~{dirflag},~{fpsr},~{flags}"(<2 x double> %5) #1, !srcloc !4
  store i32 %6, i32* %b, align 4
  %7 = load <2 x double>, <2 x double>* %v2df.addr, align 16
  %8 = call i32 asm "", "=r,f,~{dirflag},~{fpsr},~{flags}"(<2 x double> %7) #1, !srcloc !5
  store i32 %8, i32* %b, align 4
  %9 = load <2 x i64>, <2 x i64>* %v2di.addr, align 16
  %10 = call i32 asm "", "=r,f,~{dirflag},~{fpsr},~{flags}"(<2 x i64> %9) #1, !srcloc !6
  store i32 %10, i32* %b, align 4
  %11 = load <8 x i16>, <8 x i16>* %v8hi.addr, align 16
  %12 = call i32 asm "", "=r,f,~{dirflag},~{fpsr},~{flags}"(<8 x i16> %11) #1, !srcloc !7
  store i32 %12, i32* %b, align 4
  %13 = load <16 x i8>, <16 x i8>* %v16qi.addr, align 16
  %14 = call i32 asm "", "=r,f,~{dirflag},~{fpsr},~{flags}"(<16 x i8> %13) #1, !srcloc !8
  store i32 %14, i32* %b, align 4
  ret void
}

attributes #0 = { noinline nounwind optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind readnone }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 5.0.0 (trunk 306467)"}
!2 = !{i32 631}
!3 = !{i32 667}
!4 = !{i32 704}
!5 = !{i32 742}
!6 = !{i32 779}
!7 = !{i32 816}
!8 = !{i32 853}
