; RUN: opt < %s -basicaa -functionattrs -rpo-functionattrs -S | FileCheck %s
; RUN: opt < %s -aa-pipeline=basic-aa -passes='cgscc(function-attrs),rpo-functionattrs' -S | FileCheck %s

; CHECK: define i32 @leaf() #0
define i32 @leaf() {
  ret i32 1
}

; CHECK: define i32 @self_rec() #1
define i32 @self_rec() {
  %a = call i32 @self_rec()
  ret i32 4
}

; CHECK: define i32 @indirect_rec() #1
define i32 @indirect_rec() {
  %a = call i32 @indirect_rec2()
  ret i32 %a
}
; CHECK: define i32 @indirect_rec2() #1
define i32 @indirect_rec2() {
  %a = call i32 @indirect_rec()
  ret i32 %a
}

; CHECK: define i32 @extern() #1
define i32 @extern() {
  %a = call i32 @k()
  ret i32 %a
}
declare i32 @k() readnone

; CHECK: Function Attrs: norecurse
; CHECK-NEXT: define void @intrinsic(i8* nocapture %dest, i8* nocapture readonly %src, i32 %len)
define void @intrinsic(i8* %dest, i8* %src, i32 %len) {
  call void @llvm.memcpy.p0i8.p0i8.i32(i8* %dest, i8* %src, i32 %len, i32 1, i1 false)
  ret void
}
declare void @llvm.memcpy.p0i8.p0i8.i32(i8*, i8*, i32, i32, i1)

; CHECK: define internal i32 @called_by_norecurse() #0
define internal i32 @called_by_norecurse() {
  %a = call i32 @k()
  ret i32 %a
}
define void @m() norecurse {
  %a = call i32 @called_by_norecurse()
  ret void
}

; CHECK: define internal i32 @called_by_norecurse_indirectly() #0
define internal i32 @called_by_norecurse_indirectly() {
  %a = call i32 @k()
  ret i32 %a
}
define internal void @o() {
  %a = call i32 @called_by_norecurse_indirectly()
  ret void
}
define void @p() norecurse {
  call void @o()
  ret void
}

; CHECK: define void @checkRecursiveIntrinsics() {
define void @checkRecursiveIntrinsics() {
  %result = tail call i64 (i64, i32, i8*, i32, ...) @llvm.experimental.patchpoint.i64(i64 2, i32 15, i8* null, i32 4, i64 0, i64 0, i64 0, i64 0)
  tail call void (i64, i32, i8*, i32, ...) @llvm.experimental.patchpoint.void(i64 3, i32 15, i8* null, i32 2, i64 0, i64 %result)
  ret void
}

; CHECK: declare void @llvm.experimental.patchpoint.void
declare void @llvm.experimental.patchpoint.void(i64, i32, i8*, i32, ...)
declare i64 @llvm.experimental.patchpoint.i64(i64, i32, i8*, i32, ...)

; CHECK: attributes #0 = { norecurse readnone }
; CHECK: attributes #1 = { readnone }
