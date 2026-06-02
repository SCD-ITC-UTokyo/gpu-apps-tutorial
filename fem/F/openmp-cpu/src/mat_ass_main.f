!C
!C***
!C*** JACOBI
!C*** 
!C
      subroutine JACOBI (DETJ, PNQ, PNE, PNT, PNX, PNY, PNZ,
     &  X1, X2, X3, X4, X5, X6, X7, X8, Y1, Y2, Y3, Y4, Y5, Y6, Y7, Y8,
     &  Z1, Z2, Z3, Z4, Z5, Z6, Z7, Z8 )

!C
!C    calculates JACOBIAN & INVERSE JACOBIAN
!C             dNi/dx, dNi/dy & dNi/dz         
!C 
      use precision
      implicit none
      real(kind=kreal), intent(out) :: DETJ(2,2,2)
      real(kind=kreal), intent(in)  :: PNQ(2,2,8)
      real(kind=kreal), intent(in)  :: PNE(2,2,8)
      real(kind=kreal), intent(in)  :: PNT(2,2,8)
      real(kind=kreal), intent(out) :: PNX(2,2,2,8)
      real(kind=kreal), intent(out) :: PNY(2,2,2,8)
      real(kind=kreal), intent(out) :: PNZ(2,2,2,8)
      real(kind=kreal), intent(in)  :: X1, X2, X3, X4, X5, X6, X7, X8
      real(kind=kreal), intent(in)  :: Y1, Y2, Y3, Y4, Y5, Y6, Y7, Y8
      real(kind=kreal), intent(in)  :: Z1, Z2, Z3, Z4, Z5, Z6, Z7, Z8

      integer(kind=kint) :: ip,jp,kp
      real(kind=kreal)   :: dXdQ,dYdQ,dZdQ,dXdE,dYdE,dZdE,dXdT,dYdT,dZdT
      real(kind=kreal)   :: coef
      real(kind=kreal)   :: a11,a12,a13,a21,a22,a23,a31,a32,a33

      do kp= 1, 2
      do jp= 1, 2
      do ip= 1, 2
        PNX(ip,jp,kp,1)=0.d0
        PNX(ip,jp,kp,2)=0.d0
        PNX(ip,jp,kp,3)=0.d0
        PNX(ip,jp,kp,4)=0.d0
        PNX(ip,jp,kp,5)=0.d0
        PNX(ip,jp,kp,6)=0.d0
        PNX(ip,jp,kp,7)=0.d0
        PNX(ip,jp,kp,8)=0.d0

        PNY(ip,jp,kp,1)=0.d0
        PNY(ip,jp,kp,2)=0.d0
        PNY(ip,jp,kp,3)=0.d0
        PNY(ip,jp,kp,4)=0.d0
        PNY(ip,jp,kp,5)=0.d0
        PNY(ip,jp,kp,6)=0.d0
        PNY(ip,jp,kp,7)=0.d0
        PNY(ip,jp,kp,8)=0.d0

        PNZ(ip,jp,kp,1)=0.d0
        PNZ(ip,jp,kp,2)=0.d0
        PNZ(ip,jp,kp,3)=0.d0
        PNZ(ip,jp,kp,4)=0.d0
        PNZ(ip,jp,kp,5)=0.d0
        PNZ(ip,jp,kp,6)=0.d0
        PNZ(ip,jp,kp,7)=0.d0
        PNZ(ip,jp,kp,8)=0.d0

