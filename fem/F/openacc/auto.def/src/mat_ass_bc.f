!C
!C***
!C*** MAT_ASS_BC
!C***
!C
      subroutine MAT_ASS_BC
      use pfem_util
      implicit none
      integer :: k,in,ib,ib0
      integer(kind=kint), dimension(:,:),allocatable :: IWKX

      allocate (IWKX(NP,2))
      IWKX= 0

!C
!C== Z=Zmax

!$omp parallel do private(in)
      do in= 1, NP
        IWKX(in,1)= 0
      enddo

      ib0= -1
      do ib0= 1, NODGRPtot
        if (NODGRP_NAME(ib0).eq.'Zmax') exit
      enddo

!$omp parallel do private(ib,in)      
      do ib= NODGRP_INDEX(ib0-1)+1, NODGRP_INDEX(ib0)
        in= NODGRP_ITEM(ib)
        IWKX(in,1)= 1
      enddo

!$acc parallel loop
!$acc& private(in,k)
      do in= 1, NP
        if (IWKX(in,1).eq.1) then
          B(in)= 0.d0
          D(in)= 1.d0
          do k= indexLU(in-1)+1, indexLU(in)
            AMAT(k)= 0.d0
          enddo
        endif
      enddo

!$acc parallel loop
!$acc& private(in,k)
      do in= 1, NP
        do k=indexLU(in-1)+1,indexLU(in)
          if (IWKX(itemLU(k),1).eq.1) then
            AMAT(k)= 0.d0
          endif
        enddo
      enddo
!C==

      deallocate(IWKX)
      return
      end
