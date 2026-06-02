!C
!C***
!C*** INPUT_CNTL
!C***
!C
      subroutine INPUT_CNTL( input )
      use pfem_util
      implicit none

      character(*), intent(in) :: input

        open (11,file=input, status='unknown')
        read (11,'(a80)') fname
        read (11,*) ITER
        read (11,*) COND, QVOL
        read (11,*) RESID
        close (11)

      pfemIarray(1)= ITER
      pfemRarray(1)= RESID

      return
      end
