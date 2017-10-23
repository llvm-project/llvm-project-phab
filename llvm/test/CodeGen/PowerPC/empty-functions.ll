; RUN: llc -verify-machineinstrs < %s \
; RUN:   -mtriple=powerpc-apple-darwin | FileCheck -check-prefix=CHECK-MACHO %s
; RUN: llc -verify-machineinstrs < %s \
; RUN:   -mtriple=powerpc-apple-darwin -disable-fp-elim | FileCheck -check-prefix=CHECK-MACHO %s
; RUN: llc -ppc-strip-register-prefix -verify-machineinstrs < %s \
; RUN:   -mtriple=powerpc-linux-gnu | FileCheck -check-prefix=LINUX-NO-FP %s
; RUN: llc -ppc-strip-register-prefix -verify-machineinstrs < %s \
; RUN:   -mtriple=powerpc-linux-gnu -disable-fp-elim | FileCheck \
; RUN:   -check-prefix=LINUX-FP %s

define void @func() {
entry:
  unreachable
}

; MachO cannot handle an empty function.
; CHECK-MACHO:     _func:
; CHECK-MACHO-NEXT: .cfi_startproc
; CHECK-MACHO-NEXT: {{^}};
; CHECK-MACHO-NEXT:     nop
; CHECK-MACHO-NEXT: .cfi_endproc

; An empty function is perfectly fine on ELF.
; LINUX-NO-FP: func:
; LINUX-NO-FP-NEXT: {{^}}.L[[BEGIN:.*]]:{{$}}
; LINUX-NO-FP-NEXT: .cfi_startproc
; LINUX-NO-FP-NEXT: {{^}}#
; LINUX-NO-FP-NEXT: {{^}}.L[[END:.*]]:{{$}}
; LINUX-NO-FP-NEXT: .size   func, .L[[END]]-.L[[BEGIN]]
; LINUX-NO-FP-NEXT: .cfi_endproc

; A cfi directive cannot point to the end of a function.
; LINUX-FP: func:
; LINUX-FP-NEXT: {{^}}.L[[BEGIN:.*]]:{{$}}
; LINUX-FP-NEXT: .cfi_startproc
; LINUX-FP-NEXT: {{^}}#
; LINUX-FP-NEXT: stwu 1, -16(1)
; LINUX-FP-NEXT: stw 31, 12(1)
; LINUX-FP-NEXT:  .cfi_def_cfa_offset 16
; LINUX-FP-NEXT: .cfi_offset r31, -4
; LINUX-FP-NEXT: mr 31, 1
; LINUX-FP-NEXT: {{^}}.L[[END:.*]]:{{$}}
; LINUX-FP-NEXT: .size   func, .L[[END]]-.L[[BEGIN]]
; LINUX-FP-NEXT: .cfi_endproc
