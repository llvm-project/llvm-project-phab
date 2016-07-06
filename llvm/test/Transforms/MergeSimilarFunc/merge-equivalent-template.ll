; RUN: opt -S -mergesimilarfunc %s -o - | FileCheck %s

; CHECK:      define linkonce_odr void @_ZN11FooTemplateIPvE7method1EP9FooStruct(%class.FooTemplate.0* nocapture readnone %this, %struct.FooStruct* %arg0) #1 align 2 {
; CHECK-NEXT: entry:
; CHECK-NEXT:   %field27 = getelementptr inbounds %struct.FooStruct, %struct.FooStruct* %arg0, i32 0, i32 2
; CHECK-NEXT:   %0 = load i8, i8* %field27, align 1
; CHECK-NEXT:   %lnot8 = icmp eq i8 %0, 0
; CHECK-NEXT:   br i1 %lnot8, label %for.body, label %for.end

; CHECK:      for.body:                                         ; preds = %for.body, %entry
; CHECK-NEXT:   %arg0.addr.09 = phi %struct.FooStruct* [ %2, %for.body ], [ %arg0, %entry ]
; CHECK-NEXT:   %field1 = getelementptr inbounds %struct.FooStruct, %struct.FooStruct* %arg0.addr.09, i32 0, i32 1
; CHECK-NEXT:   %1 = load %struct.FooStruct*, %struct.FooStruct** %field1, align 4
; CHECK-NEXT:   tail call void @_ZN11FooTemplateIPvE7method1EP9FooStruct(%class.FooTemplate.0* %this, %struct.FooStruct* %1) #2
; CHECK-NEXT:   %field0 = getelementptr inbounds %struct.FooStruct, %struct.FooStruct* %arg0.addr.09, i32 0, i32 0
; CHECK-NEXT:   %2 = load %struct.FooStruct*, %struct.FooStruct** %field0, align 4
; CHECK-NEXT:   %3 = bitcast %struct.FooStruct* %arg0.addr.09 to i8*
; CHECK-NEXT:   tail call void @_Z4bar0Pv(i8* %3) #3
; CHECK-NEXT:   tail call void @_Z4bar1Pv(i8* %3) #3
; CHECK-NEXT:   %field2 = getelementptr inbounds %struct.FooStruct, %struct.FooStruct* %2, i32 0, i32 2
; CHECK-NEXT:   %4 = load i8, i8* %field2, align 1
; CHECK-NEXT:   %lnot = icmp eq i8 %4, 0
; CHECK-NEXT:   br i1 %lnot, label %for.body, label %for.end

; CHECK:      for.end:                                          ; preds = %for.body, %entry
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

; CHECK:      define linkonce_odr void @_ZN11FooTemplateIiE7method1EP9FooStruct(%class.FooTemplate* nocapture readnone, %struct.FooStruct*) #1 align 2 {
; CHECK-NEXT:   %3 = bitcast %class.FooTemplate* %0 to %class.FooTemplate.0*
; CHECK-NEXT:   tail call void @_ZN11FooTemplateIPvE7method1EP9FooStruct(%class.FooTemplate.0* nocapture readnone %3, %struct.FooStruct* %1)
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

; CHECK:      attributes #0 = { optsize "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
; CHECK-NEXT: attributes #1 = { nounwind optsize ssp "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
; CHECK-NEXT: attributes #2 = { optsize }
; CHECK-NEXT: attributes #3 = { nounwind optsize }

target datalayout = "e-m:e-p:32:32-i1:32-i64:64-a:0-v32:32-n16:32"

%class.Foo = type { %class.FooTemplate, %class.FooTemplate.0 }
%class.FooTemplate = type { i8 }
%class.FooTemplate.0 = type { i8 }
%struct.FooStruct = type { %struct.FooStruct*, %struct.FooStruct*, i8 }

@_ZN3FooD1Ev = alias void(%class.Foo*), void (%class.Foo*)* @_ZN3FooD2Ev

declare %struct.FooStruct* @_ZNK11FooTemplateIPvE7method2Ev(%class.FooTemplate.0*) #1
declare void @_Z4bar0Pv(i8*) #1
declare void @_Z4bar1Pv(i8*) #1
declare %struct.FooStruct* @_ZNK11FooTemplateIiE7method2Ev(%class.FooTemplate*) #1

; Function Attrs: nounwind optsize ssp
define void @_ZN3FooD2Ev(%class.Foo* %this) unnamed_addr #0 align 2 {
entry:
  %bar_things = getelementptr inbounds %class.Foo, %class.Foo* %this, i32 0, i32 0
  tail call void @_ZN11FooTemplateIiE7method0Ev(%class.FooTemplate* %bar_things) #2
  %baz_things = getelementptr inbounds %class.Foo, %class.Foo* %this, i32 0, i32 1
  tail call void @_ZN11FooTemplateIPvE7method0Ev(%class.FooTemplate.0* %baz_things) #2
  ret void
}

