; RUN: opt < %s  -loop-vectorize  -S | FileCheck %s

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

;CHECK-LABEL: @test
;CHECK: vector.body:
;CHECK: load <8 x i8>
;CHECK: mul <8 x i8>
;CHECK: store <8 x i8>


; Function Attrs: noinline norecurse nounwind uwtable
define  void @test(i8* %a, i8* readnone %a_end, i64 %b) unnamed_addr  {
entry:
  %cmp1 = icmp ult i8* %a, %a_end
  br i1 %cmp1, label %for.body.preheader, label %for.end

for.body.preheader:                               ; preds = %entry
  %b.i8 = inttoptr i64 %b to i8*
  br label %for.body

for.body:                                         ; preds = %for.body.preheader, %for.body
  %a.addr.03 = phi i8* [ %incdec.ptr, %for.body ], [ %a, %for.body.preheader ]
  %b.addr.i8 = phi i8* [ %b.addr.i8.inc, %for.body ], [ %b.i8, %for.body.preheader ]
  %b.addr.i64 = phi i64 [ %b.addr.i64.inc, %for.body ], [ %b, %for.body.preheader ]
  %l = load i8, i8* %b.addr.i8, align 4 
  %mul.i = mul i8 %l, 4
  store i8 %mul.i, i8* %a.addr.03, align 4
  %b.addr.i8.2 = inttoptr i64 %b.addr.i64 to i8*
  %b.addr.i8.inc = getelementptr inbounds i8, i8* %b.addr.i8.2, i64 1
  %b.addr.i64.inc = ptrtoint i8* %b.addr.i8.inc to i64
  %incdec.ptr = getelementptr inbounds i8, i8* %a.addr.03, i64 1
  %cmp = icmp ult i8* %incdec.ptr, %a_end
  br i1 %cmp, label %for.body, label %for.end

for.end:                                          ; preds = %for.body, %entry
  ret void
}