!C     
!C==   DETERMINANT of the JACOBIAN
        dXdQ =                                                          &
     &           + PNQ(jp,kp,1) * X1 + PNQ(jp,kp,2) * X2                &
     &           + PNQ(jp,kp,3) * X3 + PNQ(jp,kp,4) * X4                &
     &           + PNQ(jp,kp,5) * X5 + PNQ(jp,kp,6) * X6                &
     &           + PNQ(jp,kp,7) * X7 + PNQ(jp,kp,8) * X8                
        dYdQ =                                                          &
     &           + PNQ(jp,kp,1) * Y1 + PNQ(jp,kp,2) * Y2                &
     &           + PNQ(jp,kp,3) * Y3 + PNQ(jp,kp,4) * Y4                &
     &           + PNQ(jp,kp,5) * Y5 + PNQ(jp,kp,6) * Y6                &
     &           + PNQ(jp,kp,7) * Y7 + PNQ(jp,kp,8) * Y8                
        dZdQ =                                                          &
     &           + PNQ(jp,kp,1) * Z1 + PNQ(jp,kp,2) * Z2                &
     &           + PNQ(jp,kp,3) * Z3 + PNQ(jp,kp,4) * Z4                &
     &           + PNQ(jp,kp,5) * Z5 + PNQ(jp,kp,6) * Z6                &
     &           + PNQ(jp,kp,7) * Z7 + PNQ(jp,kp,8) * Z8                
        dXdE =                                                          &
     &           + PNE(ip,kp,1) * X1 + PNE(ip,kp,2) * X2                &
     &           + PNE(ip,kp,3) * X3 + PNE(ip,kp,4) * X4                &
     &           + PNE(ip,kp,5) * X5 + PNE(ip,kp,6) * X6                &
     &           + PNE(ip,kp,7) * X7 + PNE(ip,kp,8) * X8
        dYdE =                                                          &
     &           + PNE(ip,kp,1) * Y1 + PNE(ip,kp,2) * Y2                &
     &           + PNE(ip,kp,3) * Y3 + PNE(ip,kp,4) * Y4                &
     &           + PNE(ip,kp,5) * Y5 + PNE(ip,kp,6) * Y6                &
     &           + PNE(ip,kp,7) * Y7 + PNE(ip,kp,8) * Y8
        dZdE =                                                          &
     &           + PNE(ip,kp,1) * Z1 + PNE(ip,kp,2) * Z2                &
     &           + PNE(ip,kp,3) * Z3 + PNE(ip,kp,4) * Z4                &
     &           + PNE(ip,kp,5) * Z5 + PNE(ip,kp,6) * Z6                &
     &           + PNE(ip,kp,7) * Z7 + PNE(ip,kp,8) * Z8
        dXdT =                                                          &
     &           + PNT(ip,jp,1) * X1 + PNT(ip,jp,2) * X2                &
     &           + PNT(ip,jp,3) * X3 + PNT(ip,jp,4) * X4                &
     &           + PNT(ip,jp,5) * X5 + PNT(ip,jp,6) * X6                &
     &           + PNT(ip,jp,7) * X7 + PNT(ip,jp,8) * X8
        dYdT =                                                          &
     &           + PNT(ip,jp,1) * Y1 + PNT(ip,jp,2) * Y2                &
     &           + PNT(ip,jp,3) * Y3 + PNT(ip,jp,4) * Y4                &
     &           + PNT(ip,jp,5) * Y5 + PNT(ip,jp,6) * Y6                &
     &           + PNT(ip,jp,7) * Y7 + PNT(ip,jp,8) * Y8
        dZdT =                                                          &
     &           + PNT(ip,jp,1) * Z1 + PNT(ip,jp,2) * Z2                &
     &           + PNT(ip,jp,3) * Z3 + PNT(ip,jp,4) * Z4                &
     &           + PNT(ip,jp,5) * Z5 + PNT(ip,jp,6) * Z6                &
     &           + PNT(ip,jp,7) * Z7 + PNT(ip,jp,8) * Z8

        DETJ(ip,jp,kp)= dXdQ*(dYdE*dZdT-dZdE*dYdT) +                    &
     &                  dYdQ*(dZdE*dXdT-dXdE*dZdT) +                    &
     &                  dZdQ*(dXdE*dYdT-dYdE*dXdT)

!C
!C==   INVERSE JACOBIAN
        coef= 1.d0 / DETJ(ip,jp,kp)
        a11= coef * ( dYdE*dZdT - dZdE*dYdT )
        a12= coef * ( dZdQ*dYdT - dYdQ*dZdT )
        a13= coef * ( dYdQ*dZdE - dZdQ*dYdE )

        a21= coef * ( dZdE*dXdT - dXdE*dZdT )
        a22= coef * ( dXdQ*dZdT - dZdQ*dXdT )
        a23= coef * ( dZdQ*dXdE - dXdQ*dZdE )

        a31= coef * ( dXdE*dYdT - dYdE*dXdT )
        a32= coef * ( dYdQ*dXdT - dXdQ*dYdT )
        a33= coef * ( dXdQ*dYdE - dYdQ*dXdE )

        DETJ(ip,jp,kp)= abs(DETJ(ip,jp,kp))