; Function Attrs: nounwind optsize ssp
define linkonce_odr void @_ZN11FooTemplateIiE7method0Ev(%class.FooTemplate* %this) #0 align 2 {
entry:
  %call = tail call %struct.FooStruct* @_ZNK11FooTemplateIiE7method2Ev(%class.FooTemplate* %this) #3
  tail call void @_ZN11FooTemplateIiE7method1EP9FooStruct(%class.FooTemplate* %this, %struct.FooStruct* %call) #2
  ret void
}

; Function Attrs: nounwind optsize ssp
define linkonce_odr void @_ZN11FooTemplateIPvE7method0Ev(%class.FooTemplate.0* %this) #0 align 2 {
entry:
  %call = tail call %struct.FooStruct* @_ZNK11FooTemplateIPvE7method2Ev(%class.FooTemplate.0* %this) #3
  tail call void @_ZN11FooTemplateIPvE7method1EP9FooStruct(%class.FooTemplate.0* %this, %struct.FooStruct* %call) #2
  ret void
}

; Function Attrs: nounwind optsize ssp
define linkonce_odr void @_ZN11FooTemplateIPvE7method1EP9FooStruct(%class.FooTemplate.0* nocapture readnone %this, %struct.FooStruct* %arg0) #0 align 2 {
entry:
  %field27 = getelementptr inbounds %struct.FooStruct, %struct.FooStruct* %arg0, i32 0, i32 2
  %0 = load i8, i8* %field27, align 1
  %lnot8 = icmp eq i8 %0, 0
  br i1 %lnot8, label %for.body, label %for.end

for.body:                                         ; preds = %entry, %for.body
  %arg0.addr.09 = phi %struct.FooStruct* [ %2, %for.body ], [ %arg0, %entry ]
  %field1 = getelementptr inbounds %struct.FooStruct, %struct.FooStruct* %arg0.addr.09, i32 0, i32 1
  %1 = load %struct.FooStruct*, %struct.FooStruct** %field1, align 4
  tail call void @_ZN11FooTemplateIPvE7method1EP9FooStruct(%class.FooTemplate.0* %this, %struct.FooStruct* %1) #2
  %field0 = getelementptr inbounds %struct.FooStruct, %struct.FooStruct* %arg0.addr.09, i32 0, i32 0
  %2 = load %struct.FooStruct*, %struct.FooStruct** %field0, align 4
  %3 = bitcast %struct.FooStruct* %arg0.addr.09 to i8*
  tail call void @_Z4bar0Pv(i8* %3) #3
  tail call void @_Z4bar1Pv(i8* %3) #3
  %field2 = getelementptr inbounds %struct.FooStruct, %struct.FooStruct* %2, i32 0, i32 2
  %4 = load i8, i8* %field2, align 1
  %lnot = icmp eq i8 %4, 0
  br i1 %lnot, label %for.body, label %for.end

for.end:                                          ; preds = %for.body, %entry
  ret void
}

; Function Attrs: nounwind optsize ssp
define linkonce_odr void @_ZN11FooTemplateIiE7method1EP9FooStruct(%class.FooTemplate* nocapture readnone %this, %struct.FooStruct* %arg0) #0 align 2 {
entry:
  %field27 = getelementptr inbounds %struct.FooStruct, %struct.FooStruct* %arg0, i32 0, i32 2
  %0 = load i8, i8* %field27, align 1
  %lnot8 = icmp eq i8 %0, 0
  br i1 %lnot8, label %for.body, label %for.end

for.body:                                         ; preds = %entry, %for.body
  %arg0.addr.09 = phi %struct.FooStruct* [ %2, %for.body ], [ %arg0, %entry ]
  %field1 = getelementptr inbounds %struct.FooStruct, %struct.FooStruct* %arg0.addr.09, i32 0, i32 1
  %1 = load %struct.FooStruct*, %struct.FooStruct** %field1, align 4
  tail call void @_ZN11FooTemplateIiE7method1EP9FooStruct(%class.FooTemplate* %this, %struct.FooStruct* %1) #2
  %field0 = getelementptr inbounds %struct.FooStruct, %struct.FooStruct* %arg0.addr.09, i32 0, i32 0
  %2 = load %struct.FooStruct*, %struct.FooStruct** %field0, align 4
  %3 = bitcast %struct.FooStruct* %arg0.addr.09 to i8*
  tail call void @_Z4bar0Pv(i8* %3) #3
  tail call void @_Z4bar1Pv(i8* %3) #3
  %field2 = getelementptr inbounds %struct.FooStruct, %struct.FooStruct* %2, i32 0, i32 2
  %4 = load i8, i8* %field2, align 1
  %lnot = icmp eq i8 %4, 0
  br i1 %lnot, label %for.body, label %for.end

for.end:                                          ; preds = %for.body, %entry
  ret void
}

attributes #0 = { nounwind optsize ssp "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { optsize "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { optsize }
attributes #3 = { nounwind optsize }

