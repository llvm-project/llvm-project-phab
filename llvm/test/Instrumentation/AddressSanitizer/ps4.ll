; RUN: %opt_asan < %s -asan -asan-module -S -mtriple=x86_64-scei-ps4 | FileCheck --check-prefixes=CHECK,CHECK-S%scale %s

define i32 @read_4_bytes(i32* %a) sanitize_address {
entry:
  %tmp1 = load i32, i32* %a, align 4
  ret i32 %tmp1
}

; CHECK: @read_4_bytes
; CHECK-NOT: ret
; Check for ASAN's Offset on the PS4 (2^40 or 0x10000000000)
; CHECK-S3: lshr {{.*}} 3
; CHECK-S5: lshr {{.*}} 5
; CHECK-NEXT: {{1099511627776}}
; CHECK: ret
