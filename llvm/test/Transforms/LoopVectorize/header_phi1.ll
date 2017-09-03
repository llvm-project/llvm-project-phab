;RUN: opt < %s  -loop-vectorize  -S | FileCheck %s

;CHECK-LABEL: @foo
;CHECK: vector.body:
;CHECK: store <4 x i32>
;CHECK: load <4 x i32>

target triple = "x86_64-grtev4-linux-gnu"

define  void @foo() {
entry:
  br label %loop

loop:
  %t1 = phi i32* [ %t3, %loop ], [ null, %entry ]
  %t2 = phi i32* [ %t5, %loop ], [ undef, %entry ]
  %t3 = getelementptr inbounds i32, i32* %t1, i64 1
  store i32 0, i32* %t1, align 4
  %t4 = load i32, i32* %t3, align 4
  %t5 = getelementptr inbounds i32, i32* %t2, i64 1
  %t6 = icmp ugt i32* undef, %t5
  br i1 %t6, label %loop, label %exit

exit:
  ret void

}

!llvm.ident = !{!0}

!0 = !{!"clang version 6.0.0 (trunk 311306) (llvm/trunk 311373)"}
