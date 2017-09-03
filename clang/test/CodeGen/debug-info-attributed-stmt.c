// RUN: %clang_cc1 -triple x86_64-unk-unk -debug-info-kind=limited -emit-llvm %s -o - | FileCheck %s
int data[50] = { 0 };

void foo()
{
    int i = 0;
    int x = 7;
#pragma nounroll
    while (i < 50)
    {
        data[i] = i;
        ++i;
    }

// CHECK: br label %while.cond, !dbg ![[NUM:[0-9]+]]
// CHECK: br i1 %cmp, label %while.body, label %while.end, !dbg ![[NUM]]
// CHECK: br label %while.cond, !dbg ![[NUM]], !llvm.loop
// CHECK: ![[NUM]] = !DILocation(line: 9, scope: !14)
}
