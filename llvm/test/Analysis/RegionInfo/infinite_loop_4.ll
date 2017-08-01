; REQUIRES: asserts
; RUN: opt -regions -analyze < %s 
; RUN: opt -regions -stats -disable-output < %s 2>&1 | FileCheck -check-prefix=STAT %s
; RUN: opt -regions -print-region-style=bb  -analyze < %s 2>&1 | FileCheck -check-prefix=BBIT %s
; RUN: opt -regions -print-region-style=rn  -analyze < %s 2>&1 | FileCheck -check-prefix=RNIT %s

define void @normal_condition() nounwind {
0:
	br label %"7"
7:
	br i1 1, label %"1", label %"8"
1:
	br i1 1, label %"2", label %"3"
2:
	br label %"5"
5:
	br i1 1, label %"11", label %"12"
11:
        br label %"6"
12:
        br label %"6"
6:
	br i1 1, label %"2", label %"10"
8:
	br label %"9"
9:
	br i1 1, label %"13", label %"14"
13:
        br label %"10"
14:
        br label %"10"
10:
        br label %"8"
3:
	br label %"4"
4:
	ret void
}

; CHECK: [0] 0 => <Function Return>
; CHECK-NEXT:   [1] 7 => 3
; CHECK-NEXT:     [2] 2 => 10
; CHECK-NEXT:       [3] 5 => 6

; STAT: 4 region - The # of regions
; STAT: 2 region - The # of simple regions

; BBIT:      Region tree:
; BBIT-NEXT: [0] 0 => <Function Return>
; BBIT-NEXT: {
; BBIT-NEXT:   0, 7, 1, 2, 5, 11, 6, 10, 8, 9, 13, 14, 12, 3, 4,
; BBIT-NEXT:   [1] 7 => 3
; BBIT-NEXT:   {
; BBIT-NEXT:     7, 1, 2, 5, 11, 6, 10, 8, 9, 13, 14, 12,
; BBIT-NEXT:     [2] 2 => 10
; BBIT-NEXT:     {
; BBIT-NEXT:       2, 5, 11, 6, 12,
; BBIT-NEXT:       [3] 5 => 6
; BBIT-NEXT:       {
; BBIT-NEXT:         5, 11, 12,
; BBIT-NEXT:       }
; BBIT-NEXT:     }
; BBIT-NEXT:   }
; BBIT-NEXT: }
; BBIT-NEXT: End region tree


; RNIT:      Region tree:
; RNIT-NEXT: [0] 0 => <Function Return>
; RNIT-NEXT: {
; RNIT-NEXT:   0, 7 => 3, 3, 4,
; RNIT-NEXT:   [1] 7 => 3
; RNIT-NEXT:   {
; RNIT-NEXT:     7, 1, 2 => 10, 10, 8, 9, 13, 14,
; RNIT-NEXT:     [2] 2 => 10
; RNIT-NEXT:     {
; RNIT-NEXT:       2, 5 => 6, 6,
; RNIT-NEXT:       [3] 5 => 6
; RNIT-NEXT:       {
; RNIT-NEXT:         5, 11, 12,
; RNIT-NEXT:       }
; RNIT-NEXT:     }
; RNIT-NEXT:   }
; RNIT-NEXT: }
; RNIT-NEXT: End region tree

