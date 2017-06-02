; RUN: llc -verify-machineinstrs -mtriple=armv7-linux-gnu -O0 %s -o - | FileCheck %s
; RUN: llc -verify-machineinstrs -mtriple=thumbv8-linux-gnu -O0 %s -o - | FileCheck %s
; RUN: llc -verify-machineinstrs -mtriple=thumbv6m-none-eabi -O0 %s -o - | FileCheck %s --check-prefix=CHECK-T1

; CHECK-T1-NOT: ldrex
; CHECK-T1-NOT: strex

define { i8, i1 } @test_cmpxchg_8(i8* %addr, i8 %desired, i8 %new) nounwind {
; CHECK-LABEL: test_cmpxchg_8:
; CHECK:     dmb ish
; CHECK:     uxtb [[DESIRED:r[0-9]+]], [[DESIRED]]
; CHECK: [[RETRY:.LBB[0-9]+_[0-9]+]]:
; CHECK:     ldrexb [[OLD:r[0-9]+]], [r0]
; CHECK:     cmp [[OLD]], [[DESIRED]]
; CHECK:     bne [[DONE:.LBB[0-9]+_[0-9]+]]
; CHECK:     strexb [[STATUS:r[0-9]+]], r2, [r0]
; CHECK:     cmp{{(\.w)?}} [[STATUS]], #0
; CHECK:     bne [[RETRY]]
; CHECK: [[DONE]]:
; CHECK:     cmp{{(\.w)?}} [[OLD]], [[DESIRED]]
; CHECK:     {{moveq|movweq}} {{r[0-9]+}}, #1
; CHECK:     dmb ish
  %res = cmpxchg i8* %addr, i8 %desired, i8 %new seq_cst monotonic
  ret { i8, i1 } %res
}

define { i16, i1 } @test_cmpxchg_16(i16* %addr, i16 %desired, i16 %new) nounwind {
; CHECK-LABEL: test_cmpxchg_16:
; CHECK:     dmb ish
; CHECK:     uxth [[DESIRED:r[0-9]+]], [[DESIRED]]
; CHECK: [[RETRY:.LBB[0-9]+_[0-9]+]]:
; CHECK:     ldrexh [[OLD:r[0-9]+]], [r0]
; CHECK:     cmp [[OLD]], [[DESIRED]]
; CHECK:     bne [[DONE:.LBB[0-9]+_[0-9]+]]
; CHECK:     strexh [[STATUS:r[0-9]+]], r2, [r0]
; CHECK:     cmp{{(\.w)?}} [[STATUS]], #0
; CHECK:     bne [[RETRY]]
; CHECK: [[DONE]]:
; CHECK:     cmp{{(\.w)?}} [[OLD]], [[DESIRED]]
; CHECK:     {{moveq|movweq}} {{r[0-9]+}}, #1
; CHECK:     dmb ish
  %res = cmpxchg i16* %addr, i16 %desired, i16 %new seq_cst monotonic
  ret { i16, i1 } %res
}

define { i32, i1 } @test_cmpxchg_32(i32* %addr, i32 %desired, i32 %new) nounwind {
; CHECK-LABEL: test_cmpxchg_32:
; CHECK:     dmb ish
; CHECK-NOT:     uxt
; CHECK: [[RETRY:.LBB[0-9]+_[0-9]+]]:
; CHECK:     ldrex [[OLD:r[0-9]+]], [r0]
; CHECK:     cmp [[OLD]], [[DESIRED]]
; CHECK:     bne [[DONE:.LBB[0-9]+_[0-9]+]]
; CHECK:     strex [[STATUS:r[0-9]+]], r2, [r0]
; CHECK:     cmp{{(\.w)?}} [[STATUS]], #0
; CHECK:     bne [[RETRY]]
; CHECK: [[DONE]]:
; CHECK:     cmp{{(\.w)?}} [[OLD]], [[DESIRED]]
; CHECK:     {{moveq|movweq}} {{r[0-9]+}}, #1
; CHECK:     dmb ish
  %res = cmpxchg i32* %addr, i32 %desired, i32 %new seq_cst monotonic
  ret { i32, i1 } %res
}

define { i64, i1 } @test_cmpxchg_64(i64* %addr, i64 %desired, i64 %new) nounwind {
; CHECK-LABEL: test_cmpxchg_64:
; CHECK:     dmb ish
; CHECK-NOT: uxt
; CHECK: [[RETRY:.LBB[0-9]+_[0-9]+]]:
; CHECK:     ldrexd [[OLDLO:r[0-9]+]], [[OLDHI:r[0-9]+]], [r0]
; CHECK:     cmp [[OLDLO]], r6
; CHECK:     cmpeq [[OLDHI]], r7
; CHECK:     bne [[DONE:.LBB[0-9]+_[0-9]+]]
; CHECK:     strexd [[STATUS:r[0-9]+]], r4, r5, [r0]
; CHECK:     cmp{{(\.w)?}} [[STATUS]], #0
; CHECK:     bne [[RETRY]]
; CHECK: [[DONE]]:
; CHECK:     dmb ish
  %res = cmpxchg i64* %addr, i64 %desired, i64 %new seq_cst monotonic
  ret { i64, i1 } %res
}

