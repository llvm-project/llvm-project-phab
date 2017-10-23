; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py
; RUN: llc -ppc-strip-register-prefix -verify-machineinstrs \
; RUN:   -mtriple=powerpc64le-unknown-linux-gnu -mcpu=pwr8 < %s | FileCheck %s

%class.PB2 = type { [1 x i32], %class.PB1* }
%class.PB1 = type { [1 x i32], i64, i64, i32 }

; Function Attrs: norecurse nounwind readonly
define zeroext i1 @test1(%class.PB2* %s_a, %class.PB2* %s_b) local_unnamed_addr #0 {
; CHECK-LABEL: test1:
; CHECK:       # BB#0: # %entry
; CHECK-NEXT:    lwz 3, 0(3)
; CHECK-NEXT:    lwz 4, 0(4)
; CHECK-NEXT:    rlwinm 3, 3, 0, 28, 28
; CHECK-NEXT:    rlwinm 4, 4, 0, 28, 28
; CHECK-NEXT:    sub 3, 3, 4
; CHECK-NEXT:    rldicl 3, 3, 1, 63
; CHECK-NEXT:    blr
entry:
  %arrayidx.i6 = bitcast %class.PB2* %s_a to i32*
  %0 = load i32, i32* %arrayidx.i6, align 8, !tbaa !1
  %and.i = and i32 %0, 8
  %arrayidx.i37 = bitcast %class.PB2* %s_b to i32*
  %1 = load i32, i32* %arrayidx.i37, align 8, !tbaa !1
  %and.i4 = and i32 %1, 8
  %cmp.i5 = icmp ult i32 %and.i, %and.i4
  ret i1 %cmp.i5
}

; Function Attrs: norecurse nounwind readonly
define zeroext i1 @test2(%class.PB2* %s_a, %class.PB2* %s_b) local_unnamed_addr #0 {
; CHECK-LABEL: test2:
; CHECK:       # BB#0: # %entry
; CHECK-NEXT:    lwz 3, 0(3)
; CHECK-NEXT:    lwz 4, 0(4)
; CHECK-NEXT:    rlwinm 3, 3, 0, 28, 28
; CHECK-NEXT:    rlwinm 4, 4, 0, 28, 28
; CHECK-NEXT:    sub 3, 4, 3
; CHECK-NEXT:    rldicl 3, 3, 1, 63
; CHECK-NEXT:    xori 3, 3, 1
; CHECK-NEXT:    blr
entry:
  %arrayidx.i6 = bitcast %class.PB2* %s_a to i32*
  %0 = load i32, i32* %arrayidx.i6, align 8, !tbaa !1
  %and.i = and i32 %0, 8
  %arrayidx.i37 = bitcast %class.PB2* %s_b to i32*
  %1 = load i32, i32* %arrayidx.i37, align 8, !tbaa !1
  %and.i4 = and i32 %1, 8
  %cmp.i5 = icmp ule i32 %and.i, %and.i4
  ret i1 %cmp.i5
}

; Function Attrs: norecurse nounwind readonly
define zeroext i1 @test3(%class.PB2* %s_a, %class.PB2* %s_b) local_unnamed_addr #0 {
; CHECK-LABEL: test3:
; CHECK:       # BB#0: # %entry
; CHECK-NEXT:    lwz 3, 0(3)
; CHECK-NEXT:    lwz 4, 0(4)
; CHECK-NEXT:    rlwinm 3, 3, 0, 28, 28
; CHECK-NEXT:    rlwinm 4, 4, 0, 28, 28
; CHECK-NEXT:    sub 3, 4, 3
; CHECK-NEXT:    rldicl 3, 3, 1, 63
; CHECK-NEXT:    blr
entry:
  %arrayidx.i6 = bitcast %class.PB2* %s_a to i32*
  %0 = load i32, i32* %arrayidx.i6, align 8, !tbaa !1
  %and.i = and i32 %0, 8
  %arrayidx.i37 = bitcast %class.PB2* %s_b to i32*
  %1 = load i32, i32* %arrayidx.i37, align 8, !tbaa !1
  %and.i4 = and i32 %1, 8
  %cmp.i5 = icmp ugt i32 %and.i, %and.i4
  ret i1 %cmp.i5
}

; Function Attrs: norecurse nounwind readonly
define zeroext i1 @test4(%class.PB2* %s_a, %class.PB2* %s_b) local_unnamed_addr #0 {
; CHECK-LABEL: test4:
; CHECK:       # BB#0: # %entry
; CHECK-NEXT:    lwz 3, 0(3)
; CHECK-NEXT:    lwz 4, 0(4)
; CHECK-NEXT:    rlwinm 3, 3, 0, 28, 28
; CHECK-NEXT:    rlwinm 4, 4, 0, 28, 28
; CHECK-NEXT:    sub 3, 3, 4
; CHECK-NEXT:    rldicl 3, 3, 1, 63
; CHECK-NEXT:    xori 3, 3, 1
; CHECK-NEXT:    blr
entry:
  %arrayidx.i6 = bitcast %class.PB2* %s_a to i32*
  %0 = load i32, i32* %arrayidx.i6, align 8, !tbaa !1
  %and.i = and i32 %0, 8
  %arrayidx.i37 = bitcast %class.PB2* %s_b to i32*
  %1 = load i32, i32* %arrayidx.i37, align 8, !tbaa !1
  %and.i4 = and i32 %1, 8
  %cmp.i5 = icmp uge i32 %and.i, %and.i4
  ret i1 %cmp.i5
}

!1 = !{!2, !2, i64 0}
!2 = !{!"int", !3, i64 0}
!3 = !{!"omnipotent char", !4, i64 0}
!4 = !{!"Simple C++ TBAA"}