!C
!C== set the dNi/dX, dNi/dY & dNi/dZ components
        PNX(ip,jp,kp,1)= a11*PNQ(jp,kp,1) + a12*PNE(ip,kp,1) +          &
     &                   a13*PNT(ip,jp,1)
        PNX(ip,jp,kp,2)= a11*PNQ(jp,kp,2) + a12*PNE(ip,kp,2) +          &
     &                   a13*PNT(ip,jp,2)
        PNX(ip,jp,kp,3)= a11*PNQ(jp,kp,3) + a12*PNE(ip,kp,3) +          &
     &                   a13*PNT(ip,jp,3)
        PNX(ip,jp,kp,4)= a11*PNQ(jp,kp,4) + a12*PNE(ip,kp,4) +          &
     &                   a13*PNT(ip,jp,4)
        PNX(ip,jp,kp,5)= a11*PNQ(jp,kp,5) + a12*PNE(ip,kp,5) +          &
     &                   a13*PNT(ip,jp,5)
        PNX(ip,jp,kp,6)= a11*PNQ(jp,kp,6) + a12*PNE(ip,kp,6) +          &
     &                   a13*PNT(ip,jp,6)
        PNX(ip,jp,kp,7)= a11*PNQ(jp,kp,7) + a12*PNE(ip,kp,7) +          &
     &                   a13*PNT(ip,jp,7)
        PNX(ip,jp,kp,8)= a11*PNQ(jp,kp,8) + a12*PNE(ip,kp,8) +          &
     &                   a13*PNT(ip,jp,8)

        PNY(ip,jp,kp,1)= a21*PNQ(jp,kp,1) + a22*PNE(ip,kp,1) +          &
     &                   a23*PNT(ip,jp,1)
        PNY(ip,jp,kp,2)= a21*PNQ(jp,kp,2) + a22*PNE(ip,kp,2) +          &
     &                   a23*PNT(ip,jp,2)
        PNY(ip,jp,kp,3)= a21*PNQ(jp,kp,3) + a22*PNE(ip,kp,3) +          &
     &                   a23*PNT(ip,jp,3)
        PNY(ip,jp,kp,4)= a21*PNQ(jp,kp,4) + a22*PNE(ip,kp,4) +          &
     &                   a23*PNT(ip,jp,4)
        PNY(ip,jp,kp,5)= a21*PNQ(jp,kp,5) + a22*PNE(ip,kp,5) +          &
     &                   a23*PNT(ip,jp,5)
        PNY(ip,jp,kp,6)= a21*PNQ(jp,kp,6) + a22*PNE(ip,kp,6) +          &
     &                   a23*PNT(ip,jp,6)
        PNY(ip,jp,kp,7)= a21*PNQ(jp,kp,7) + a22*PNE(ip,kp,7) +          &
     &                   a23*PNT(ip,jp,7)
        PNY(ip,jp,kp,8)= a21*PNQ(jp,kp,8) + a22*PNE(ip,kp,8) +          &
     &                   a23*PNT(ip,jp,8)

        PNZ(ip,jp,kp,1)= a31*PNQ(jp,kp,1) + a32*PNE(ip,kp,1) +          &
     &                   a33*PNT(ip,jp,1)
        PNZ(ip,jp,kp,2)= a31*PNQ(jp,kp,2) + a32*PNE(ip,kp,2) +          &
     &                   a33*PNT(ip,jp,2)
        PNZ(ip,jp,kp,3)= a31*PNQ(jp,kp,3) + a32*PNE(ip,kp,3) +          &
     &                   a33*PNT(ip,jp,3)
        PNZ(ip,jp,kp,4)= a31*PNQ(jp,kp,4) + a32*PNE(ip,kp,4) +          &
     &                   a33*PNT(ip,jp,4)
        PNZ(ip,jp,kp,5)= a31*PNQ(jp,kp,5) + a32*PNE(ip,kp,5) +          &
     &                   a33*PNT(ip,jp,5)
        PNZ(ip,jp,kp,6)= a31*PNQ(jp,kp,6) + a32*PNE(ip,kp,6) +          &
     &                   a33*PNT(ip,jp,6)
        PNZ(ip,jp,kp,7)= a31*PNQ(jp,kp,7) + a32*PNE(ip,kp,7) +          &
     &                   a33*PNT(ip,jp,7)
        PNZ(ip,jp,kp,8)= a31*PNQ(jp,kp,8) + a32*PNE(ip,kp,8) +          &
     &                   a33*PNT(ip,jp,8)

      enddo
      enddo
      enddo

      return
      end

