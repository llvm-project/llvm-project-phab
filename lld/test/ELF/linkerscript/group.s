# REQUIRES: x86

# RUN: mkdir -p %t.dir
# RUN: rm -f %t.dir/libxyz.a
# RUN: llvm-mc -filetype=obj -triple=x86_64-unknown-linux %s -o %t
# RUN: llvm-mc -filetype=obj -triple=x86_64-unknown-linux \
# RUN:   %p/Inputs/libsearch-st.s -o %t2.o
# RUN: llvm-ar rcs %t.dir/libxyz.a %t2.o

# RUN: echo "GROUP(\"%t\")" > %t.script
# RUN: ld.lld -o %t2 %t.script
# RUN: llvm-readobj %t2 > /dev/null

# RUN: echo "INPUT(\"%t\")" > %t.script
# RUN: ld.lld -o %t2 %t.script
# RUN: llvm-readobj %t2 > /dev/null

# RUN: echo "GROUP(\"%t\" libxyz.a )" > %t.script
# RUN: not ld.lld -o %t2 %t.script 2>/dev/null
# RUN: ld.lld -o %t2 %t.script -L%t.dir
# RUN: llvm-readobj %t2 > /dev/null

# RUN: echo "GROUP(\"%t\" =libxyz.a )" > %t.script
# RUN: not ld.lld -o %t2 %t.script  2>/dev/null
# RUN: ld.lld -o %t2 %t.script --sysroot=%t.dir
# RUN: llvm-readobj %t2 > /dev/null

# RUN: echo "GROUP(\"%t\" -lxyz )" > %t.script
# RUN: not ld.lld -o %t2 %t.script  2>/dev/null
# RUN: ld.lld -o %t2 %t.script -L%t.dir
# RUN: llvm-readobj %t2 > /dev/null

# RUN: echo "GROUP(\"%t\" libxyz.a )" > %t.script
# RUN: not ld.lld -o %t2 %t.script  2>/dev/null
# RUN: ld.lld -o %t2 %t.script -L%t.dir
# RUN: llvm-readobj %t2 > /dev/null

# RUN: echo "GROUP(\"%t\" /libxyz.a )" > %t.script
# RUN: echo "GROUP(\"%t\" /libxyz.a )" > %t.dir/xyz.script
# RUN: not ld.lld -o %t2 %t.script 2>/dev/null
# RUN: not ld.lld -o %t2 %t.script --sysroot=%t.dir  2>/dev/null
# RUN: ld.lld -o %t2 %t.dir/xyz.script --sysroot=%t.dir
# RUN: llvm-readobj %t2 > /dev/null

# RUN: echo "GROUP(\"%t.script2\")" > %t.script1
# RUN: echo "GROUP(\"%t\")" > %t.script2
# RUN: ld.lld -o %t2 %t.script1
# RUN: llvm-readobj %t2 > /dev/null

# RUN: echo "GROUP(AS_NEEDED(\"%t\"))" > %t.script
# RUN: ld.lld -o %t2 %t.script
# RUN: llvm-readobj %t2 > /dev/null

# RUN: echo "INPUT(/no_such_file)" > %t.script
# RUN: not ld.lld -o %t2 %t.script 2>&1 | FileCheck -check-prefix=ERR %s
# ERR: cannot open {{.*}}no_such_file: {{[Nn]}}o such file or directory
# ERR: >>> location:{{.*}}.script:1

# RUN: echo "INPUT(AS_NEEDED(/no_such_file))" > %t.script
# RUN: not ld.lld -o %t2 %t.script 2>&1 | FileCheck -check-prefix=ERR %s

# RUN: echo "GROUP(/no_such_file)" > %t.script
# RUN: not ld.lld -o %t2 %t.script 2>&1 | FileCheck -check-prefix=ERR %s

# RUN: echo "GROUP(AS_NEEDED(/no_such_file))" > %t.script
# RUN: not ld.lld -o %t2 %t.script 2>&1 | FileCheck -check-prefix=ERR %s

.globl _start
_start:
  ret
