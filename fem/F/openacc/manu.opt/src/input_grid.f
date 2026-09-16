!C
!C***
!C*** INPUT_GRID
!C***
!C  1 辺 nxn 節点の立方体メッシュ (8 節点六面体 1 次要素) を内部生成する。
!C  旧版はメッシュファイル cube.0 を読んでいたが、格子が完全に規則的で
!C  nxn 一つから決まるため、diffusion / nbody と同様にファイル不要にした。
!C  生成されるデータは旧 cube.0 (nxn=65) と完全に同一。
!C
!C    節点番号 (1 origin): n0 = 1 + i + j*nxn + k*nxn*nxn   (x が最内)
!C    節点座標            : (i, j, k)  格子間隔 1.0
!C    要素結合            : 下面 4 節点 (反時計回り) -> 上面 4 節点
!C    節点グループ        : Xmin(i=0)/Ymin(j=0)/Zmin(k=0)/Zmax(k=nxn-1)
!C                          MAT_ASS_BC が使うのは Zmax のみ
!C
      subroutine INPUT_GRID (nxn)
      use pfem_util
      implicit none
      integer(kind=kint), intent(in) :: nxn
      integer(kind=kint) :: i,j,k,icel,ib,n0
      integer(kind=kint) :: nxe, nn

      if (nxn.lt.2) then
        write (*,'("N must be 2 or larger (given: ",i8,")")') nxn
        stop
      endif

      nxe= nxn - 1
      nn = nxn * nxn

!C
!C-- NODE
      NP= nxn * nxn * nxn

      allocate (XYZ(NP,3))
      XYZ= 0.d0

      do k= 0, nxn-1
      do j= 0, nxn-1
      do i= 0, nxn-1
        n0= 1 + i + j*nxn + k*nn
        XYZ(n0,1)= dble(i)
        XYZ(n0,2)= dble(j)
        XYZ(n0,3)= dble(k)
      enddo
      enddo
      enddo

!C
!C-- ELEMENT
      ICELTOT= nxe * nxe * nxe
      allocate (ICELNOD(ICELTOT,8))

      icel= 0
      do k= 0, nxe-1
      do j= 0, nxe-1
      do i= 0, nxe-1
        icel= icel + 1
        n0= 1 + i + j*nxn + k*nn
        ICELNOD(icel,1)= n0
        ICELNOD(icel,2)= n0 + 1
        ICELNOD(icel,3)= n0 + 1 + nxn
        ICELNOD(icel,4)= n0     + nxn
        ICELNOD(icel,5)= n0           + nn
        ICELNOD(icel,6)= n0 + 1       + nn
        ICELNOD(icel,7)= n0 + 1 + nxn + nn
        ICELNOD(icel,8)= n0     + nxn + nn
      enddo
      enddo
      enddo

!C
!C-- NODE grp. info.
      NODGRPtot= 4
      allocate (NODGRP_INDEX(0:NODGRPtot),NODGRP_NAME(NODGRPtot))

      do i= 0, NODGRPtot
        NODGRP_INDEX(i)= i * nn
      enddo

      allocate (NODGRP_ITEM(NODGRP_INDEX(NODGRPtot)))

      NODGRP_NAME(1)= 'Xmin'
      NODGRP_NAME(2)= 'Ymin'
      NODGRP_NAME(3)= 'Zmin'
      NODGRP_NAME(4)= 'Zmax'

      ib= 0
      do k= 0, nxn-1
      do j= 0, nxn-1
        ib= ib + 1
        NODGRP_ITEM(ib)= 1 + j*nxn + k*nn
      enddo
      enddo

      do k= 0, nxn-1
      do i= 0, nxn-1
        ib= ib + 1
        NODGRP_ITEM(ib)= 1 + i + k*nn
      enddo
      enddo

      do j= 0, nxn-1
      do i= 0, nxn-1
        ib= ib + 1
        NODGRP_ITEM(ib)= 1 + i + j*nxn
      enddo
      enddo

      do j= 0, nxn-1
      do i= 0, nxn-1
        ib= ib + 1
        NODGRP_ITEM(ib)= 1 + i + j*nxn + (nxn-1)*nn
      enddo
      enddo

      return
      end
