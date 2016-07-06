; RUN: opt -mergesimilarfunc -S %s -o - | FileCheck %s
;
; CHECK:      define i8* @foo_a(%struct.a_type* %arg0) #1 {
; CHECK-NEXT: entry:
; CHECK-NEXT:   br label %for.cond

; CHECK:      for.cond:                                         ; preds = %for.inc24, %entry
; CHECK-NEXT:   %i.0 = phi i8 [ 0, %entry ], [ %inc25, %for.inc24 ]
; CHECK-NEXT:   %ptr0.0 = phi i8* [ null, %entry ], [ %ptr0.3, %for.inc24 ]
; CHECK-NEXT:   %conv = zext i8 %i.0 to i32
; CHECK-NEXT:   %cmp = icmp slt i32 %conv, 16
; CHECK-NEXT:   br i1 %cmp, label %for.body, label %for.end26

; CHECK:      for.body:                                         ; preds = %for.cond
; CHECK-NEXT:   %call = call i8* @bar4(i32 %conv) #2
; CHECK-NEXT:   %call4 = call i8* @bar0(i32 %conv) #2
; CHECK-NEXT:   %0 = bitcast i8* %call4 to %struct.a_type*
; CHECK-NEXT:   %call5 = call i32 @bar1(i8* %call) #2
; CHECK-NEXT:   %tobool = icmp ne i32 %call5, 0
; CHECK-NEXT:   br i1 %tobool, label %for.cond6, label %for.inc24

; CHECK:      for.cond6:                                        ; preds = %for.inc, %for.body
; CHECK-NEXT:   %k.0 = phi i8 [ %inc, %for.inc ], [ 0, %for.body ]
; CHECK-NEXT:   %ptr0.1 = phi i8* [ %ptr0.2, %for.inc ], [ %call, %for.body ]
; CHECK-NEXT:   %idxprom = zext i8 %k.0 to i32
; CHECK-NEXT:   %field4 = getelementptr inbounds %struct.a_type, %struct.a_type* %arg0, i32 0, i32 4
; CHECK-NEXT:   %arrayidx = getelementptr inbounds [2 x i8*], [2 x i8*]* %field4, i32 0, i32 %idxprom
; CHECK-NEXT:   %1 = load i8*, i8** %arrayidx, align 4
; CHECK-NEXT:   %cmp7 = icmp ne i8* %1, null
; CHECK-NEXT:   br i1 %cmp7, label %land.rhs, label %for.inc24

; CHECK:      land.rhs:                                         ; preds = %for.cond6
; CHECK-NEXT:   %field410 = getelementptr inbounds %struct.a_type, %struct.a_type* %0, i32 0, i32 4
; CHECK-NEXT:   %arrayidx11 = getelementptr inbounds [2 x i8*], [2 x i8*]* %field410, i32 0, i32 %idxprom
; CHECK-NEXT:   %2 = load i8*, i8** %arrayidx11, align 4
; CHECK-NEXT:   %cmp12 = icmp ne i8* %2, null
; CHECK-NEXT:   br i1 %cmp12, label %for.body14, label %for.inc24

; CHECK:      for.body14:                                       ; preds = %land.rhs
; CHECK-NEXT:   %3 = bitcast %struct.a_type* %0 to i8*
; CHECK-NEXT:   %4 = bitcast %struct.a_type* %arg0 to i8*
; CHECK-NEXT:   %call15 = call i32 @bar2(i8* %3, i8* %4) #2
; CHECK-NEXT:   %tobool16 = icmp ne i32 %call15, 0
; CHECK-NEXT:   br i1 %tobool16, label %if.then17, label %for.inc

; CHECK:      if.then17:                                        ; preds = %for.body14
; CHECK-NEXT:   %call18 = call i32 @bar3(i8* %3, i8* %4) #2
; CHECK-NEXT:   %tobool19 = icmp ne i32 %call18, 0
; CHECK-NEXT:   br i1 %tobool19, label %if.then20, label %for.inc

; CHECK:      if.then20:                                        ; preds = %if.then17
; CHECK-NEXT:   br label %for.inc

; CHECK:      for.inc:                                          ; preds = %if.then20, %if.then17, %for.body14
; CHECK-NEXT:   %ptr0.2 = phi i8* [ null, %if.then20 ], [ %ptr0.1, %if.then17 ], [ null, %for.body14 ]
; CHECK-NEXT:   %inc = add i8 %k.0, 1
; CHECK-NEXT:   br label %for.cond6

