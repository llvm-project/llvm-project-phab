; RUN: opt < %s -mtriple=x86_64-unknown-linux-gnu -memaccessshrink -S | FileCheck %s
; Check the memory accesses in the test are shrinked as expected.

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

%class.B = type { i56, [4 x i16], i64 }
@i = local_unnamed_addr global i64 0, align 8

define zeroext i1 @foo(%class.B* nocapture %this, i8* %p1, i64 %p2) local_unnamed_addr align 2 {
; CHECK-LABEL: @foo(
entry:
; CHECK: entry:
; CHECK: %t0 = bitcast %class.B* %this to i64*
; CHECK: %cast36 = bitcast i64* %t0 to i16*
; CHECK: %load.trunc37 = load i16, i16* %cast36, align 8
; CHECK: %trunc.zext38 = zext i16 %load.trunc37 to i64
; CHECK: %cmp = icmp eq i64 %trunc.zext38, 1

  %t0 = bitcast %class.B* %this to i64*
  %bf.load = load i64, i64* %t0, align 8
  %bf.clear = and i64 %bf.load, 65535
  %cmp = icmp eq i64 %bf.clear, 1
  br i1 %cmp, label %return, label %if.end

if.end:                                           ; preds = %entry
; CHECK: if.end:
; CHECK: %cast35 = bitcast i64* %t0 to i16*
; CHECK: %load.trunc = load i16, i16* %cast35, align 8
; CHECK: %[[ADD1:.*]] = add i16 %load.trunc, -1
; CHECK: %trunc.zext = zext i16 %[[ADD1]] to i64
; CHECK: %cast33 = bitcast i64* %t0 to i16*
; CHECK: %trunc34 = trunc i64 %trunc.zext to i16
; CHECK: store i16 %trunc34, i16* %cast33, align 8

  %dec = add i64 %bf.load, 65535
  %bf.value = and i64 %dec, 65535
  %bf.clear5 = and i64 %bf.load, -65536
  %bf.set = or i64 %bf.value, %bf.clear5
  store i64 %bf.set, i64* %t0, align 8
  %t1 = ptrtoint i8* %p1 to i64
  %t2 = load i64, i64* @i, align 8
  %cmp.i = icmp ult i64 %t2, 3
  br i1 %cmp.i, label %if.then.i, label %if.else.i

if.then.i:                                        ; preds = %if.end
  %and.i = lshr i64 %t1, 3
  %div.i = and i64 %and.i, 1023
  br label %_ZNK1B5m_fn3EPv.exit

if.else.i:                                        ; preds = %if.end
  %first_page_.i = getelementptr inbounds %class.B, %class.B* %this, i64 0, i32 2
  %t3 = load i64, i64* %first_page_.i, align 8
  %mul.i = shl i64 %t3, 13
  %sub.i = sub i64 %t1, %mul.i
  %div2.i = lshr i64 %sub.i, 2
  br label %_ZNK1B5m_fn3EPv.exit

_ZNK1B5m_fn3EPv.exit:                             ; preds = %if.then.i, %if.else.i
; CHECK: _ZNK1B5m_fn3EPv.exit:
; CHECK: %cast76 = bitcast i64* %t0 to i8*
; CHECK: %uglygep77 = getelementptr i8, i8* %cast76, i32 6
; CHECK: %load.trunc78 = load i8, i8* %uglygep77, align 2
; CHECK: %trunc.zext79 = zext i8 %load.trunc78 to i64
; CHECK: %cmp8 = icmp eq i64 %trunc.zext79, 4

  %j.0.i = phi i64 [ %div.i, %if.then.i ], [ %div2.i, %if.else.i ]
  %conv.i = trunc i64 %j.0.i to i16
  %bf.lshr = lshr i64 %bf.load, 48
  %bf.clear7 = and i64 %bf.lshr, 255
  %cmp8 = icmp eq i64 %bf.clear7, 4
  br i1 %cmp8, label %if.else, label %if.then9

if.then9:                                         ; preds = %_ZNK1B5m_fn3EPv.exit
; CHECK: if.then9:
; CHECK: %arrayidx = getelementptr inbounds %class.B, %class.B* %this, i64 0, i32 1, i64 %trunc.zext79
; CHECK: store i16 %conv.i, i16* %arrayidx, align 2
; CHECK: %cast25 = bitcast i64* %t0 to i8*
; CHECK: %uglygep26 = getelementptr i8, i8* %cast25, i32 6
; CHECK: %cast71 = bitcast i64* %t0 to i8*
; CHECK: %uglygep72 = getelementptr i8, i8* %cast71, i32 6
; CHECK: %load.trunc73 = load i8, i8* %uglygep72, align 2
; CHECK: %[[ADD2:.*]] = add i8 %load.trunc73, 1
; CHECK: %[[AND1:.*]] = and i8 %[[ADD2]], -1
; CHECK: store i8 %[[AND1]], i8* %uglygep26, align 2

  %arrayidx = getelementptr inbounds %class.B, %class.B* %this, i64 0, i32 1, i64 %bf.clear7
  store i16 %conv.i, i16* %arrayidx, align 2
  %inc79 = add i64 %bf.set, 281474976710656
  %bf.shl = and i64 %inc79, 71776119061217280
  %bf.clear18 = and i64 %bf.set, -71776119061217281
  %bf.set19 = or i64 %bf.shl, %bf.clear18
  store i64 %bf.set19, i64* %t0, align 8
  br label %return

if.else:                                          ; preds = %_ZNK1B5m_fn3EPv.exit
; CHECK: if.else:
; CHECK: %cast66 = bitcast i64* %t0 to i8*
; CHECK: %uglygep67 = getelementptr i8, i8* %cast66, i32 4
; CHECK: %cast68 = bitcast i8* %uglygep67 to i16*
; CHECK: %load.trunc69 = load i16, i16* %cast68, align 4
; CHECK: %trunc.zext70 = zext i16 %load.trunc69 to i64
; CHECK: %cmp24 = icmp eq i64 %trunc.zext70, 1

  %bf.lshr21 = lshr i64 %bf.load, 32
  %bf.clear22 = and i64 %bf.lshr21, 65535
  %cmp24 = icmp eq i64 %bf.clear22, 1
  br i1 %cmp24, label %if.else55, label %land.lhs.true

land.lhs.true:                                    ; preds = %if.else
; CHECK: land.lhs.true:
; CHECK: %cast61 = bitcast i64* %t0 to i8*
; CHECK: %uglygep62 = getelementptr i8, i8* %cast61, i32 2
; CHECK: %cast63 = bitcast i8* %uglygep62 to i16*
; CHECK: %load.trunc64 = load i16, i16* %cast63, align 2
; CHECK: %trunc.zext65 = zext i16 %load.trunc64 to i64
; CHECK: %cmp28 = icmp eq i64 %trunc.zext65, %sub

  %bf.lshr26 = lshr i64 %bf.load, 16
  %bf.clear27 = and i64 %bf.lshr26, 65535
  %div = lshr i64 %p2, 1
  %sub = add nsw i64 %div, -1
  %cmp28 = icmp eq i64 %bf.clear27, %sub
  br i1 %cmp28, label %if.else55, label %if.then29

if.then29:                                        ; preds = %land.lhs.true
  %cmp30 = icmp ult i64 %p2, 3
  br i1 %cmp30, label %if.then31, label %if.else35

if.then31:                                        ; preds = %if.then29
; CHECK: if.then31:
; CHECK: %mul = shl nuw nsw i64 %trunc.zext70, 3

  %and = and i64 %t1, -8192
  %mul = shl nuw nsw i64 %bf.clear22, 3
  %add = add i64 %mul, %and
  br label %if.end41

if.else35:                                        ; preds = %if.then29
; CHECK: if.else35:
; CHECK: %cast56 = bitcast i64* %t0 to i8*
; CHECK: %uglygep57 = getelementptr i8, i8* %cast56, i32 4
; CHECK: %cast58 = bitcast i8* %uglygep57 to i16*
; CHECK: %load.trunc59 = load i16, i16* %cast58, align 4
; CHECK: %trunc.zext60 = zext i16 %load.trunc59 to i64
; CHECK: %shl = shl i64 %trunc.zext60, 6

  %first_page_.i80 = getelementptr inbounds %class.B, %class.B* %this, i64 0, i32 2
  %t4 = load i64, i64* %first_page_.i80, align 8
  %mul.i81 = shl i64 %t4, 13
  %conv.i82 = shl nuw nsw i64 %bf.lshr21, 6
  %mul3.i = and i64 %conv.i82, 4194240
  %add.i = add i64 %mul.i81, %mul3.i
  br label %if.end41

if.end41:                                         ; preds = %if.else35, %if.then31
; CHECK: if.end41:
; CHECK: %cast8 = bitcast i64* %t0 to i8*
; CHECK: %uglygep9 = getelementptr i8, i8* %cast8, i32 2
; CHECK: %cast10 = bitcast i8* %uglygep9 to i16*
; CHECK: %cast84 = bitcast i64* %t0 to i8*
; CHECK: %uglygep85 = getelementptr i8, i8* %cast84, i32 2
; CHECK: %cast86 = bitcast i8* %uglygep85 to i16*
; CHECK: %load.trunc87 = load i16, i16* %cast86, align 2
; CHECK: %[[ADD3:.*]] = add i16 %load.trunc87, 1
; CHECK: %[[AND2:.*]] = and i16 %[[ADD3]], -1
; CHECK: %cast45 = bitcast i64* %t0 to i8*
; CHECK: %uglygep46 = getelementptr i8, i8* %cast45, i32 2
; CHECK: %cast47 = bitcast i8* %uglygep46 to i16*
; CHECK: %load.trunc48 = load i16, i16* %cast47, align 2
; CHECK: %[[ADD4:.*]] = add i16 %load.trunc48, 1
; CHECK: %trunc.zext49 = zext i16 %[[ADD4]] to i64
; CHECK: store i16 %[[AND2]], i16* %cast10, align 2
; CHECK: %arrayidx54 = getelementptr inbounds i16, i16* %t5, i64 %trunc.zext49

  %add.i.sink = phi i64 [ %add.i, %if.else35 ], [ %add, %if.then31 ]
  %t5 = inttoptr i64 %add.i.sink to i16*
  %inc4578 = add i64 %bf.set, 65536
  %bf.shl48 = and i64 %inc4578, 4294901760
  %bf.clear49 = and i64 %bf.set, -4294901761
  %bf.set50 = or i64 %bf.shl48, %bf.clear49
  store i64 %bf.set50, i64* %t0, align 8
  %bf.lshr52 = lshr i64 %inc4578, 16
  %bf.clear53 = and i64 %bf.lshr52, 65535
  %arrayidx54 = getelementptr inbounds i16, i16* %t5, i64 %bf.clear53
  store i16 %conv.i, i16* %arrayidx54, align 2
  br label %return

if.else55:                                        ; preds = %land.lhs.true, %if.else
; CHECK: if.else55:
; CHECK: %cast80 = bitcast i64* %t0 to i8*
; CHECK: %uglygep81 = getelementptr i8, i8* %cast80, i32 4
; CHECK: %cast82 = bitcast i8* %uglygep81 to i16*
; CHECK: %load.trunc83 = load i16, i16* %cast82, align 4
; CHECK: store i16 %load.trunc83, i16* %t6, align 2
; CHECK: %uglygep = getelementptr i8, i8* %cast, i32 2
; CHECK: %cast1 = bitcast i8* %uglygep to i32*
; CHECK: %lshr = lshr i64 %bf.shl63, 16
; CHECK: %trunc = trunc i64 %lshr to i32
; CHECK: store i32 %trunc, i32* %cast1, align 2

  %conv59 = trunc i64 %bf.lshr21 to i16
  %t6 = bitcast i8* %p1 to i16*
  store i16 %conv59, i16* %t6, align 2
  %bf.load61 = load i64, i64* %t0, align 8
  %conv60 = shl i64 %j.0.i, 32
  %bf.shl63 = and i64 %conv60, 281470681743360
  %bf.clear64 = and i64 %bf.load61, -281474976645121
  %bf.clear67 = or i64 %bf.clear64, %bf.shl63
  store i64 %bf.clear67, i64* %t0, align 8
  br label %return

return:                                           ; preds = %if.then9, %if.else55, %if.end41, %entry
  %retval.0 = phi i1 [ false, %entry ], [ true, %if.end41 ], [ true, %if.else55 ], [ true, %if.then9 ]
  ret i1 %retval.0
}

