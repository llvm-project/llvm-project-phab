; CHECK the test won't timeout (will happen when -scalar-evolution-max-const-expr-size= is set to 
; large value
; RUN: opt < %s -indvars 
%struct.ST = type { %struct.ST* }

@global = internal global [121 x i8] zeroinitializer, align 1

define void @func() {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.body, %entry
  %0 = phi %struct.ST* [ %2, %for.body ], [ bitcast ([121 x i8]* @global to %struct.ST*), %entry ]
  %inc1 = phi i32 [ %inc, %for.body ], [ 0, %entry ]
  %cmp = icmp slt i32 %inc1, 30
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %add.ptr1 = getelementptr inbounds %struct.ST, %struct.ST* %0, i32 1
  %1 = ptrtoint %struct.ST* %add.ptr1 to i32
  %rem = and i32 %1, 1
  %add = add i32 %rem, %1
  %2 = inttoptr i32 %add to %struct.ST*
  %next = getelementptr inbounds %struct.ST, %struct.ST* %0, i32 0, i32 0
  store %struct.ST* %2, %struct.ST** %next, align 4
  %inc = add nsw i32 %inc1, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  %next6 = getelementptr inbounds %struct.ST, %struct.ST* %0, i32 0, i32 0
  store %struct.ST* null, %struct.ST** %next6, align 4
  ret void
}
