; RUN: opt < %s -instcombine -S | FileCheck %s
; CHECK-LABEL: @foo
; CHECK: %2 = call i8* @malloc(i64 808)
; CHECK-NEXT: %.cast = bitcast i8* %2 to %struct.ABC* 
; CHECK-NEXT: call void @Setup(%struct.ABC* %.cast)
; CHECK-NEXT: %.cast1 = bitcast i8* %2 to %struct.ABC*
; CHECK-NEXT: %3 = getelementptr inbounds %struct.ABC, %struct.ABC* %.cast1, i64 0, i32 2, i32 0
; CHECK-NEXT: %4 = load i32, i32* %3, align 4
; CHECK-NEXT: %.cast2 = bitcast i8* %2 to %struct.ABC*
; CHECK-NEXT: %5 = getelementptr inbounds %struct.ABC, %struct.ABC* %.cast2, i64 0, i32 2, i32 1, i64 33

%struct.ABC = type { i32, [100 x i32], %struct.XYZ }
%struct.XYZ = type { i32, [100 x i32] }

; Function Attrs: noinline nounwind optnone uwtable
define i32 @foo(i32)  {
  %2 = alloca i32, align 4
  %3 = alloca %struct.ABC*, align 8
  store i32 %0, i32* %2, align 4
  %4 = call i8* @malloc(i64 808)
  %5 = bitcast i8* %4 to %struct.ABC*
  store %struct.ABC* %5, %struct.ABC** %3, align 8
  %6 = load %struct.ABC*, %struct.ABC** %3, align 8
  call void @Setup(%struct.ABC* %6)
  %7 = load %struct.ABC*, %struct.ABC** %3, align 8
  %8 = getelementptr inbounds %struct.ABC, %struct.ABC* %7, i32 0, i32 2
  %9 = getelementptr inbounds %struct.XYZ, %struct.XYZ* %8, i32 0, i32 0
  %10 = load i32, i32* %9, align 4
  %11 = load %struct.ABC*, %struct.ABC** %3, align 8
  %12 = getelementptr inbounds %struct.ABC, %struct.ABC* %11, i32 0, i32 2
  %13 = getelementptr inbounds %struct.XYZ, %struct.XYZ* %12, i32 0, i32 1
  %14 = getelementptr inbounds [100 x i32], [100 x i32]* %13, i64 0, i64 33
  %15 = load i32, i32* %14, align 4
  %16 = add nsw i32 %10, %15
  ret i32 %16
}

declare i8* @malloc(i64) 

declare void @Setup(%struct.ABC*) 