; CHECK:      for.inc24:                                        ; preds = %land.rhs, %for.cond6, %for.body
; CHECK-NEXT:   %ptr0.3 = phi i8* [ %ptr0.1, %land.rhs ], [ %ptr0.1, %for.cond6 ], [ null, %for.body ]
; CHECK-NEXT:   %inc25 = add i8 %i.0, 1
; CHECK-NEXT:   br label %for.cond

; CHECK:      for.end26:                                        ; preds = %for.cond
; CHECK-NEXT:   ret i8* %ptr0.0
; CHECK-NEXT: }

; CHECK:      ; Function Attrs: nounwind optsize ssp
; CHECK-NEXT: define i8* @foo_b(%struct.b_type*) #1 {
; CHECK-NEXT:   %2 = bitcast %struct.b_type* %0 to %struct.a_type*
; CHECK-NEXT:   %3 = tail call i8* @foo_a(%struct.a_type* %2)
; CHECK-NEXT:   ret i8* %3
; CHECK-NEXT: }

; CHECK:      attributes #0 = { optsize "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
; CHECK-NEXT: attributes #1 = { nounwind optsize ssp "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
; CHECK-NEXT: attributes #2 = { optsize }

target datalayout = "e-m:e-p:32:32-i1:32-i64:64-a:0-v32:32-n16:32"

%struct.a_type = type { i32, i32, i32, i8*, [2 x i8*] }
%struct.b_type = type { i32, i32, i32, i8*, [2 x i8*], i32 }

; Function Attrs: optsize
declare i8* @bar4(i32) #1
declare i8* @bar0(i32) #1
declare i32 @bar1(i8*) #1
declare i32 @bar2(i8*, i8*) #1
declare i32 @bar3(i8*, i8*) #1

; Function Attrs: nounwind optsize ssp
define i8* @foo_a(%struct.a_type* %arg0) #0 {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc24, %entry
  %i.0 = phi i8 [ 0, %entry ], [ %inc25, %for.inc24 ]
  %ptr0.0 = phi i8* [ null, %entry ], [ %ptr0.3, %for.inc24 ]
  %conv = zext i8 %i.0 to i32
  %cmp = icmp slt i32 %conv, 16
  br i1 %cmp, label %for.body, label %for.end26

for.body:                                         ; preds = %for.cond
  %call = call i8* @bar4(i32 %conv) #2
  %call4 = call i8* @bar0(i32 %conv) #2
  %0 = bitcast i8* %call4 to %struct.a_type*
  %call5 = call i32 @bar1(i8* %call) #2
  %tobool = icmp ne i32 %call5, 0
  br i1 %tobool, label %for.cond6, label %for.inc24

for.cond6:                                        ; preds = %for.body, %for.inc
  %k.0 = phi i8 [ %inc, %for.inc ], [ 0, %for.body ]
  %ptr0.1 = phi i8* [ %ptr0.2, %for.inc ], [ %call, %for.body ]
  %idxprom = zext i8 %k.0 to i32
  %field4 = getelementptr inbounds %struct.a_type, %struct.a_type* %arg0, i32 0, i32 4
  %arrayidx = getelementptr inbounds [2 x i8*], [2 x i8*]* %field4, i32 0, i32 %idxprom
  %1 = load i8*, i8** %arrayidx, align 4
  %cmp7 = icmp ne i8* %1, null
  br i1 %cmp7, label %land.rhs, label %for.inc24

land.rhs:                                         ; preds = %for.cond6
  %field410 = getelementptr inbounds %struct.a_type, %struct.a_type* %0, i32 0, i32 4
  %arrayidx11 = getelementptr inbounds [2 x i8*], [2 x i8*]* %field410, i32 0, i32 %idxprom
  %2 = load i8*, i8** %arrayidx11, align 4
  %cmp12 = icmp ne i8* %2, null
  br i1 %cmp12, label %for.body14, label %for.inc24

for.body14:                                       ; preds = %land.rhs
  %3 = bitcast %struct.a_type* %0 to i8*
  %4 = bitcast %struct.a_type* %arg0 to i8*
  %call15 = call i32 @bar2(i8* %3, i8* %4) #2
  %tobool16 = icmp ne i32 %call15, 0
  br i1 %tobool16, label %if.then17, label %for.inc

if.then17:                                        ; preds = %for.body14
  %call18 = call i32 @bar3(i8* %3, i8* %4) #2
  %tobool19 = icmp ne i32 %call18, 0
  br i1 %tobool19, label %if.then20, label %for.inc

