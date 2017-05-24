define void  @patatino() {
  ret void
}

!llvm.asan.globals = !{!0, !2, !4, !6, !8, !10, !12}

!0  =  distinct !{!1}
!1  = !{i8 1}
!2  =  distinct !{!3}
!3  = !{i8 2}
!4  =  distinct !{!5}
!5  = !{i8 3}
!6  =  distinct !{!7}
!7  = !{i8 4,  i8 5}
!8  =  distinct !{!9}
!9  = !{i8 6,  i8 7}
!10 =  distinct !{!11}
!11 = !{i8 8,  i8 9}
!12 =  distinct !{!13, i8 0}
!13 = !{i8 10, i8 11}
