# RUN: llvm-mc -filetype=obj -triple=x86_64-unknown-linux %s -o %t.o
# RUN: not ld.lld %t.o -o %t1 2>&1 | FileCheck -check-prefix=ERR1 %s
# ERR1: error: undefined symbol: foo
# ERR1: error: undefined symbol: foo
# ERR1: error: undefined symbol: boo
# ERR1: error: undefined symbol: boo

# RUN: not ld.lld --warn-once %t.o -o %t1 2>&1 | FileCheck -check-prefix=ERR2 %s
# ERR2: error: undefined symbol: foo
# ERR2-NOT: foo
# ERR2: error: undefined symbol: boo
# ERR2-NOT: boo

callq foo@PLT
callq foo@PLT
callq boo@PLT
callq boo@PLT
