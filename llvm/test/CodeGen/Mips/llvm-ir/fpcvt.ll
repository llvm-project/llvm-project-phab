; RUN: llc < %s -march=mips -mcpu=mips32r2 -verify-machineinstrs | FileCheck --check-prefixes=FP32,ALL %s
; RUN: llc < %s -march=mips -mcpu=mips32r2 -mattr=+fp64 -verify-machineinstrs | FileCheck --check-prefixes=FP64,FP64EB,ALL %s
; RUN: llc < %s -march=mips64 -mcpu=mips64r2 -target-abi n64 -verify-machineinstrs | FileCheck --check-prefixes=MIPS64,ALL %s
; RUN: llc < %s -march=mipsel -mcpu=mips32r2 -verify-machineinstrs | FileCheck --check-prefixes=FP32,ALL %s
; RUN: llc < %s -march=mipsel -mcpu=mips32r2 -mattr=+fp64 -verify-machineinstrs | FileCheck --check-prefixes=FP64,FP64EL,ALL %s
; RUN: llc < %s -march=mips64el -mcpu=mips64r2 -target-abi n64 -verify-machineinstrs | FileCheck --check-prefixes=MIPS64,ALL %s


define float @uitofp32(i32 %a) {
entry:
; ALL-LABEL: uitofp32:
; FP32: mtc1 $[[R0:[0-9]+]], $f[[F0:[0-9]+]]
; FP32: bgez $[[R0]], $BB0_2
; FP32: cvt.d.w $f[[F1:[0-9]+]], $f[[F0]]
; FP32: lui $1, 16880
; FP32: mtc1 $zero, $f[[F2:[0-9]+]]
; FP32: mthc1 $1, $f[[F2]]
; FP32: add.d $f0, $f[[F1]], $f[[F2]] 

; FP64: mtc1    ${{.*}}, $f[[F0:[0-9]+]]
; FP64: mthc1   $zero, $f[[F0]]
; FP64: cvt.s.l $f{{.*}}, $f[[F0]]

; MIPS64: dext $[[R0:[0-9]+]], ${{.*}}, 0, 32
; MIPS64: dmtc1   $[[R0]], $f[[F0:[0-9]+]]
; MIPS64: cvt.s.l $f{{.*}}, $f[[F0]]

  %0 = uitofp i32 %a to float
  ret float %0
}

define i32 @fptoui32(float %a) {
entry:
; ALL-LABEL: fptoui32:
; ALL: lui $[[R0:[0-9]+]], 20224
; ALL: mtc1 $[[R0]], $f[[F0:[0-9]+]]
; ALL: c.le.s $f[[F0]], $f[[F1:[0-9]+]]
; ALL: bc1t [[BR1:.*]]
; ALL: nop
; ALL: trunc.w.s $f[[F2:[0-9]+]], $f[[F1]]
; ALL: jr $ra
; ALL: mfc1 {{.*}}, $f[[F2]]
; ALL: [[BR1]]:
; ALL: sub.s $f[[F2:[0-9]+]], $f[[F1]], $f[[F0]]
; ALL: trunc.w.s $f[[F3:[0-9]+]], $f[[F2]]
; ALL: mfc1 $[[R1:[0-9]+]], $f[[F3]]
; ALL: lui $[[R2:[0-9]+]], 32768
; ALL: jr $ra
; ALL: or {{.*}}, $[[R1]], $[[R2]]

  %0 = fptoui float %a to i32
  ret i32 %0
}

define double @uitofp64(i64 %a) {
entry:
; ALL-LABEL: uitofp64:
; FP32: jal __floatundidf

; FP64: jal __floatundidf

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

; FP64: jal __fixunsdfdi

; MIPS64: lui $[[R0:[0-9]+]], 16864
; MIPS64: dsll $[[R1:[0-9]+]], $[[R0]], 32
; MIPS64: dmtc1 $[[R1]], $f[[F0:[0-9]+]]
; MIPS64: c.le.d $f[[F0]], $f[[F1:[0-9]+]]
; MIPS64: bc1t [[BR:.*]]
; MIPS64: nop
; MIPS64: trunc.l.d $f[[F2:[0-9]+]], $f[[F1]]
; MIPS64: jr $ra
; MIPS64: dmfc1 $2, $f[[F2]]
; MIPS64: [[BR]]:
; MIPS64: sub.d $f[[F2:[0-9]+]], $f[[F1]], $f[[F0]]
; MIPS64: trunc.l.d $f[[F3:[0-9]+]], $f[[F2]]
; MIPS64: dmfc1 $[[R2:[0-9]+]], $f[[F3]]
; MIPS64: lui $[[R3:[0-9]+]], 32768
; MIPS64: dsll32 $[[R4:[0-9]+]], $[[R3]], 0
; MIPS64: jr $ra
; MIPS64: or $2, $[[R2]], $[[R4]]

  %0 = fptoui double %a to i64
  ret i64 %0
}
