; RUN: opt -S < %s -loop-rotate -verify-dom-info -verify-loop-info
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%0 = type { i64, i64, i8* }
%1 = type { %2 }
%2 = type { %3 }
%3 = type { %4 }
%4 = type { %5 }
%5 = type { %6 }
%6 = type { i64, i64, i8* }
%7 = type { i8 }

declare void @foo(%0*, %0*)

define linkonce_odr void @bar(%7*, %0*, %0*, %0**) {
  br label %5

; <label>:5:                                      ; preds = %8, %4
  %6 = phi %0* [ %2, %4 ], [ %11, %8 ]
  %7 = icmp eq %0* %6, %1
  br i1 %7, label %14, label %8

; <label>:8:                                      ; preds = %5
  %9 = load %0*, %0** %3, align 8
  %10 = getelementptr inbounds %0, %0* %9, i64 -1
  %11 = getelementptr inbounds %0, %0* %6, i64 -1
  tail call void @foo(%0* %10, %0* nonnull dereferenceable(24) %11)
  %12 = load %0*, %0** %3, align 8
  %13 = getelementptr inbounds %0, %0* %12, i64 -1
  store %0* %13, %0** %3, align 8
  br label %5

; <label>:14:                                     ; preds = %5
  ret void
}
