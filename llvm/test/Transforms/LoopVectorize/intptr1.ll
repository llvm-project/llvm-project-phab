; RUN: opt < %s  -loop-vectorize  -S | FileCheck %s

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

;CHECK-LABEL: @test
;CHECK: vector.body:
;CHECK: load <4 x float>
;CHECK: fmul <4 x float>
;CHECK: store <4 x float>


@.str = private unnamed_addr constant [4 x i8] c"%f\0A\00", align 1

define void @test(float* %a, float* readnone %a_end, i64 %b) unnamed_addr {
entry:
  %cmp1 = icmp ult float* %a, %a_end
  br i1 %cmp1, label %for.body.preheader, label %for.end

for.body.preheader:                               ; preds = %entry
  br label %for.body

for.body:                                         ; preds = %for.body, %for.body.preheader
  %a.addr.03 = phi float* [ %incdec.ptr, %for.body ], [ %a, %for.body.preheader ]
  %b.addr.02 = phi i64 [ %add, %for.body ], [ %b, %for.body.preheader ]
  %tmp = inttoptr i64 %b.addr.02 to float*
  %tmp1 = load float, float* %tmp, align 4
  %mul.i = fmul float %tmp1, 4.200000e+01
  store float %mul.i, float* %a.addr.03, align 4
  %add = add nsw i64 %b.addr.02, 4
  %incdec.ptr = getelementptr inbounds float, float* %a.addr.03, i64 1
  %cmp = icmp ult float* %incdec.ptr, %a_end
  br i1 %cmp, label %for.body, label %for.end

for.end:                                          ; preds = %for.body, %entry
  ret void
}
