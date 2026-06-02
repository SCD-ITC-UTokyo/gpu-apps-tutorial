      module SOLVER11

      contains
        subroutine SOLVE11

        use pfem_util
        use solver_CG
        implicit none

C        integer :: ERROR, ICFLAG
C        character(len=char_length) :: BUF
C        data ICFLAG/0/
        integer :: ERROR

!C
!C +------------+
!C | PARAMETERs |
!C +------------+
!C===
        ITER      = pfemIarray(1)
        RESID     = pfemRarray(1)
!C===

!C
!C +------------------+
!C | ITERATIVE solver |
!C +------------------+
!C===
        call CG                                                         &
     &  ( NP, NPLU, D, AMAT, indexLU, itemLU, B, X, RESID, ITER, ERROR )

      ITERactual= ITER
!C===

      end subroutine SOLVE11
      end module SOLVER11
