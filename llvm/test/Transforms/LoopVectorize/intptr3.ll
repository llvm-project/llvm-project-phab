; RUN: opt < %s  -loop-vectorize  -S | FileCheck %s

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

;CHECK-LABEL: @test
;CHECK: vector.body:
;CHECK: load <4 x float>
;CHECK: fmul <4 x float>
;CHECK: store <4 x float>

; Function Attrs: noinline norecurse nounwind uwtable
define  void @test(float* %a, float* readnone %a_end, i64 %b) unnamed_addr  {
entry:
  %cmp1 = icmp ult float* %a, %a_end
  br i1 %cmp1, label %for.body.preheader, label %for.end

for.body.preheader:                               ; preds = %entry
  %b.float = inttoptr i64 %b to float*
  br label %for.body

for.body:                                         ; preds = %for.body.preheader, %for.body
  %a.addr.03 = phi float* [ %incdec.ptr, %for.body ], [ %a, %for.body.preheader ]
  %b.addr.float = phi float* [ %b.addr.float.inc, %for.body ], [ %b.float, %for.body.preheader ]
  %b.addr.i64 = phi i64 [ %b.addr.i64.inc, %for.body ], [ %b, %for.body.preheader ]
  %l = load float, float* %b.addr.float, align 4 
  %mul.i = fmul float %l, 4.200000e+01
  store float %mul.i, float* %a.addr.03, align 4
  %b.addr.float.2 = inttoptr i64 %b.addr.i64 to float*
  %b.addr.float.inc = getelementptr inbounds float, float* %b.addr.float.2, i64 1
  %b.addr.i64.inc = ptrtoint float* %b.addr.float.inc to i64
  %incdec.ptr = getelementptr inbounds float, float* %a.addr.03, i64 1
  %cmp = icmp ult float* %incdec.ptr, %a_end
  br i1 %cmp, label %for.body, label %for.end

for.end:                                          ; preds = %for.body, %entry
  ret void
}



