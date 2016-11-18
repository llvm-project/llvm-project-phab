; RUN: llc -mtriple=aarch64-none-eabi -mattr=-neon,-crypto,-fp-armv8,-fullfp16,+neonasm,+cryptoasm,+fp-armv8asm,+fullfp16asm %s -o - | FileCheck %s

; CHECK-LABEL: fun
; CHECK: ld2
; CHECK: st2
; CHECK aese
; CHECK: dup
; CHECK: umov
; CHECK: fabd
define i32 @fun(i8 *%addr0, i8 *%addr1, i32 %input) {
entry:
  ; We can assemble neon instructions
  tail call void asm "ld2 {v0.16b, v1.16b}, $1 ; st2 {v0.16b, v1.16b}, $0", "=*Q,*Q,~{v0},~{v1}"(i8* %addr0, i8* %addr1)

  ; We can assemble crypto instructions
  tail call void asm "aese v0.16b, v1.16b;", "~{v0},~{v1}"()

  ; We can move data form the simd register file.
  %retval = tail call i32 asm "dup v1.4s, ${1:w} ; umov ${0:w}, v1.b[0];", "=r,r,~{v0},~{v1}"(i32 %input)

  ; We can use fullfp16 instructions.
  tail call void asm "fabd v0.4h, v1.4h, v2.4h;", "~{v0},~{v1},~{v2}"()

  ret i32 %retval
}
