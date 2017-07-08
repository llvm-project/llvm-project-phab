; RUN: llc -march=mips -mattr=-long-calls %s -o - \
; RUN:   | FileCheck -check-prefix=OFF %s
; RUN: llc -march=mips -mattr=+long-calls %s -o - \
; RUN:   | FileCheck -check-prefix=ON %s

declare void @callee()
declare void @llvm.memset.p0i8.i32(i8* nocapture writeonly, i8, i32, i32, i1)

@val = internal unnamed_addr global [20 x i32] zeroinitializer, align 4

define void @caller() {

; Use `jal` instruction with R_MIPS_26 relocation.
; OFF: jal callee
; OFF: jal memset

; Save the `callee` and `memset` addresses in $25 register
; and use `jalr` for the jumps.
; ON:  lui    $1, %hi(callee)
; ON:  addiu  $25, $1, %lo(callee)
; ON:  jalr   $25

; ON:  addiu  $1, $zero, %lo(memset)
; ON:  lui    $2, %hi(memset)
; ON:  addu   $25, $2, $1
; ON:  jalr   $25

  call void @callee()
  call void @llvm.memset.p0i8.i32(i8* bitcast ([20 x i32]* @val to i8*), i8 0, i32 80, i32 4, i1 false)
  ret  void
}
