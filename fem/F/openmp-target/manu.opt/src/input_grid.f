!C
!C***
!C*** INPUT_GRID
!C***
!C
      subroutine INPUT_GRID
      use pfem_util
      implicit none
      integer(kind=kint) :: i,k,ii,kk,nn,icel,iS,iE
      integer(kind=kint) :: NTYPE,IMAT
      
      open (11, file= fname, status= 'unknown', form= 'formatted')

!C
!C-- NEIB-PE

!C
!C-- NODE
      read (11,*)  NP

      allocate (XYZ(NP,3))
      XYZ= 0.d0
        do i= 1, NP
          read (11,*) ii, (XYZ(i,kk),kk=1,3)
        enddo

!C
!C-- ELEMENT
      read (11,*)  ICELTOT
      allocate (ICELNOD(ICELTOT,8))
      read (11,'(10i10)') (NTYPE, i= 1, ICELTOT)

      do icel= 1, ICELTOT
        read (11,'(10i10,2i5,8i8)') ii, IMAT, (ICELNOD(icel,k), k=1,8)
      enddo
!C
!C-- NODE grp. info.
      read (11,'(10i10)') NODGRPtot
      allocate (NODGRP_INDEX(0:NODGRPtot),NODGRP_NAME(NODGRPtot))
      NODGRP_INDEX= 0

      read (11,'(10i10)') (NODGRP_INDEX(i), i= 1, NODGRPtot)
      nn= NODGRP_INDEX(NODGRPtot)
      allocate (NODGRP_ITEM(nn))
      
      do k= 1, NODGRPtot
        iS= NODGRP_INDEX(k-1) + 1
        iE= NODGRP_INDEX(k  )
        read (11,'(a80)') NODGRP_NAME(k)
        nn= iE - iS + 1
        if (nn.ne.0) then
          read (11,'(10i10)') (NODGRP_ITEM(kk),kk=iS, iE)
        endif
      enddo

      close (11)

      return
      end