!     C
!     C***
!     C*** MAT_ASS_MAIN
!     C***
!     C
      subroutine MAT_ASS_MAIN
      use pfem_util
      implicit none

      integer(kind=kint) :: i,k,kk
      integer(kind=kint) :: ip,jp,kp
      integer(kind=kint) :: ipn,jpn,kpn
      integer(kind=kint) :: icel,icou,icol,icel0
      integer(kind=kint) :: ie,je
      integer(kind=kint) :: iiS,iiE
      integer(kind=kint) :: in1,in2,in3,in4,in5,in6,in7,in8
      integer(kind=kint) :: ip1,ip2,ip3,ip4,ip5,ip6,ip7,ip8,isum
      real(kind=kreal)   :: SHi
      real(kind=kreal)   :: QP1,QM1,EP1,EM1,TP1,TM1
      real(kind=kreal)   :: X1,X2,X3,X4,X5,X6,X7,X8
      real(kind=kreal)   :: Y1,Y2,Y3,Y4,Y5,Y6,Y7,Y8
      real(kind=kreal)   :: Z1,Z2,Z3,Z4,Z5,Z6,Z7,Z8
      real(kind=kreal)   :: PNXi,PNYi,PNZi,PNXj,PNYj,PNZj
      real(kind=kreal)   :: QV0, QVC, COEFij
      real(kind=kreal)   :: coef
      
      integer(kind=kint), dimension(  8) :: nodLOCAL
      integer(kind=kint), dimension(:,:),allocatable :: IWKX
      integer(kind=kint), dimension(:),allocatable   :: ELMCOLORindex
      integer(kind=kint), dimension(:),allocatable   :: ELMCOLORitem

!     C
!     C +------------------+
!     C | ELEMENT Coloring |
!     C +------------------+
!     C===
      allocate (ELMCOLORindex(0:NP))
      allocate (ELMCOLORitem (ICELTOT))

      allocate (IWKX(0:NP,3))

      IWKX= 0
      icou= 0
      do icol= 1, NP
         do i= 1, NP
            IWKX(i,1)= 0
         enddo
         do icel= 1, ICELTOT
            if (IWKX(icel,2).eq.0) then
               in1= ICELNOD(icel,1)
               in2= ICELNOD(icel,2)
               in3= ICELNOD(icel,3)
               in4= ICELNOD(icel,4)
               in5= ICELNOD(icel,5)
               in6= ICELNOD(icel,6)
               in7= ICELNOD(icel,7)
               in8= ICELNOD(icel,8)

               ip1= IWKX(in1,1)
               ip2= IWKX(in2,1)
               ip3= IWKX(in3,1)
               ip4= IWKX(in4,1)
               ip5= IWKX(in5,1)
               ip6= IWKX(in6,1)
               ip7= IWKX(in7,1)
               ip8= IWKX(in8,1)

               isum= ip1 + ip2 + ip3 + ip4 + ip5 + ip6 + ip7 + ip8
               if (isum.eq.0) then 
                  icou= icou + 1
                  IWKX(icol,3)= icou
                  IWKX(icel,2)= icol
                  ELMCOLORitem(icou)= icel

                  IWKX(in1,1)= 1
                  IWKX(in2,1)= 1
                  IWKX(in3,1)= 1
                  IWKX(in4,1)= 1
                  IWKX(in5,1)= 1
                  IWKX(in6,1)= 1
                  IWKX(in7,1)= 1
                  IWKX(in8,1)= 1
                  if (icou.eq.ICELTOT) goto 100            
               endif
            endif
         enddo
      enddo

 100  continue
      ELMCOLORtot= icol
      IWKX(0          ,3)= 0
      IWKX(ELMCOLORtot,3)= ICELTOT

      do icol= 0, ELMCOLORtot
         ELMCOLORindex(icol)= IWKX(icol,3)
      enddo

!     write (*,'(a,2i8)') '### Number of Element Colors', 
!     &                     my_rank, ELMCOLORtot

