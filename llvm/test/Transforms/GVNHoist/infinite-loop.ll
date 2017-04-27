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
define void @bazv() local_unnamed_addr #0 {
entry:
  %agg.tmp = alloca %class.bar, align 8
  %x.sroa.2.0..sroa_idx2 = getelementptr inbounds %class.bar, %class.bar* %agg.tmp, i64 0, i32 1
  store %class.base* null, %class.base** %x.sroa.2.0..sroa_idx2, align 8
  call void @_Z3foo3bar(%class.bar* nonnull %agg.tmp) #2
  %0 = load %class.base*, %class.base** %x.sroa.2.0..sroa_idx2, align 8, !tbaa !1
  %1 = bitcast %class.bar* %agg.tmp to %class.base*
  %cmp.i = icmp eq %class.base* %0, %1
  br i1 %cmp.i, label %if.then.i, label %if.else.i

if.then.i:                                        ; preds = %entry
  %2 = bitcast %class.base* %0 to void (%class.base*)***
  %vtable.i = load void (%class.base*)**, void (%class.base*)*** %2, align 8, !tbaa !6
  %vfn.i = getelementptr inbounds void (%class.base*)*, void (%class.base*)** %vtable.i, i64 2
  %3 = load void (%class.base*)*, void (%class.base*)** %vfn.i, align 8
  call void %3(%class.base* %0) #2
  br label %while.cond.preheader

if.else.i:                                        ; preds = %entry
  %tobool.i = icmp eq %class.base* %0, null
  br i1 %tobool.i, label %while.cond.preheader, label %if.then4.i

if.then4.i:                                       ; preds = %if.else.i
  %4 = bitcast %class.base* %0 to void (%class.base*)***
  %vtable6.i = load void (%class.base*)**, void (%class.base*)*** %4, align 8, !tbaa !6
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

declare void @_Z3foo3bar(%class.bar*) local_unnamed_addr #1

declare i32 @sleep(i32) local_unnamed_addr #1

attributes #0 = { noreturn nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nounwind }

!llvm.ident = !{!0}

!0 = !{!"clang version 5.0.0  (llvm/trunk 301510)"}
!1 = !{!2, !3, i64 8}
!2 = !{!"_ZTS3bar", !3, i64 0, !3, i64 8}
!3 = !{!"any pointer", !4, i64 0}
!4 = !{!"omnipotent char", !5, i64 0}
!5 = !{!"Simple C++ TBAA"}
!6 = !{!7, !7, i64 0}
!7 = !{!"vtable pointer", !5, i64 0}
