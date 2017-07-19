; RUN: llc < %s -mtriple=arm-linux -mcpu=generic | FileCheck %s

define i32 @sadd(i32 %a, i32 %b) local_unnamed_addr #0 {
entry:
  %0 = tail call { i32, i1 } @llvm.sadd.with.overflow.i32(i32 %a, i32 %b)
  %1 = extractvalue { i32, i1 } %0, 1
  br i1 %1, label %trap, label %cont

trap:
  tail call void @llvm.trap() #2
  unreachable

cont:
  %2 = extractvalue { i32, i1 } %0, 0
  ret i32 %2

  ; CHECK-LABEL: sadd:
  ; CHECK: mov
  ; CHECK-NOT: mov
  ; CHECK: movvc
}

define i32 @uadd(i32 %a, i32 %b) local_unnamed_addr #0 {
entry:
  %0 = tail call { i32, i1 } @llvm.uadd.with.overflow.i32(i32 %a, i32 %b)
  %1 = extractvalue { i32, i1 } %0, 1
  br i1 %1, label %trap, label %cont

trap:
  tail call void @llvm.trap() #2
  unreachable

cont:
  %2 = extractvalue { i32, i1 } %0, 0
  ret i32 %2

  ; CHECK-LABEL: uadd:
  ; CHECK: mov
  ; CHECK-NOT: mov
  ; CHECK: movhs
}

define i32 @ssub(i32 %a, i32 %b) local_unnamed_addr #0 {
entry:
  %0 = tail call { i32, i1 } @llvm.ssub.with.overflow.i32(i32 %a, i32 %b)
  %1 = extractvalue { i32, i1 } %0, 1
  br i1 %1, label %trap, label %cont

trap:
  tail call void @llvm.trap() #2
  unreachable

cont:
  %2 = extractvalue { i32, i1 } %0, 0
  ret i32 %2

  ; CHECK-LABEL: ssub:
  ; CHECK-NOT: mov
  ; CHECK: movvc
}

define i32 @usub(i32 %a, i32 %b) local_unnamed_addr #0 {
entry:
  %0 = tail call { i32, i1 } @llvm.usub.with.overflow.i32(i32 %a, i32 %b)
  %1 = extractvalue { i32, i1 } %0, 1
  br i1 %1, label %trap, label %cont

trap:
  tail call void @llvm.trap() #2
  unreachable

cont:
  %2 = extractvalue { i32, i1 } %0, 0
  ret i32 %2

  ; CHECK-LABEL: usub:
  ; CHECK-NOT: mov
  ; CHECK: movhs
}

declare void @llvm.trap() #2
declare { i32, i1 } @llvm.sadd.with.overflow.i32(i32, i32) #1
declare { i32, i1 } @llvm.uadd.with.overflow.i32(i32, i32) #1
declare { i32, i1 } @llvm.ssub.with.overflow.i32(i32, i32) #1
declare { i32, i1 } @llvm.usub.with.overflow.i32(i32, i32) #1
