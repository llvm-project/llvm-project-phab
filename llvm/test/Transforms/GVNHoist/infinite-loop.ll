; RUN: opt -S -gvn-hoist < %s | FileCheck %s

; Check that nothing is hoisted in this case.
; CHECK: @bazv
; CHECK: if.then.i:
; CHECK: bitcast
; CHECK-NEXT: load
; CHECK: if.then4.i:
; CHECK: bitcast
; CHECK-NEXT: load

%class.bar = type { i8*, %class.base* }
%class.base = type { i32 (...)** }

; Function Attrs: noreturn nounwind uwtable
define void @bazv() local_unnamed_addr {
entry:
  %agg.tmp = alloca %class.bar, align 8
  %x.sroa.2.0..sroa_idx2 = getelementptr inbounds %class.bar, %class.bar* %agg.tmp, i64 0, i32 1
  store %class.base* null, %class.base** %x.sroa.2.0..sroa_idx2, align 8
  call void @_Z3foo3bar(%class.bar* nonnull %agg.tmp) #2
  %0 = load %class.base*, %class.base** %x.sroa.2.0..sroa_idx2, align 8
  %1 = bitcast %class.bar* %agg.tmp to %class.base*
  %cmp.i = icmp eq %class.base* %0, %1
  br i1 %cmp.i, label %if.then.i, label %if.else.i

if.then.i:                                        ; preds = %entry
  %2 = bitcast %class.base* %0 to void (%class.base*)***
  %vtable.i = load void (%class.base*)**, void (%class.base*)*** %2, align 8
  %vfn.i = getelementptr inbounds void (%class.base*)*, void (%class.base*)** %vtable.i, i64 2
  %3 = load void (%class.base*)*, void (%class.base*)** %vfn.i, align 8
  call void %3(%class.base* %0) #2
  br label %while.cond.preheader

if.else.i:                                        ; preds = %entry
  %tobool.i = icmp eq %class.base* %0, null
  br i1 %tobool.i, label %while.cond.preheader, label %if.then4.i

if.then4.i:                                       ; preds = %if.else.i
  %4 = bitcast %class.base* %0 to void (%class.base*)***
  %vtable6.i = load void (%class.base*)**, void (%class.base*)*** %4, align 8
  %vfn7.i = getelementptr inbounds void (%class.base*)*, void (%class.base*)** %vtable6.i, i64 3
  %5 = load void (%class.base*)*, void (%class.base*)** %vfn7.i, align 8
  call void %5(%class.base* nonnull %0) #2
  br label %while.cond.preheader

while.cond.preheader:                             ; preds = %if.then.i, %if.else.i, %if.then4.i
  br label %while.cond

while.cond:                                       ; preds = %while.cond.preheader, %while.cond
  %call = call i32 @sleep(i32 10) #2
  br label %while.cond
}

declare void @_Z3foo3bar(%class.bar*) local_unnamed_addr

declare i32 @sleep(i32) local_unnamed_addr

