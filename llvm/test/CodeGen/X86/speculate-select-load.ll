; RUN: opt < %s -mcpu=x86-64 -S -x86-speculateload  | FileCheck %s
; RUN: opt < %s -mcpu=i386 -S -x86-speculateload  | FileCheck %s

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.S = type { i64, %struct.S*, %struct.S*, i64, i16, i64, i64, i64, i64, i64 }

;; Selecting between pointers of two members of the same structure with offset
;; smaller than a cache line's size (64 bytes) between them, load speculatively.
; Function Attrs: norecurse nounwind readonly uwtable
define %struct.S* @spec_load(i32 %x, %struct.S* nocapture readnone %A, %struct.S* nocapture readonly %B) local_unnamed_addr #0 {
entry:
; CHECK-LABEL:@spec_load
; CHECK: getelementptr inbounds %struct.S, %struct.S* %B, i64 0, i32 1
; CHECK: getelementptr inbounds %struct.S, %struct.S* %B, i64 0, i32 2
; CHECK: [[A:%[0-9]+]] = load %struct.S*, %struct.S** %{{.*}}, align 8
; CHECK: [[B:%[0-9]+]] = load %struct.S*, %struct.S** %{{.*}}, align 8
; CHECK: select i1 %tobool, %struct.S* [[A]], %struct.S* [[B]]

  %tobool = icmp eq i32 %x, 0
  %b = getelementptr inbounds %struct.S, %struct.S* %B, i64 0, i32 1
  %c = getelementptr inbounds %struct.S, %struct.S* %B, i64 0, i32 2
  %A.addr.0.in = select i1 %tobool, %struct.S** %c, %struct.S** %b
  %A.addr.0 = load %struct.S*, %struct.S** %A.addr.0.in, align 8
  ret %struct.S* %A.addr.0
}

;; Selecting between pointers of two members of the same structure with offset
;; greater than a cache line's size (64 bytes) between them, thus do not load speculatively.
; Function Attrs: norecurse nounwind readonly uwtable
define i64 @no_spec_load(i32 %x, i64 %A, %struct.S* nocapture readonly %B) local_unnamed_addr #0 {
entry:
; CHECK-LABEL:@no_spec_load
; CHECK: [[A:%[a-z]+]] = getelementptr inbounds %struct.S, %struct.S* %B, i64 0, i32 0
; CHECK: [[B:%[a-z]+]] = getelementptr inbounds %struct.S, %struct.S* %B, i64 0, i32 9
; CHECK: select i1 %tobool, i64* [[B]], i64* [[A]]
; CHECK-NEXT: load i64, i64* %{{.*}}, align 8

  %tobool = icmp eq i32 %x, 0
  %a = getelementptr inbounds %struct.S, %struct.S* %B, i64 0, i32 0
  %j = getelementptr inbounds %struct.S, %struct.S* %B, i64 0, i32 9
  %A.addr.0.in = select i1 %tobool, i64* %j, i64* %a
  %A.addr.0 = load i64, i64* %A.addr.0.in, align 8
  ret i64 %A.addr.0
}

;; Selecting into an aggregate type of the struct, do not load speculatively
%struct.S2 = type { i32, [10 x i32] }

; Function Attrs: norecurse nounwind readonly uwtable
define i32 @no_load_agg(%struct.S2* nocapture readonly %s1, %struct.S2* nocapture readnone %s2, i32 %x) local_unnamed_addr #0 {
entry:
; CHECK-LABEL:@no_load_agg
; CHECK: [[A:%[a-z]+]] = getelementptr inbounds %struct.S2, %struct.S2* %s1, i64 0, i32 0
; CHECK: [[B:%[a-z]+]] = getelementptr inbounds %struct.S2, %struct.S2* %s1, i64 0, i32 1, i64 10
; CHECK: %retval.0.in = select i1 %tobool, i32* [[B]], i32* [[A]]

  %tobool = icmp eq i32 %x, 0
  %a = getelementptr inbounds %struct.S2, %struct.S2* %s1, i64 0, i32 0
  %arrayidx = getelementptr inbounds %struct.S2, %struct.S2* %s1, i64 0, i32 1, i64 10
  %retval.0.in = select i1 %tobool, i32* %arrayidx, i32* %a
  %retval.0 = load i32, i32* %retval.0.in, align 4
  ret i32 %retval.0
}



attributes #0 = { norecurse nounwind readonly uwtable "target-cpu"="core-avx2" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87"}

