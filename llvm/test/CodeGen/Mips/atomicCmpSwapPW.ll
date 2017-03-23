; RUN: llc -O0 -march=mipsel -mcpu=mips32r2 -target-abi=o32 < %s -filetype=asm -o - \
; RUN:   | FileCheck -check-prefixes=PTR32,ALL %s
; RUN: llc -O0 -march=mips64el -mcpu=mips64r2 -target-abi=n32 < %s -filetype=asm -o - \
; RUN:   | FileCheck  -check-prefixes=PTR32,ALL %s
; RUN: llc -O0 -march=mips64el -mcpu=mips64r2 -target-abi=n64 < %s -filetype=asm -o - \
; RUN:   | FileCheck -check-prefixes=PTR64,ALL %s

; PTR32: lw $[[R0:[0-9]+]]
; PTR64: ld $[[R0:[0-9]+]]

; ALL: ll ${{[0-9]+}}, 0($[[R0]])

@sym = external global i32 *

define void @foo(i32 %new, i32 %old) {
entry:
  %0 = load i32 *, i32 ** @sym
  cmpxchg i32 * %0, i32 %new, i32 %old seq_cst seq_cst
  ret void
}