; If r9 and fp are reserved, cmpxchg can only use r0/r1, r2/r3, r4/r5, or r6/r7
; for the 64-bit inputs to ldrexd and strexd. Ensure fast-regalloc can find
; enough registers without spilling.
define i64 @test_cmpxchg_64_register_pressure(i64* %addr, i64 %desired, i64 %new) nounwind "no-frame-pointer-elim"="true" "target-features"="+reserve-r9" {
; CHECK-LABEL: test_cmpxchg_64_register_pressure:
  %addr.addr = alloca i64*, align 4
  %desired.addr = alloca i64, align 8
  %new.addr = alloca i64, align 8
  store i64* %addr, i64** %addr.addr, align 4
  store i64 %desired, i64* %desired.addr, align 8
  store i64 %new, i64* %new.addr, align 8
  br label %while.cond

while.cond:
  %addr.tmp = load i64*, i64** %addr.addr, align 4
  %desired.tmp = load i64, i64* %desired.addr, align 8
  %new.tmp = load i64, i64* %new.addr, align 8

; CHECK-DAG: mov [[NEWLO:r[0-9]+]], r3
; CHECK-NEXT:mov [[NEWHI:r[0-9]+]], {{r[0-9]+}}
; CHECK-DAG: ldrd [[DESIREDLO:r[0-9]+]], [[DESIREDHI:r[0-9]+]], [sp, #{{[0-9]+}}] @ 8-byte Reload
; CHECK-DAG: dmb ish
; CHECK: [[INNERRETRY:.LBB[0-9]+_[0-9]+]]:
; CHECK-NOT: {{ldr[^e]|str}}
; CHECK:     ldrexd [[OLDLO:r[0-9]+]], [[OLDHI:r[0-9]+]], {{\[}}[[ADDR:r[0-9]+]]{{\]}}
; CHECK-NOT: {{ldr|str}}
; CHECK:     cmp [[OLDLO]], [[DESIREDLO]]
; CHECK-NOT: {{ldr|str}}
; CHECK:     cmpeq [[OLDHI]], [[DESIREDHI]]
; CHECK-NOT: {{ldr|str}}
; CHECK:     bne [[INNERDONE:.LBB[0-9]+_[0-9]+]]
; CHECK-NOT: {{ldr|str[^e]}}
; CHECK:     strexd [[STATUS:r[0-9]+]], [[NEWLO]], [[NEWHI]], {{\[}}[[ADDR]]{{\]}}
; CHECK-NOT: {{ldr|str}}
; CHECK:     cmp{{(\.w)?}} [[STATUS]], #0
; CHECK-NOT: {{ldr|str}}
; CHECK:     bne [[INNERRETRY]]
; CHECK: [[INNERDONE]]:
; CHECK:     dmb ish
  %tmp = cmpxchg i64* %addr.tmp, i64 %desired.tmp, i64 %new.tmp seq_cst seq_cst

  %status = extractvalue { i64, i1 } %tmp, 1
  br i1 %status, label %done, label %while.cond

done:
  ret i64 0
}

define { i64, i1 } @test_nontrivial_args(i64* %addr, i64 %desired, i64 %new) {
; CHECK-LABEL: test_nontrivial_args:
; CHECK:     dmb ish
; CHECK-NOT: uxt
; CHECK: [[RETRY:.LBB[0-9]+_[0-9]+]]:
; CHECK:     ldrexd [[OLDLO:r[0-9]+]], [[OLDHI:r[0-9]+]], [r0]
; CHECK:     cmp [[OLDLO]], {{r[0-9]+}}
; CHECK:     cmpeq [[OLDHI]], {{r[0-9]+}}
; CHECK:     bne [[DONE:.LBB[0-9]+_[0-9]+]]
; CHECK:     strexd [[STATUS:r[0-9]+]], {{r[0-9]+}}, {{r[0-9]+}}, [r0]
; CHECK:     cmp{{(\.w)?}} [[STATUS]], #0
; CHECK:     bne [[RETRY]]
; CHECK: [[DONE]]:
; CHECK:     dmb ish

  %desired1 = add i64 %desired, 1
  %new1 = add i64 %new, 1
  %res = cmpxchg i64* %addr, i64 %desired1, i64 %new1 seq_cst seq_cst
  ret { i64, i1 } %res
}

; The following used to trigger an assertion when creating a spill on thumb2
; for a physreg with RC==GPRPairRegClass.
; CHECK-LABEL: test_cmpxchg_spillbug:
; CHECK: ldrexd
; CHECK: strexd
; CHECK: bne
define void @test_cmpxchg_spillbug() {
  %v = cmpxchg i64* undef, i64 undef, i64 undef seq_cst seq_cst
  ret void
}
