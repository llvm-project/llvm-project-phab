; RUN: opt < %s -S -globals-aa -lcssa -argpromotion
;
; This is to verify a fix to bug 27050. The opt arguments expands to a series
; that involves -basiccg twice. Before the fix, both those CallGraphs were
; live at the same time during execution, which caused a value handle assert
; during argpromotion since only the latest CG's asserting value handles were
; removed when a function was deleted.

%rec = type { i8 }

define void @bar() {
  call void @foo(%rec* undef)
  ret void
}

define internal void @foo(%rec* %par0) {
  ret void
}

