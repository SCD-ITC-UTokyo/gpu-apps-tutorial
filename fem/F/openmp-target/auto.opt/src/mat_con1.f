!C
!C***
!C*** MAT_CON1
!C***
!C
      subroutine MAT_CON1
      use pfem_util
      implicit none

      integer(kind=kint) :: i,k,kk

      allocate (indexLU(0:NP))
      indexLU= 0

      do i= 1, NP
        indexLU(i)= indexLU(i-1) + INLU(i)
      enddo

      NPLU= indexLU(NP)

      allocate (itemLU(NPLU))

      do i= 1, NP
        do k= 1, INLU(i)
           kk = k + indexLU(i-1)
          itemLU(kk)=      IALU(i,k)
        enddo
      enddo

      deallocate (INLU, IALU)

      end subroutine MAT_CON1
