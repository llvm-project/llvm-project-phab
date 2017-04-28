; RUN: llc < %s -o - | FileCheck %s

target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64--linux-gnu"

%struct.16B = type { i16, i16 , i16, i16, i16, i16, i16, i16}
define i32 @func_16B(i16 %c, i16 %c2, i16* %base, i32 %i, i16 %v16, %struct.16B* %P) {
; CHECK-LABEL: @func_16B

entry:
  %s_ext = sext i32 %i to i64

; CHECK-LABEL: %entry
; CHECK: ldrh   w{{[0-9]+}}, [x[[ADDR0:[0-9]+]]]

  %addr0 = getelementptr inbounds %struct.16B, %struct.16B* %P, i64 %s_ext, i32 0
  %cc = load i16, i16* %addr0
  %cmp = icmp eq i16 %cc, %c
  br i1 %cmp, label %if.then, label %out

if.then:
; CHECK-LABEL: %if.then
; CHECK-NOT: sxtw x{{[0-9]+}}, w{{[0-9]+}}
; CHECK: ldrh   w{{[0-9]+}}, [x{{[0-9]+}}, w{{[0-9]+}}, sxtw #1]
  %addr1 = getelementptr inbounds i16, i16* %base, i64 %s_ext
  %v = load i16, i16* %addr1
  %cmp2 = icmp eq i16 %v, %c2
  br i1 %cmp2, label %if.then2, label %out

if.then2:
; CHECK-LABEL: %if.then2
; CHECK-NOT: add x{{[0-9]+}}, x{{[0-9]+}}, x{{[0-9]+}}
; CHECK: strh   w{{[0-9]+}}, [x[[ADDR0]]]
; CHECK: strh   w{{[0-9]+}}, [x[[ADDR0]], #2]

  %addr2 = getelementptr inbounds %struct.16B, %struct.16B* %P, i64 %s_ext, i32 1
  store i16 %v16, i16* %addr0
  store i16 %v16, i16* %addr2
  ret i32  0

out:
  ret i32 0
}

%struct.8B = type { i16, i16 , i16, i16}
define i32 @func_8B(i16 %c, i16 %c2, i16* %base, i32 %i, i16 %v16, %struct.8B* %P) {
; CHECK-LABEL: @func_8B

entry:
  %s_ext = sext i32 %i to i64

; CHECK-LABEL: %entry
; CHECK: ldrh   w{{[0-9]+}}, [x[[ADDR0:[0-9]+]]]

  %addr0 = getelementptr inbounds %struct.8B, %struct.8B* %P, i64 %s_ext, i32 0
  %cc = load i16, i16* %addr0
  %cmp = icmp eq i16 %cc, %c
  br i1 %cmp, label %if.then, label %out

if.then:
; CHECK-LABEL: %if.then
; CHECK-NOT: sxtw x{{[0-9]+}}, w{{[0-9]+}}
; CHECK: ldrh   w{{[0-9]+}}, [x{{[0-9]+}}, w{{[0-9]+}}, sxtw #1]
  %addr1 = getelementptr inbounds i16, i16* %base, i64 %s_ext
  %v = load i16, i16* %addr1
  %cmp2 = icmp eq i16 %v, %c2
  br i1 %cmp2, label %if.then2, label %out

if.then2:
; CHECK-LABEL: %if.then2
; CHECK-NOT: add x{{[0-9]+}}, x{{[0-9]+}}, x{{[0-9]+}}
; CHECK: strh   w{{[0-9]+}}, [x[[ADDR0]]]
; CHECK: strh   w{{[0-9]+}}, [x[[ADDR0]], #2]

  %addr2 = getelementptr inbounds %struct.8B, %struct.8B* %P, i64 %s_ext, i32 1
  store i16 %v16, i16* %addr0
  store i16 %v16, i16* %addr2
  ret i32  0

out:
  ret i32 0
}

%struct.4B = type { i16, i16 }
define i32 @func_4B(i16 %c, i16 %c2, i16* %base, i32 %i, i16 %v16, %struct.4B* %P) {
; CHECK-LABEL: @func_4B

entry:
  %s_ext = sext i32 %i to i64

; CHECK-LABEL: %entry
; CHECK: ldrh   w{{[0-9]+}}, [x[[ADDR0:[0-9]+]]]

  %addr0 = getelementptr inbounds %struct.4B, %struct.4B* %P, i64 %s_ext, i32 0
  %cc = load i16, i16* %addr0
  %cmp = icmp eq i16 %cc, %c
  br i1 %cmp, label %if.then, label %out

if.then:
; CHECK-LABEL: %if.then
; CHECK-NOT: sxtw x{{[0-9]+}}, w{{[0-9]+}}
; CHECK: ldrh   w{{[0-9]+}}, [x{{[0-9]+}}, w{{[0-9]+}}, sxtw #1]
  %addr1 = getelementptr inbounds i16, i16* %base, i64 %s_ext
  %v = load i16, i16* %addr1
  %cmp2 = icmp eq i16 %v, %c2
  br i1 %cmp2, label %if.then2, label %out

if.then2:
; CHECK-LABEL: %if.then2
; CHECK-NOT: add x{{[0-9]+}}, x{{[0-9]+}}, x{{[0-9]+}}
; CHECK: strh   w{{[0-9]+}}, [x[[ADDR0]]]
; CHECK: strh   w{{[0-9]+}}, [x[[ADDR0]], #2]

  %addr2 = getelementptr inbounds %struct.4B, %struct.4B* %P, i64 %s_ext, i32 1
  store i16 %v16, i16* %addr0
  store i16 %v16, i16* %addr2
  ret i32  0

out:
  ret i32 0
}

