# RUN: llvm-mc -filetype=obj -triple=armv7a-none-linux-gnueabi %s -o %t.o
# RUN: llvm-mc -filetype=obj -triple=armv7a-none-linux-gnueabi \
# RUN:    -defsym=ARRAYS_EXIST=1 %s -o %t-with-arrays.o
# RUN: ld.lld --static %t.o -o %t.exe
# RUN: ld.lld --static %t-with-arrays.o -o %t-with-arrays.exe
# RUN: llvm-objdump -t %t.exe | FileCheck %s -check-prefixes STATIC,BOTH,EMPTY-ARRAYS
# RUN: llvm-objdump -t %t-with-arrays.exe | FileCheck %s -check-prefixes STATIC,BOTH,WITH-ARRAYS
# RUN: ld.lld -pie %t.o -o %t-pie.exe -z notext
# RUN: ld.lld -pie %t-with-arrays.o -o %t-with-arrays-pie.exe -z notext
# RUN: llvm-objdump -t %t-pie.exe | FileCheck -check-prefixes PIE-EMPTY,BOTH,EMPTY-ARRAYS %s
# RUN: llvm-objdump -t %t-with-arrays-pie.exe | FileCheck -check-prefixes PIE-WITH-ARRAYS,BOTH,WITH-ARRAYS %s
# REQUIRES: arm

 .syntax unified
.section .text

  .weak _DYNAMIC
  .global _start
  .global _start
_start:
  .fnstart
  bx lr
  .word __exidx_start
  .word __exidx_end
  .word __preinit_array_start
  .word __preinit_array_end
  .word __init_array_start
  .word __init_array_end
  .word __fini_array_start
  .word __fini_array_end
  .word _DYNAMIC
  .word __start_foo
  .word __stop_foo
  .word __bss_start
  .word __ehdr_start
  .word __executable_start
  .word __dso_handle
.ifdef NOTYET
  # FIXME: if I try to reference __rel_iplt_start here, llvm-objdump
  # fails with the error: index past the end of the symbol table
  # this also happens without my patch setting the sizes so there seems to be
  # some corruption happening with a zero size .rel.iplt?
  .weak __rel_iplt_start
  .word __rel_iplt_start
  .weak __rel_iplt_end
  .word __rel_iplt_end
.endif

  .cantunwind
  .fnend

.section foo,"aw"
  .quad 0
  .quad 1

.comm bss_sym,4,4

.ifdef ARRAYS_EXIST

.section .init_array,"aw",%init_array
  .quad 0

.section .preinit_array,"aw",%preinit_array
  .quad 0
  .byte 0

.section .fini_array,"aw",%fini_array
  .quad 0
  .short 0

.endif
# PIE-EMPTY:       .dynamic     00000068 .hidden _DYNAMIC
# PIE-WITH-ARRAYS: .dynamic     00000098 .hidden _DYNAMIC
# BOTH:           .ARM.exidx   00000034 .hidden __dso_handle
# BOTH:           .ARM.exidx   00000034 .hidden __ehdr_start
# BOTH:           .ARM.exidx   00000034 .hidden __executable_start
# BOTH:           .ARM.exidx   00000000 .hidden __exidx_end
# BOTH:           .ARM.exidx   00000010 .hidden __exidx_start
# STATIC-EMPTY:   *ABS*        00000000 .hidden __fini_array_end
# STATIC-EMPTY:   *ABS*        00000000 .hidden __fini_array_start
# STATIC-EMPTY:   *ABS*        00000000 .hidden __init_array_end
# STATIC-EMPTY:   *ABS*        00000000 .hidden __init_array_start
# STATIC-EMPTY:   *ABS*        00000000 .hidden __preinit_array_end
# STATIC-EMPTY:   *ABS*        00000000 .hidden __preinit_array_start
# In PIE executables the __init/__fini symbols point to the ELF header
# PIE-EMPTY:      .ARM.exidx        00000034 .hidden __fini_array_end
# PIE-EMPTY:      .ARM.exidx        00000034 .hidden __fini_array_start
# PIE-EMPTY:      .ARM.exidx        00000034 .hidden __init_array_end
# PIE-EMPTY:      .ARM.exidx        00000034 .hidden __init_array_start
# PIE-EMPTY:      .ARM.exidx        00000034 .hidden __preinit_array_end
# PIE-EMPTY:      .ARM.exidx        00000034 .hidden __preinit_array_start
# WITH-ARRAYS:   .fini_array    00000000 .hidden __fini_array_end
# WITH-ARRAYS:   .fini_array    0000000a .hidden __fini_array_start
# WITH-ARRAYS:   .init_array    00000000 .hidden __init_array_end
# WITH-ARRAYS:   .init_array    00000008 .hidden __init_array_start
# WITH-ARRAYS:   .preinit_array 00000000 .hidden __preinit_array_end
# WITH-ARRAYS:   .preinit_array 00000009 .hidden __preinit_array_start
# STATIC:      w *UND*        00000000 _DYNAMIC
# BOTH:          .bss         00000034 __bss_start
# BOTH:          foo          00000010 __start_foo
# BOTH:          foo          00000000 __stop_foo