!     C===
      
      allocate(AMAT(NPLU),B(NP),D(NP),X(NP))
      
      AMAT= 0.d0
      B= 0.d0
      X= 0.d0
      D= 0.d0

      WEI(1)= +1.0000000000D+00
      WEI(2)= +1.0000000000D+00

      POS(1)= -0.5773502692D+00
      POS(2)= +0.5773502692D+00

!     C
!     C-- INIT.
!     C     PNQ   - 1st-order derivative of shape function by QSI
!     C     PNE   - 1st-order derivative of shape function by ETA
!     C     PNT   - 1st-order derivative of shape function by ZET
!     C
      do kp= 1, 2
         do jp= 1, 2
            do ip= 1, 2
               QP1= 1.d0 + POS(ip)
               QM1= 1.d0 - POS(ip)
               EP1= 1.d0 + POS(jp)
               EM1= 1.d0 - POS(jp)
               TP1= 1.d0 + POS(kp)
               TM1= 1.d0 - POS(kp)
               SHAPE(ip,jp,kp,1)= O8th * QM1 * EM1 * TM1
               SHAPE(ip,jp,kp,2)= O8th * QP1 * EM1 * TM1
               SHAPE(ip,jp,kp,3)= O8th * QP1 * EP1 * TM1
               SHAPE(ip,jp,kp,4)= O8th * QM1 * EP1 * TM1
               SHAPE(ip,jp,kp,5)= O8th * QM1 * EM1 * TP1
               SHAPE(ip,jp,kp,6)= O8th * QP1 * EM1 * TP1
               SHAPE(ip,jp,kp,7)= O8th * QP1 * EP1 * TP1
               SHAPE(ip,jp,kp,8)= O8th * QM1 * EP1 * TP1
               PNQ(jp,kp,1)= - O8th * EM1 * TM1
               PNQ(jp,kp,2)= + O8th * EM1 * TM1
               PNQ(jp,kp,3)= + O8th * EP1 * TM1
               PNQ(jp,kp,4)= - O8th * EP1 * TM1
               PNQ(jp,kp,5)= - O8th * EM1 * TP1
               PNQ(jp,kp,6)= + O8th * EM1 * TP1
               PNQ(jp,kp,7)= + O8th * EP1 * TP1
               PNQ(jp,kp,8)= - O8th * EP1 * TP1
               PNE(ip,kp,1)= - O8th * QM1 * TM1
               PNE(ip,kp,2)= - O8th * QP1 * TM1
               PNE(ip,kp,3)= + O8th * QP1 * TM1
               PNE(ip,kp,4)= + O8th * QM1 * TM1
               PNE(ip,kp,5)= - O8th * QM1 * TP1
               PNE(ip,kp,6)= - O8th * QP1 * TP1
               PNE(ip,kp,7)= + O8th * QP1 * TP1
               PNE(ip,kp,8)= + O8th * QM1 * TP1
               PNT(ip,jp,1)= - O8th * QM1 * EM1
               PNT(ip,jp,2)= - O8th * QP1 * EM1
               PNT(ip,jp,3)= - O8th * QP1 * EP1
               PNT(ip,jp,4)= - O8th * QM1 * EP1
               PNT(ip,jp,5)= + O8th * QM1 * EM1
               PNT(ip,jp,6)= + O8th * QP1 * EM1
               PNT(ip,jp,7)= + O8th * QP1 * EP1
               PNT(ip,jp,8)= + O8th * QM1 * EP1
            enddo
         enddo
      enddo

      do icol= 1, ELMCOLORtot
!$omp parallel do
!$omp& private (icel0,icel,in1,in2,in3,in4,in5,in6,in7,in8,nodLOCAL)
!$omp& private (X1,X2,X3,X4,X5,X6,X7,X8)
!$omp& private (Y1,Y2,Y3,Y4,Y5,Y6,Y7,Y8)
!$omp& private (Z1,Z2,Z3,Z4,Z5,Z6,Z7,Z8)
!$omp& private (QVC,DETJ,PNX,PNY,PNZ)
!$omp& private (ie,je,ip,jp,kk,iiS,iiE,k)
!$omp& private (QV0,COEFij,ipn,jpn,kpn,coef)
!$omp& private (PNXi,PNYi,PNZi,PNXj,PNYj,PNZj,SHi)
         do icel0= ELMCOLORindex(icol-1)+1, ELMCOLORindex(icol)
            icel= ELMCOLORitem(icel0)

            in1= ICELNOD(icel,1)
            in2= ICELNOD(icel,2)
            in3= ICELNOD(icel,3)
            in4= ICELNOD(icel,4)
            in5= ICELNOD(icel,5)
            in6= ICELNOD(icel,6)
            in7= ICELNOD(icel,7)
            in8= ICELNOD(icel,8)
