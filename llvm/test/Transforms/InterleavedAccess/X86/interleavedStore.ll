; NOTE: Assertions have been autogenerated by utils/update_test_checks.py
; RUN: opt < %s -mtriple=x86_64-pc-linux -mattr=+avx -mattr=+avx2 -interleaved-access -S | FileCheck %s

define void @interleaved_store_vf32_i8_stride4(<32 x i8> %x1, <32 x i8> %x2, <32 x i8> %x3, <32 x i8> %x4, <128 x i8>* %p) {
; CHECK-LABEL: @interleaved_store_vf32_i8_stride4(
; CHECK-NEXT:    [[V1:%.*]] = shufflevector <32 x i8> [[X1:%.*]], <32 x i8> [[X2:%.*]], <64 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15, i32 16, i32 17, i32 18, i32 19, i32 20, i32 21, i32 22, i32 23, i32 24, i32 25, i32 26, i32 27, i32 28, i32 29, i32 30, i32 31, i32 32, i32 33, i32 34, i32 35, i32 36, i32 37, i32 38, i32 39, i32 40, i32 41, i32 42, i32 43, i32 44, i32 45, i32 46, i32 47, i32 48, i32 49, i32 50, i32 51, i32 52, i32 53, i32 54, i32 55, i32 56, i32 57, i32 58, i32 59, i32 60, i32 61, i32 62, i32 63>
; CHECK-NEXT:    [[V2:%.*]] = shufflevector <32 x i8> [[X3:%.*]], <32 x i8> [[X4:%.*]], <64 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15, i32 16, i32 17, i32 18, i32 19, i32 20, i32 21, i32 22, i32 23, i32 24, i32 25, i32 26, i32 27, i32 28, i32 29, i32 30, i32 31, i32 32, i32 33, i32 34, i32 35, i32 36, i32 37, i32 38, i32 39, i32 40, i32 41, i32 42, i32 43, i32 44, i32 45, i32 46, i32 47, i32 48, i32 49, i32 50, i32 51, i32 52, i32 53, i32 54, i32 55, i32 56, i32 57, i32 58, i32 59, i32 60, i32 61, i32 62, i32 63>
; CHECK-NEXT:    [[TMP1:%.*]] = shufflevector <64 x i8> [[V1]], <64 x i8> [[V2]], <32 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15, i32 16, i32 17, i32 18, i32 19, i32 20, i32 21, i32 22, i32 23, i32 24, i32 25, i32 26, i32 27, i32 28, i32 29, i32 30, i32 31>
; CHECK-NEXT:    [[TMP2:%.*]] = shufflevector <64 x i8> [[V1]], <64 x i8> [[V2]], <32 x i32> <i32 32, i32 33, i32 34, i32 35, i32 36, i32 37, i32 38, i32 39, i32 40, i32 41, i32 42, i32 43, i32 44, i32 45, i32 46, i32 47, i32 48, i32 49, i32 50, i32 51, i32 52, i32 53, i32 54, i32 55, i32 56, i32 57, i32 58, i32 59, i32 60, i32 61, i32 62, i32 63>
; CHECK-NEXT:    [[TMP3:%.*]] = shufflevector <64 x i8> [[V1]], <64 x i8> [[V2]], <32 x i32> <i32 64, i32 65, i32 66, i32 67, i32 68, i32 69, i32 70, i32 71, i32 72, i32 73, i32 74, i32 75, i32 76, i32 77, i32 78, i32 79, i32 80, i32 81, i32 82, i32 83, i32 84, i32 85, i32 86, i32 87, i32 88, i32 89, i32 90, i32 91, i32 92, i32 93, i32 94, i32 95>
; CHECK-NEXT:    [[TMP4:%.*]] = shufflevector <64 x i8> [[V1]], <64 x i8> [[V2]], <32 x i32> <i32 96, i32 97, i32 98, i32 99, i32 100, i32 101, i32 102, i32 103, i32 104, i32 105, i32 106, i32 107, i32 108, i32 109, i32 110, i32 111, i32 112, i32 113, i32 114, i32 115, i32 116, i32 117, i32 118, i32 119, i32 120, i32 121, i32 122, i32 123, i32 124, i32 125, i32 126, i32 127>
; CHECK-NEXT:    [[VPUNPCKLBW:%.*]] = shufflevector <32 x i8> [[TMP1]], <32 x i8> [[TMP2]], <32 x i32> <i32 0, i32 32, i32 1, i32 33, i32 2, i32 34, i32 3, i32 35, i32 4, i32 36, i32 5, i32 37, i32 6, i32 38, i32 7, i32 39, i32 16, i32 48, i32 17, i32 49, i32 18, i32 50, i32 19, i32 51, i32 20, i32 52, i32 21, i32 53, i32 22, i32 54, i32 23, i32 55>
; CHECK-NEXT:    [[VPUNPCKHBW:%.*]] = shufflevector <32 x i8> [[TMP1]], <32 x i8> [[TMP2]], <32 x i32> <i32 8, i32 40, i32 9, i32 41, i32 10, i32 42, i32 11, i32 43, i32 12, i32 44, i32 13, i32 45, i32 14, i32 46, i32 15, i32 47, i32 24, i32 56, i32 25, i32 57, i32 26, i32 58, i32 27, i32 59, i32 28, i32 60, i32 29, i32 61, i32 30, i32 62, i32 31, i32 63>
; CHECK-NEXT:    [[VPUNPCKLBW1:%.*]] = shufflevector <32 x i8> [[TMP3]], <32 x i8> [[TMP4]], <32 x i32> <i32 0, i32 32, i32 1, i32 33, i32 2, i32 34, i32 3, i32 35, i32 4, i32 36, i32 5, i32 37, i32 6, i32 38, i32 7, i32 39, i32 16, i32 48, i32 17, i32 49, i32 18, i32 50, i32 19, i32 51, i32 20, i32 52, i32 21, i32 53, i32 22, i32 54, i32 23, i32 55>
; CHECK-NEXT:    [[VPUNPCKHBW2:%.*]] = shufflevector <32 x i8> [[TMP3]], <32 x i8> [[TMP4]], <32 x i32> <i32 8, i32 40, i32 9, i32 41, i32 10, i32 42, i32 11, i32 43, i32 12, i32 44, i32 13, i32 45, i32 14, i32 46, i32 15, i32 47, i32 24, i32 56, i32 25, i32 57, i32 26, i32 58, i32 27, i32 59, i32 28, i32 60, i32 29, i32 61, i32 30, i32 62, i32 31, i32 63>
; CHECK-NEXT:    [[TMP5:%.*]] = bitcast <32 x i8> [[VPUNPCKLBW]] to <16 x i16>
; CHECK-NEXT:    [[TMP6:%.*]] = bitcast <32 x i8> [[VPUNPCKHBW]] to <16 x i16>
; CHECK-NEXT:    [[TMP7:%.*]] = bitcast <32 x i8> [[VPUNPCKLBW1]] to <16 x i16>
; CHECK-NEXT:    [[TMP8:%.*]] = bitcast <32 x i8> [[VPUNPCKHBW2]] to <16 x i16>
; CHECK-NEXT:    [[VPUNPCKHBW3:%.*]] = shufflevector <16 x i16> [[TMP5]], <16 x i16> [[TMP7]], <16 x i32> <i32 4, i32 20, i32 5, i32 21, i32 6, i32 22, i32 7, i32 23, i32 12, i32 28, i32 13, i32 29, i32 14, i32 30, i32 15, i32 31>
; CHECK-NEXT:    [[VPUNPCKHBW4:%.*]] = shufflevector <16 x i16> [[TMP6]], <16 x i16> [[TMP8]], <16 x i32> <i32 4, i32 20, i32 5, i32 21, i32 6, i32 22, i32 7, i32 23, i32 12, i32 28, i32 13, i32 29, i32 14, i32 30, i32 15, i32 31>
; CHECK-NEXT:    [[VPUNPCKLBW5:%.*]] = shufflevector <16 x i16> [[TMP5]], <16 x i16> [[TMP7]], <16 x i32> <i32 0, i32 16, i32 1, i32 17, i32 2, i32 18, i32 3, i32 19, i32 8, i32 24, i32 9, i32 25, i32 10, i32 26, i32 11, i32 27>
; CHECK-NEXT:    [[VPUNPCKLBW6:%.*]] = shufflevector <16 x i16> [[TMP6]], <16 x i16> [[TMP8]], <16 x i32> <i32 0, i32 16, i32 1, i32 17, i32 2, i32 18, i32 3, i32 19, i32 8, i32 24, i32 9, i32 25, i32 10, i32 26, i32 11, i32 27>
; CHECK-NEXT:    [[CONCATLOW:%.*]] = shufflevector <16 x i16> [[VPUNPCKLBW5]], <16 x i16> [[VPUNPCKHBW3]], <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 16, i32 17, i32 18, i32 19, i32 20, i32 21, i32 22, i32 23>
; CHECK-NEXT:    [[CONCATLOW7:%.*]] = shufflevector <16 x i16> [[VPUNPCKLBW6]], <16 x i16> [[VPUNPCKHBW4]], <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 16, i32 17, i32 18, i32 19, i32 20, i32 21, i32 22, i32 23>
; CHECK-NEXT:    [[CONCATHIGH:%.*]] = shufflevector <16 x i16> [[VPUNPCKLBW5]], <16 x i16> [[VPUNPCKHBW3]], <16 x i32> <i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15, i32 24, i32 25, i32 26, i32 27, i32 28, i32 29, i32 30, i32 31>
; CHECK-NEXT:    [[CONCATHIGH8:%.*]] = shufflevector <16 x i16> [[VPUNPCKLBW6]], <16 x i16> [[VPUNPCKHBW4]], <16 x i32> <i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15, i32 24, i32 25, i32 26, i32 27, i32 28, i32 29, i32 30, i32 31>
; CHECK-NEXT:    [[TMP9:%.*]] = bitcast <128 x i8>* [[P:%.*]] to <16 x i16>*
; CHECK-NEXT:    [[TMP10:%.*]] = getelementptr <16 x i16>, <16 x i16>* [[TMP9]], i32 0
; CHECK-NEXT:    store <16 x i16> [[CONCATLOW]], <16 x i16>* [[TMP10]]
; CHECK-NEXT:    [[TMP11:%.*]] = getelementptr <16 x i16>, <16 x i16>* [[TMP9]], i32 1
; CHECK-NEXT:    store <16 x i16> [[CONCATLOW7]], <16 x i16>* [[TMP11]]
; CHECK-NEXT:    [[TMP12:%.*]] = getelementptr <16 x i16>, <16 x i16>* [[TMP9]], i32 2
; CHECK-NEXT:    store <16 x i16> [[CONCATHIGH]], <16 x i16>* [[TMP12]]
; CHECK-NEXT:    [[TMP13:%.*]] = getelementptr <16 x i16>, <16 x i16>* [[TMP9]], i32 3
; CHECK-NEXT:    store <16 x i16> [[CONCATHIGH8]], <16 x i16>* [[TMP13]]
; CHECK-NEXT:    ret void
;
  %v1 = shufflevector <32 x i8> %x1, <32 x i8> %x2, <64 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15, i32 16, i32 17, i32 18, i32 19, i32 20, i32 21, i32 22, i32 23, i32 24, i32 25, i32 26, i32 27, i32 28, i32 29, i32 30, i32 31, i32 32, i32 33, i32 34, i32 35, i32 36, i32 37, i32 38, i32 39, i32 40, i32 41, i32 42, i32 43, i32 44, i32 45, i32 46, i32 47, i32 48, i32 49, i32 50, i32 51, i32 52, i32 53, i32 54, i32 55, i32 56, i32 57, i32 58, i32 59, i32 60, i32 61, i32 62, i32 63>
  %v2 = shufflevector <32 x i8> %x3, <32 x i8> %x4, <64 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15, i32 16, i32 17, i32 18, i32 19, i32 20, i32 21, i32 22, i32 23, i32 24, i32 25, i32 26, i32 27, i32 28, i32 29, i32 30, i32 31, i32 32, i32 33, i32 34, i32 35, i32 36, i32 37, i32 38, i32 39, i32 40, i32 41, i32 42, i32 43, i32 44, i32 45, i32 46, i32 47, i32 48, i32 49, i32 50, i32 51, i32 52, i32 53, i32 54, i32 55, i32 56, i32 57, i32 58, i32 59, i32 60, i32 61, i32 62, i32 63>
  %interleaved.vec = shufflevector <64 x i8> %v1, <64 x i8> %v2, <128 x i32> <i32 0, i32 32, i32 64, i32 96, i32 1, i32 33, i32 65, i32 97, i32 2, i32 34, i32 66, i32 98, i32 3, i32 35, i32 67, i32 99, i32 4, i32 36, i32 68, i32 100, i32 5, i32 37, i32 69, i32 101, i32 6, i32 38, i32 70, i32 102, i32 7, i32 39, i32 71, i32 103, i32 8, i32 40, i32 72, i32 104, i32 9, i32 41, i32 73, i32 105, i32 10, i32 42, i32 74, i32 106, i32 11, i32 43, i32 75, i32 107, i32 12, i32 44, i32 76, i32 108, i32 13, i32 45, i32 77, i32 109, i32 14, i32 46, i32 78, i32 110, i32 15, i32 47, i32 79, i32 111, i32 16, i32 48, i32 80, i32 112, i32 17, i32 49, i32 81, i32 113, i32 18, i32 50, i32 82, i32 114, i32 19, i32 51, i32 83, i32 115, i32 20, i32 52, i32 84, i32 116, i32 21, i32 53, i32 85, i32 117, i32 22, i32 54, i32 86, i32 118, i32 23, i32 55, i32 87, i32 119, i32 24, i32 56, i32 88, i32 120, i32 25, i32 57, i32 89, i32 121, i32 26, i32 58, i32 90, i32 122, i32 27, i32 59, i32 91, i32 123, i32 28, i32 60, i32 92, i32 124, i32 29, i32 61, i32 93, i32 125, i32 30, i32 62, i32 94, i32 126, i32 31, i32 63, i32 95, i32 127>
  store <128 x i8> %interleaved.vec, <128 x i8>* %p
  ret void
}