if.then20:                                        ; preds = %if.then17
  br label %for.inc

for.inc:                                          ; preds = %for.body14, %if.then17, %if.then20
  %ptr0.2 = phi i8* [ null, %if.then20 ], [ %ptr0.1, %if.then17 ], [ null, %for.body14 ]
  %inc = add i8 %k.0, 1
  br label %for.cond6

for.inc24:                                        ; preds = %for.body, %for.cond6, %land.rhs
  %ptr0.3 = phi i8* [ %ptr0.1, %land.rhs ], [ %ptr0.1, %for.cond6 ], [ null, %for.body ]
  %inc25 = add i8 %i.0, 1
  br label %for.cond

for.end26:                                        ; preds = %for.cond
  ret i8* %ptr0.0
}

; Function Attrs: nounwind optsize ssp
define i8* @foo_b(%struct.b_type* %arg0) #0 {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc24, %entry
  %i.0 = phi i8 [ 0, %entry ], [ %inc25, %for.inc24 ]
  %ptr0.0 = phi i8* [ null, %entry ], [ %ptr0.3, %for.inc24 ]
  %conv = zext i8 %i.0 to i32
  %cmp = icmp slt i32 %conv, 16
  br i1 %cmp, label %for.body, label %for.end26

for.body:                                         ; preds = %for.cond
  %call = call i8* @bar4(i32 %conv) #2
  %call4 = call i8* @bar0(i32 %conv) #2
  %0 = bitcast i8* %call4 to %struct.b_type*
  %call5 = call i32 @bar1(i8* %call) #2
  %tobool = icmp ne i32 %call5, 0
  br i1 %tobool, label %for.cond6, label %for.inc24

for.cond6:                                        ; preds = %for.body, %for.inc
  %k.0 = phi i8 [ %inc, %for.inc ], [ 0, %for.body ]
  %ptr0.1 = phi i8* [ %ptr0.2, %for.inc ], [ %call, %for.body ]
  %idxprom = zext i8 %k.0 to i32
  %field4 = getelementptr inbounds %struct.b_type, %struct.b_type* %arg0, i32 0, i32 4
  %arrayidx = getelementptr inbounds [2 x i8*], [2 x i8*]* %field4, i32 0, i32 %idxprom
  %1 = load i8*, i8** %arrayidx, align 4
  %cmp7 = icmp ne i8* %1, null
  br i1 %cmp7, label %land.rhs, label %for.inc24

land.rhs:                                         ; preds = %for.cond6
  %field410 = getelementptr inbounds %struct.b_type, %struct.b_type* %0, i32 0, i32 4
  %arrayidx11 = getelementptr inbounds [2 x i8*], [2 x i8*]* %field410, i32 0, i32 %idxprom
  %2 = load i8*, i8** %arrayidx11, align 4
  %cmp12 = icmp ne i8* %2, null
  br i1 %cmp12, label %for.body14, label %for.inc24

for.body14:                                       ; preds = %land.rhs
  %3 = bitcast %struct.b_type* %0 to i8*
  %4 = bitcast %struct.b_type* %arg0 to i8*
  %call15 = call i32 @bar2(i8* %3, i8* %4) #2
  %tobool16 = icmp ne i32 %call15, 0
  br i1 %tobool16, label %if.then17, label %for.inc

if.then17:                                        ; preds = %for.body14
  %call18 = call i32 @bar3(i8* %3, i8* %4) #2
  %tobool19 = icmp ne i32 %call18, 0
  br i1 %tobool19, label %if.then20, label %for.inc

if.then20:                                        ; preds = %if.then17
  br label %for.inc

for.inc:                                          ; preds = %for.body14, %if.then17, %if.then20
  %ptr0.2 = phi i8* [ null, %if.then20 ], [ %ptr0.1, %if.then17 ], [ null, %for.body14 ]
  %inc = add i8 %k.0, 1
  br label %for.cond6

for.inc24:                                        ; preds = %for.body, %for.cond6, %land.rhs
  %ptr0.3 = phi i8* [ %ptr0.1, %land.rhs ], [ %ptr0.1, %for.cond6 ], [ null, %for.body ]
  %inc25 = add i8 %i.0, 1
  br label %for.cond

for.end26:                                        ; preds = %for.cond
  ret i8* %ptr0.0
}

attributes #0 = { nounwind optsize ssp "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { optsize "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { optsize }

