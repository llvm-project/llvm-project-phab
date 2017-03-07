; RUN: opt -dse -enable-dse-partial-store-merging -S < %s | FileCheck %s
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-f128:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

; CHECK-LABEL: @combine_overlapping_stores
define void @combine_overlapping_stores(i32 *%ptr) {
entry:
  ;; This store should disappear, it should not be merged since we can't remove
  ;; the latter ones (they're not fully contained in this one) and the latter
  ;; ones overwrite the whole memory region written to by this store.
  store i32 305419896, i32* %ptr  ; 0x12345678
  %wptr = bitcast i32* %ptr to i16*
  %wptrm1 = getelementptr inbounds i16, i16* %wptr, i64 -1  ; Lower 2B
  %wptrp1 = getelementptr inbounds i16, i16* %wptr, i64 1   ; Upper 2B
  %ptrL2B = bitcast i16* %wptrm1 to i32*
  %ptrU2B = bitcast i16* %wptrp1 to i32*

  store i32 1126266452, i32* %ptrL2B  ; 0x43217654
  store i32 1985229328, i32* %ptrU2B  ; 0x76543210

; CHECK-NEXT: entry:
; CHECK-NOT: store
; CHECK: store i32 1126266452, i32* %ptrL2B
; CHECK-NEXT: store i32 1985229328, i32* %ptrU2B
; CHECK-NOT: store
; CHECK-NEXT: ret void
  ret void
}

; CHECK-LABEL: @byte_by_byte_replacement
define void @byte_by_byte_replacement(i32 *%ptr) {
entry:
  ;; This store's value should be modified as it should be better to use larger
  ;; stores than smaller ones.
  store i32 305419896, i32* %ptr  ; 0x12345678
  %bptr = bitcast i32* %ptr to i8*
  %bptr1 = getelementptr inbounds i8, i8* %bptr, i64 1
  %bptr2 = getelementptr inbounds i8, i8* %bptr, i64 2
  %bptr3 = getelementptr inbounds i8, i8* %bptr, i64 3

  store i8 9, i8* %bptr
  store i8 10, i8* %bptr1
  store i8 11, i8* %bptr2
  store i8 12, i8* %bptr3

; CHECK-NEXT: entry:
; CHECK-NOT: store
; CHECK: store i32 202050057, i32* %ptr
; CHECK-NOT: store
; CHECK-NEXT: ret void
  ret void
}

; CHECK-LABEL: @word_replacement
define void @word_replacement(i64 *%ptr) {
entry:
  store i64 72623859790382856, i64* %ptr  ; 0x0102030405060708

  %wptr = bitcast i64* %ptr to i16*
  %wptr1 = getelementptr inbounds i16, i16* %wptr, i64 1
  %wptr2 = getelementptr inbounds i16, i16* %wptr, i64 2
  %wptr3 = getelementptr inbounds i16, i16* %wptr, i64 3

  ;; We should be able to merge these two stores with the i64 one above
  store i16  4128, i16* %wptr1  ; 0x1020
  store i16 28800, i16* %wptr3  ; 0x7080

; CHECK-NEXT: entry:
; CHECK-NOT: store
;                 0x0807201004038070
; CHECK: store i64 8106482645252179720, i64* %ptr
; CHECK-NOT: store
; CHECK-NEXT: ret void
  ret void
}


; CHECK-LABEL: @differently_sized_replacements
define void @differently_sized_replacements(i64 *%ptr) {
entry:
  store i64 579005069656919567, i64* %ptr  ; 0x08090a0b0c0d0e0f

  %bptr = bitcast i64* %ptr to i8*
  %bptr6 = getelementptr inbounds i8, i8* %bptr, i64 6
  %wptr = bitcast i64* %ptr to i16*
  %wptr2 = getelementptr inbounds i16, i16* %wptr, i64 2
  %dptr = bitcast i64* %ptr to i32*

  ;; We should be able to merge all these stores with the i64 one above
  ; value (not bytes) stored before  ; 08090a0b0c0d0e0f
  store i8         7, i8*  %bptr6    ;   07
  store i16     1541, i16* %wptr2    ;     0605
  store i32 67305985, i32* %dptr     ;         04030201

; CHECK-NEXT: entry:
; CHECK-NOT: store
;                 0x0807060504030201
; CHECK: store i64 578437695752307201, i64* %ptr
; CHECK-NOT: store
; CHECK-NEXT: ret void
  ret void
}


; CHECK-LABEL: @multiple_replacements_to_same_byte
define void @multiple_replacements_to_same_byte(i64 *%ptr) {
entry:
  store i64 579005069656919567, i64* %ptr  ; 0x08090a0b0c0d0e0f

  %bptr = bitcast i64* %ptr to i8*
  %bptr3 = getelementptr inbounds i8, i8* %bptr, i64 3
  %wptr = bitcast i64* %ptr to i16*
  %wptr1 = getelementptr inbounds i16, i16* %wptr, i64 1
  %dptr = bitcast i64* %ptr to i32*

  ;; We should be able to merge all these stores with the i64 one above
  ; value (not bytes) stored before  ; 08090a0b0c0d0e0f
  store i8         7, i8*  %bptr3    ;         07
  store i16     1541, i16* %wptr1    ;         0605
  store i32 67305985, i32* %dptr     ;         04030201

; CHECK-NEXT: entry:
; CHECK-NOT: store
;                 0x08090a0b04030201
; CHECK: store i64 579005069522043393, i64* %ptr
; CHECK-NOT: store
; CHECK-NEXT: ret void
  ret void
}

;; Test case from PR31777
%union.U = type { i64 }

; CHECK-LABEL: @foo
define void @foo(%union.U* nocapture %u) {
entry:
; CHECK-NOT: store
; CHECK: store i64 42, i64* %i, align 8
; CHECK-NEXT: ret void
  %i = getelementptr inbounds %union.U, %union.U* %u, i64 0, i32 0
  store i64 0, i64* %i, align 8
  %s = bitcast %union.U* %u to i16*
  store i16 42, i16* %s, align 8
  ret void
}
