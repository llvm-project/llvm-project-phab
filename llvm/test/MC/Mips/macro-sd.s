# RUN: llvm-mc -triple mips-unknown-freebsd -show-encoding %s | FileCheck %s
# RUN: not llvm-mc -triple mips-unknown-freebsd -show-encoding %s -defsym=BAD_MACRO=1 2>&1 | FileCheck -check-prefix=ERR %s
.data
.globl fenvp
fenvp:
.space 8


.set noreorder
.text
_start:

.ifdef BAD_MACRO
# These are not expected to work:
sd $6, fenvp  # ERR: macro-sd.s:[[@LINE]]:1: error: offset for sd macro is not an immediate
ld $6, fenvp  # ERR: macro-sd.s:[[@LINE]]:1: error: offset for ld macro is not an immediate
.else
sd $6, 0($2)
# CHECK:      sw      $6, 0($2)               # encoding: [0xac,0x46,0x00,0x00]
# CHECK-NEXT: sw      $7, 4($2)               # encoding: [0xac,0x47,0x00,0x04]
ld $6, 0($2)
# CHECK:      lw      $6, 0($2)               # encoding: [0x8c,0x46,0x00,0x00]
# CHECK-NEXT: lw      $7, 4($2)               # encoding: [0x8c,0x47,0x00,0x04]
.endif
