; RUN: llc < %s -march=mips -mcpu=mips32r2 | FileCheck --check-prefixes=FP32,ALL %s
; RUN: llc < %s -march=mips -mcpu=mips32r2 -mattr=+fp64 | FileCheck --check-prefixes=FP64,FP64EB,ALL %s
; RUN: llc < %s -march=mips64 -mcpu=mips64r2 -target-abi n64 | FileCheck --check-prefixes=MIPS64,ALL %s
; RUN: llc < %s -march=mipsel -mcpu=mips32r2 | FileCheck --check-prefixes=FP32,ALL %s
; RUN: llc < %s -march=mipsel -mcpu=mips32r2 -mattr=+fp64 | FileCheck --check-prefixes=FP64,FP64EL,ALL %s
; RUN: llc < %s -march=mips64el -mcpu=mips64r2 -target-abi n64 | FileCheck --check-prefixes=MIPS64,ALL %s


define float @uitofp32(i32 %a) {
entry:
; ALL-LABEL: uitofp32:
; FP32: mtc1 $[[R0:[0-9]+]], $f[[F0:[0-9]+]]
; FP32: bgez $[[R0]], $BB0_2
; FP32: cvt.s.w $f[[F1:[0-9]+]], $f[[F0]]
; FP32: lui $1, 16880
; FP32: mtc1 $1, $f[[F2:[0-9]+]]
; FP32: add.s $f0, $f[[F1]], $f[[F2]] 

; FP64: mtc1 $[[R0:[0-9]+]], $f[[F0:[0-9]+]]
; FP64: bgez $[[R0]], $BB0_2
; FP64: cvt.s.w $f[[F1:[0-9]+]], $f[[F0]]
; FP64: lui $1, 16880
; FP64: mtc1 $1, $f[[F2:[0-9]+]]
; FP64: add.s $f0, $f[[F1]], $f[[F2]] 

; MIPS64: mtc1 $[[R0:[0-9]+]], $f[[F0:[0-9]+]]
; MIPS64: bgez $[[R0]],
; MIPS64: cvt.s.w $f[[F1:[0-9]+]], $f[[F0]]
; MIPS64: lui $1, 16880
; MIPS64: mtc1 $1, $f[[F2:[0-9]+]]
; MIPS64: add.s $f0, $f[[F1]], $f[[F2]]

  %0 = uitofp i32 %a to float
  ret float %0
}

define i32 @fptoui32(float %a) {
entry:
; ALL-LABEL: fptoui32:
; FP32: sub.s $f[[F0:[0-9]+]], $f[[F1:[0-9]+]], $f[[F2:[0-9]+]]
; FP32: trunc.w.s $f[[F3:[0-9]+]], $f[[F0]]
; FP32: mfc1 $[[R1:.*]], $f[[F3]]
; FP32: lui $[[R3:.*]], 32768
; FP32: xor $[[R4:.*]], $[[R1]], $[[R3]]
; FP32: trunc.w.s $f[[F4:[0-9]+]], $f[[F1]]
; FP32: mfc1 $[[R2:.*]], $f[[F4]]
; FP32: c.olt.s $f[[F1]], $f[[F2]]
; FP32: jr $ra
; FP32: movt $[[R4]], $[[R2]], $fcc0

; FP64: sub.s $f[[F0:[0-9]+]], $f[[F1:[0-9]+]], $f[[F2:[0-9]+]]
; FP64: trunc.w.s $f[[F3:[0-9]+]], $f[[F0]]
; FP64: mfc1 $[[R1:.*]], $f[[F3]]
; FP64: lui $[[R3:.*]], 32768
; FP64: xor $[[R4:.*]], $[[R1]], $[[R3]]
; FP64: trunc.w.s $f[[F4:[0-9]+]], $f[[F1]]
; FP64: mfc1 $[[R2:.*]], $f[[F4]]
; FP64: c.olt.s $f[[F1]], $f[[F2]]
; FP64: jr $ra
; FP64: movt $[[R4]], $[[R2]], $fcc0

; MIPS64: sub.s $f[[F0:[0-9]+]], $f[[F1:[0-9]+]], $f[[F2:[0-9]+]]
; MIPS64: trunc.w.s $f[[F3:[0-9]+]], $f[[F0]]
; MIPS64: trunc.w.s $f[[F4:[0-9]+]], $f[[F1]]
; MIPS64: mfc1 $[[R1:.*]], $f[[F3]]
; MIPS64: mfc1 $[[R2:.*]], $f[[F4]]
; MIPS64: lui $[[R3:.*]], 32768
; MIPS64: xor $[[R4:.*]], $[[R1]], $[[R3]]
; MIPS64: c.olt.s $f[[F1]], $f[[F2]]
; MIPS64: jr $ra
; MIPS64: movt $[[R4]], $[[R2]], $fcc0

  %0 = fptoui float %a to i32
  ret i32 %0
}

define double @uitofp64(i64 %a) {
entry:
; ALL-LABEL: uitofp64:
; FP32: jal __floatundidf

; FP64EB: mtc1 $5, $f[[F0:[0-9]+]]
; FP64EB: mthc1 $4, $f[[F0]]

; FP64EL: mtc1 $4, $f[[F0:[0-9]+]]
; FP64EL: mthc1 $5, $f[[F0]]

; FP64: cvt.d.l $f0, $f[[F0]]

; MIPS64: dmtc1   $[[R0:.*]], $f[[F0:[0-9]+]]
; MIPS64: bgez    $[[R0]], .L{{.*}}
; MIPS64: cvt.d.l $f[[F1:[0-9]+]], $f[[F0]]
; MIPS64: lui     $[[R1:.*]], 17392
; MIPS64: dsll    $[[R2:.*]], $[[R1]], 32
; MIPS64: dmtc1   $[[R2]], $f[[F2:[0-9]+]]
; MIPS64: add.d   $f{{.*}}, $f[[F1]], $f[[F2]]

  %0 = uitofp i64 %a to double
  ret double %0
}

define i64 @fptoui64(double %a) {
entry:
; ALL-LABEL: fptoui64:
; FP32: jal __fixunsdfdi

; FP64: trunc.l.d $f[[F0:[0-9]+]], $f12
; FP64EB: mfhc1 $2, $f[[F0]]
; FP64EB: mfc1 $3, $f[[F0]]

; FP64EL: mfc1 $2, $f[[F0]]
; FP64EL: mfhc1 $3, $f[[F0]]

; MIPS64: trunc.l.d $f[[F0:[0-9]+]], $f12
; MIPS64: dmfc1 $2, $f[[F0]]

  %0 = fptoui double %a to i64
  ret i64 %0
}