!     C
!     C== JACOBIAN & INVERSE JACOBIAN
            nodLOCAL(1)= in1
            nodLOCAL(2)= in2
            nodLOCAL(3)= in3
            nodLOCAL(4)= in4
            nodLOCAL(5)= in5
            nodLOCAL(6)= in6
            nodLOCAL(7)= in7
            nodLOCAL(8)= in8

            X1= XYZ(in1,1)
            X2= XYZ(in2,1)
            X3= XYZ(in3,1)
            X4= XYZ(in4,1)
            X5= XYZ(in5,1)
            X6= XYZ(in6,1)
            X7= XYZ(in7,1)
            X8= XYZ(in8,1)

            Y1= XYZ(in1,2)
            Y2= XYZ(in2,2)
            Y3= XYZ(in3,2)
            Y4= XYZ(in4,2)
            Y5= XYZ(in5,2)
            Y6= XYZ(in6,2)
            Y7= XYZ(in7,2)
            Y8= XYZ(in8,2)

            Z1= XYZ(in1,3)
            Z2= XYZ(in2,3)
            Z3= XYZ(in3,3)
            Z4= XYZ(in4,3)
            Z5= XYZ(in5,3)
            Z6= XYZ(in6,3)
            Z7= XYZ(in7,3)
            Z8= XYZ(in8,3)

            QVC= O8th*(X1+X2+X3+X4+X5+X6+X7+X8+Y1+Y2+Y3+Y4+Y5+Y6+Y7+Y8)

            call JACOBI (DETJ, PNQ, PNE, PNT, PNX, PNY, PNZ,
     &           X1, X2, X3, X4, X5, X6, X7, X8,
     &           Y1, Y2, Y3, Y4, Y5, Y6, Y7, Y8,
     &           Z1, Z2, Z3, Z4, Z5, Z6, Z7, Z8 )
!     C
!     C== CONSTRUCT the GLOBAL MATRIX
            do ie= 1, 8
               ip = nodLOCAL(ie)
               do je= 1, 8
                  jp = nodLOCAL(je)

                  kk= 0
                  if (jp.ne.ip) then
                     iiS= indexLU(ip-1) + 1
                     iiE= indexLU(ip  )
                     do k= iiS, iiE
                        if ( itemLU(k).eq.jp ) then
                           kk= k
                           exit
                        endif
                     enddo
                  endif

                  QV0   = 0.d0
                  COEFij= 0.d0
                  do kpn= 1, 2
                     do jpn= 1, 2
                        do ipn= 1, 2
                           coef= WEI(ipn)*WEI(jpn)*WEI(kpn)

                           PNXi= PNX(ipn,jpn,kpn,ie)
                           PNYi= PNY(ipn,jpn,kpn,ie)
                           PNZi= PNZ(ipn,jpn,kpn,ie)

                           PNXj= PNX(ipn,jpn,kpn,je)
                           PNYj= PNY(ipn,jpn,kpn,je)
                           PNZj= PNZ(ipn,jpn,kpn,je)

                           COEFij= COEFij + coef * COND * 
     &                          (PNXi*PNXj+PNYi*PNYj+PNZi*PNZj) *
     &                          abs(DETJ(ipn,jpn,kpn))
                           SHi= SHAPE(ipn,jpn,kpn,ie)
                           QV0= QV0 + SHi * QVOL * coef *
     &                          abs(DETJ(ipn,jpn,kpn))
                        enddo
                     enddo
                  enddo

                  if (jp.eq.ip) then
                     D(ip)= D(ip) + COEFij
                     B(ip)= B(ip) + QV0*QVC
                  else
                     AMAT(kk)= AMAT(kk) + COEFij
                  endif
               enddo  ! je
            enddo ! ie
         enddo ! icel0
      enddo ! icol
      !pragma omp end parallel do

      deallocate(IWKX,ELMCOLORindex,ELMCOLORitem)

      return
      end

      subroutine MAT_ASS_CLEAR
      use pfem_util
      implicit none

      deallocate(AMAT,B,D,X)
      end
