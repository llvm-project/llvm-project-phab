; RUN: opt -S -canonicalize-aliases < %s | FileCheck %s
; RUN: opt -prepare-for-thinlto -O0 -module-summary -o - < %s | llvm-dis -o - | FileCheck %s --check-prefix=NAMEDANON

; CHECK-DAG: @fooalias = alias void (...), bitcast (void ()* @1
; CHECK-DAG: @foo = alias void (), void ()* @1
; CHECK-DAG: define private void @1()
; NAMEDANON-DAG: @fooalias = alias void (...), bitcast (void ()* @anon.d41d8cd98f00b204e9800998ecf8427e.0
; NAMEDANON-DAG: @foo = alias void (), void ()* @anon.d41d8cd98f00b204e9800998ecf8427e.0
; NAMEDANON-DAG: define private void @anon.d41d8cd98f00b204e9800998ecf8427e.0()
@fooalias = alias void (...), bitcast (void ()* @foo to void (...)*)
define void @foo() {
    ret void
}

; CHECK-DAG: @0 = private global
; CHECK-DAG: @baralias = alias i8, i8* @0
; CHECK-DAG: @bar = alias i8, i8* @0
; NAMEDANON-DAG: @anon.d41d8cd98f00b204e9800998ecf8427e.1 = private global
; NAMEDANON-DAG: @baralias = alias i8, i8* @anon.d41d8cd98f00b204e9800998ecf8427e.1
; NAMEDANON-DAG: @bar = alias i8, i8* @anon.d41d8cd98f00b204e9800998ecf8427e.1
@baralias = alias i8, i8 *@bar
@bar = global i8 0
