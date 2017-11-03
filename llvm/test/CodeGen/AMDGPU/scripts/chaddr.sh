#!/bin/bash
#
# script to convert .ll from old address space mapping
# to new address space mapping for AMDGPU.
#
# addr space change:
# 0 -> 5
# 4 -> 0
#

if [[ $# -ne 2 ]]; then
  echo "Convert .ll from old address space mapping to new address space mapping for AMDGPU"
  echo "Usage: chaddr.sh file newfile"
  exit
fi

#set -x

file=$1
newf=$2

if grep amdgiz $file >/dev/null; then
    exit
fi

sed -e 's:\([]a-zA-Z0-9_][]a-zA-Z0-9_]*\)\*:\1 addrspace(5)*:g' \
    -e 's: addrspace(4)\*:*:g' \
    -e 's: addrspace(0)\*: addrspace(5)*:g' \
    -e 's:-mtriple=\([^ ]*\):-mtriple=\1-amdgiz:g' \
    -e 's:-march=\([^ ]*\):-march=\1 -mtriple=\1---amdgiz:g' \
    -e 's:-mtriple=\([^ ]*\) \(.*\) -mtriple=\([^ ]*\): \2 -mtriple=\3:g' \
    -e 's: = alloca \(.*\): = alloca \1, addrspace(5):g' \
      $file >$newf

if ! grep mtriple $newf >/dev/null; then
  sed -i -e 's:-march=\([^ ]*\):-march=\1 -mtriple=\1---amdgiz:g' $newf
fi
 
## add missing datalayout 
if grep alloca $file >/dev/null && ! grep datalayout $file >/dev/null; then
    line=$(cat -n $file | grep ';.*RUN'| tail -1 |while read a b; do echo $a; done)
    let "line=line+1"
    sed -i "${line}i"'target datalayout = "e-p:64:64-p1:64:64-p2:64:64-p3:32:32-p4:32:32-p5:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024-v2048:2048-n32:64-A5"' $newf
fi
