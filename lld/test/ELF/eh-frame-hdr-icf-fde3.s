# REQUIRES: x86

# RUN: llvm-mc -filetype=obj -triple=x86_64-unknown-linux %s -o %t
# RUN: ld.lld %t -o %t2 --icf=all --verbose | FileCheck %s

## Check we do not apply ICF for synthetic sections. (empty .plt and .text in this case)
# CHECK-NOT: selected
