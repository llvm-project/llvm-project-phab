; RUN: opt < %s -inline -simplifycfg -S | FileCheck %s
; RUN: opt < %s -passes='cgscc(inline,function(simplify-cfg))' -S | FileCheck %s

; A call instruction with empty !callees metadata. This is undefined behavior,
; so the call is marked unreachable.
;
; CHECK-LABEL: @call.0.0(
; CHECK-NEXT:    unreachable
define void @call.0.0(void ()* %fp) {
  call void %fp(), !callees !{}
  ret void
}

; A call instruction with two possible callees that can both inlined.
;
; CHECK-LABEL: @call.2.2(
; CHECK-NEXT:    [[TMP0:%.*]] = icmp eq i64 (i64, i64)* %fp, @select_def_0
; CHECK-NEXT:    [[TMP1:%.*]] = select i1 [[TMP0]], i64 %x, i64 %y
; CHECK-NEXT:    ret i64 [[TMP1]]
;
define i64 @call.2.2(i64 %x, i64 %y, i64 (i64, i64)* %fp) {
  %tmp0 = call i64 %fp(i64 %x, i64 %y),
     !callees !{i64 (i64, i64)* @select_def_0, i64 (i64, i64)* @select_def_1}
  ret i64 %tmp0
}

; A call instruction with three possible callees, only one of which can be
; inlined. Since more than one possible callee remains after promotion, the
; cloned call is indirect.
;
; CHECK-LABEL: @call.3.1(
; CHECK-NEXT:    [[TMP0:%.*]] = icmp eq i64 (i64, i64)* %fp, @select_def_0
; CHECK-NEXT:    br i1 [[TMP0]], label %[[MERGE:.*]], label %[[ELSE:.*]]
; CHECK:       [[ELSE]]:
; CHECK-NEXT:    [[TMP1:%.*]] = call i64 %fp(i64 %x, i64 %y), !callees
; CHECK-NEXT:    br label %[[MERGE:.*]]
; CHECK:       [[MERGE]]:
; CHECK-NEXT:    [[TMP2:%.*]] = phi i64 [ [[TMP1]], %[[ELSE]] ], [ %x, {{.*}} ]
; CHECK-NEXT:    ret i64 [[TMP2]]
;
define i64 @call.3.1(i64 %x, i64 %y, i64 (i64, i64)* %fp) {
  %tmp0 = call i64 %fp(i64 %x, i64 %y),
     !callees !{i64 (i64, i64)* @select_def_0, i64 (i64, i64)* @select_dec_1,
                i64 (i64, i64)* @select_dec_0}
  ret i64 %tmp0
}

; A call instruction with two possible callees, only one of which can be
; inlined. Since only one possible callee remains after promotion, the cloned
; call is promoted as well.
;
; CHECK-LABEL: @call.2.1(
; CHECK-NEXT:    [[TMP0:%.*]] = icmp eq i64 (i64, i64)* %fp, @select_def_0
; CHECK-NEXT:    br i1 [[TMP0]], label %[[MERGE:.*]], label %[[ELSE:.*]]
; CHECK:       [[ELSE]]:
; CHECK-NEXT:    [[TMP1:%.*]] = call i64 @select_dec_1(i64 %x, i64 %y)
; CHECK-NEXT:    br label %[[MERGE:.*]]
; CHECK:       [[MERGE]]:
; CHECK-NEXT:    [[TMP2:%.*]] = phi i64 [ [[TMP1]], %[[ELSE]] ], [ %x, {{.*}} ]
; CHECK-NEXT:    ret i64 [[TMP2]]
;
define i64 @call.2.1(i64 %x, i64 %y, i64 (i64, i64)* %fp) {
  %tmp0 = call i64 %fp(i64 %x, i64 %y),
     !callees !{i64 (i64, i64)* @select_def_0, i64 (i64, i64)* @select_dec_1}
  ret i64 %tmp0
}

; A call instruction with two possible callees, only one of which can be
; inlined. Since only one possible callee remains after promotion, the cloned
; call is promoted as well. Bitcasts must be inserted for the promoted call's
; arguments and return value since the callee's function type doesn't match
; that of the function pointer.
;
; CHECK-LABEL: @call.2.1.bitcast(
; CHECK-NEXT:    [[TMP0:%.*]] = icmp eq i32* (i32**)* %fp, bitcast (i64* (i64**)* @load_def to i32* (i32**)*)
; CHECK-NEXT:    [[TMP1:%.*]] = bitcast i32** %x to i64**
; CHECK-NEXT:    br i1 [[TMP0]], label %[[THEN:.*]], label %[[ELSE:.*]]
; CHECK:       [[THEN]]:
; CHECK-NEXT:    [[TMP2:%.*]] = load i64*, i64** [[TMP1]]
; CHECK-NEXT:    br label %[[MERGE:.*]]
; CHECK:       [[ELSE]]:
; CHECK-NEXT:    [[TMP3:%.*]] = call i64* @load_dec(i64** [[TMP1]])
; CHECK-NEXT:    br label %[[MERGE:.*]]
; CHECK:       [[MERGE]]:
; CHECK-NEXT:    [[TMP4:%.*]] = phi i64* [ [[TMP3]], %[[ELSE]] ], [ [[TMP2]], %[[THEN]] ]
; CHECK-NEXT:    [[TMP5:%.*]] = bitcast i64* [[TMP4]] to i32*
; CHECK-NEXT:    ret i32* [[TMP5]]
;
define i32* @call.2.1.bitcast(i32** %x, i32* (i32**)* %fp) {
  %tmp0 = call i32* %fp(i32** %x),
     !callees !{i64* (i64**)* @load_def, i64* (i64**)* @load_dec}
  ret i32* %tmp0
}

; An invoke instruction with two possible callees, only one of which can be
; inlined. Since only one possible callee remains after promotion, the cloned
; invoke is promoted as well. The normal destination has a phi node that must
; be fixed.
;
; CHECK-LABEL: @invoke.2.1(
; CHECK:         br i1 undef, label %[[TRY:.*]], label %[[NORMAL:.*]]
; CHECK:       [[TRY]]:
; CHECK-NEXT:    [[TMP0:%.*]] = icmp eq i64* (i64**)* %op, @load_def
; CHECK-NEXT:    br i1 [[TMP0]], label %[[THEN:.*]], label %[[ELSE:.*]]
; CHECK:       [[THEN]]:
; CHECK-NEXT:    [[TMP1:%.*]] = load i64*, i64** %x
; CHECK-NEXT:    br label %[[NORMAL]]
; CHECK:       [[ELSE]]:
; CHECK:         [[TMP2:%.*]] = invoke i64* @load_dec(i64** %x)
; CHECK-NEXT:                      to label %[[NORMAL]] unwind label %[[EXCEPT:.*]]
; CHECK:       [[NORMAL]]:
; CHECK-NEXT:    [[TMP3:%.*]] = phi i64* [ null, %entry ], [ [[TMP1]], %[[THEN]] ], [ [[TMP2]], %[[ELSE]] ]
; CHECK-NEXT:    ret i64* [[TMP3]]
; CHECK:       [[EXCEPT]]:
; CHECK-NEXT:    [[TMP4:%.*]] = landingpad { i8*, i64 }
; CHECK-NEXT:                   cleanup
; CHECK-NEXT:    ret i64* null
;
define i64* @invoke.2.1(i64** %x, i64* (i64**)* %op) personality i64 (...)* null {
entry:
  br i1 undef, label %try, label %normal
try:
  %tmp0 = invoke i64* %op(i64 **%x)
             to label %normal unwind label %except,
                !callees !{i64* (i64**)* @load_def, i64* (i64**)* @load_dec}
normal:
  %tmp1 = phi i64* [ %tmp0, %try ], [ null, %entry ]
  ret i64* %tmp1
except:
  %tmp2 = landingpad {i8*, i64}
          cleanup
  ret i64* null
}

; An invoke instruction with two possible callees, only one of which can be
; inlined. Since only one possible callee remains after promotion, the cloned
; invoke is promoted as well. The normal destination has a phi node that must
; be fixed. Additionally, bitcasts must be inserted for the promoted invoke's
; arguments and return value since the callee's function type doesn't match
; that of the function pointer.
;
; CHECK-LABEL: @invoke.2.1.bitcast(
; CHECK:         br i1 undef, label %[[TRY:.*]], label %[[NORMAL:.*]]
; CHECK:       [[TRY]]:
; CHECK-NEXT:    [[TMP0:%.*]] = icmp eq i32* (i32**)* %op, bitcast (i64* (i64**)* @load_def to i32* (i32**)*)
; CHECK-NEXT:    [[TMP1:%.*]] = bitcast i32** %x to i64**
; CHECK-NEXT:    br i1 [[TMP0]], label %[[THEN:.*]], label %[[ELSE:.*]]
; CHECK:       [[THEN]]:
; CHECK-NEXT:    [[TMP2:%.*]] = load i64*, i64** [[TMP1]]
; CHECK-NEXT:    br label %[[MERGE:.*]]
; CHECK:       [[ELSE]]:
; CHECK:         [[TMP3:%.*]] = invoke i64* @load_dec(i64** [[TMP1]])
; CHECK-NEXT:                      to label %[[MERGE]] unwind label %[[EXCEPT:.*]]
; CHECK:       [[MERGE]]:
; CHECK-NEXT:    [[TMP4:%.*]] = phi i64* [ [[TMP2]], %[[THEN]] ], [ [[TMP3]], %[[ELSE]] ]
; CHECK-NEXT:    [[TMP5:%.*]] = bitcast i64* [[TMP4]] to i32*
; CHECK-NEXT:    br label %[[NORMAL]]
; CHECK:       [[NORMAL]]:
; CHECK-NEXT:    [[TMP6:%.*]] = phi i32* [ [[TMP5]], %[[MERGE]] ], [ null, %entry ]
; CHECK-NEXT:    ret i32* [[TMP6]]
; CHECK:       [[EXCEPT]]:
; CHECK-NEXT:    [[TMP7:%.*]] = landingpad { i8*, i32 }
; CHECK-NEXT:                   cleanup
; CHECK-NEXT:    ret i32* null
;
define i32* @invoke.2.1.bitcast(i32** %x, i32* (i32**)* %op) personality i32 (...)* null {
entry:
  br i1 undef, label %try, label %normal
try:
  %tmp0 = invoke i32* %op(i32 **%x)
             to label %normal unwind label %except,
                !callees !{i64* (i64**)* @load_def, i64* (i64**)* @load_dec}
normal:
  %tmp1 = phi i32* [ %tmp0, %try ], [ null, %entry ]
  ret i32* %tmp1
except:
  %tmp2 = landingpad {i8*, i32}
          cleanup
  ret i32* null
}

; The functions declarations and definitions below are used in the above tests
; and are not interesting. The !callees metadata that references the functions
; is expanded and attached to the call sites.
;
declare i64 @select_dec_0(i64 %x, i64 %y)
declare i64 @select_dec_1(i64 %x, i64 %y)
declare i64* @load_dec(i64** %x)

define i64 @select_def_0(i64 %x, i64 %y) { ret i64 %x }
define i64 @select_def_1(i64 %x, i64 %y) { ret i64 %y }
define i64* @load_def(i64** %x) {
  %tmp0 = load i64*, i64** %x
  ret i64* %tmp0
}
