!C
!C*** 
!C*** module solver_CG
!C***
!C
      module solver_CG
      use pfem_util
      contains
!C
!C*** CG
!C
!C    CG solves the linear system Ax = b using the Conjugate Gradient 
!C    iterative method with the following preconditioners
!C
      subroutine CG                                                     &
     &   (NP, NPLU, D, AMAT, indexLU, itemLU, B, X, RESID, ITER, ERROR)

      use precision
      implicit none

      integer(kind=kint ), intent(in):: NP, NPLU

      integer(kind=kint ), intent(inout):: ITER, ERROR
      real   (kind=kreal), intent(inout):: RESID

      real   (kind=kreal), dimension(NP)  , intent(inout):: B, X, D
      real   (kind=kreal), dimension(NPLU), intent(inout):: AMAT

      integer(kind=kint ), dimension(0:NP),intent(in) :: indexLU
      integer(kind=kint ), dimension(NPLU),intent(in) :: itemLU

      integer(kind=kint ) :: i,j,k
      real   (kind=kreal) :: ALPHA,BETA
      real   (kind=kreal) :: BNRM2,DNRM2
      real   (kind=kreal) :: WVAL
      real   (kind=kreal) :: C1,RHO,RHO1
      
c$$$      real   (kind=kreal), dimension(:,:),  allocatable       :: WW
c$$$
c$$$      integer(kind=kint), parameter ::  R= 1
c$$$      integer(kind=kint), parameter ::  Z= 2
c$$$      integer(kind=kint), parameter ::  Q= 2
c$$$      integer(kind=kint), parameter ::  P= 3
c$$$      integer(kind=kint), parameter :: DD= 4
      real   (kind=kreal), dimension(:), allocatable :: RW,ZW,QW,PW,DW

      integer(kind=kint ) :: MAXIT
      real   (kind=kreal) :: TOL

!C
!C +-------+
!C | INIT. |
!C +-------+
!C===
      ERROR= 0

c$$$      allocate (WW(N,4))
      allocate (RW(NP),ZW(NP),QW(NP),PW(NP),DW(NP))

      MAXIT  = ITER
      TOL    = RESID           

!$omp target teams distribute parallel do
!$omp& private(i)
      do i= 1, NP
        X(i)= 0.d0
c$$$  W(i,:)= 0.d0
        RW(i)=0.d0
        ZW(i)=0.d0
        QW(i)=0.d0
        PW(i)=0.d0
        DW(i)=0.d0
      enddo
!C===

!C
!C +-----------------------+
!C | {r0}= {b} - [A]{xini} |
!C +-----------------------+
!C===

!$omp target teams distribute parallel do
!$omp& private(i,j,WVAL)
      do i= 1, NP
c$$$    WW(i,DD)= 1.d0/D(i)
        DW(i)= 1.d0/D(i)
        WVAL= B(i) - D(i)*X(i)
        do j= indexLU(i-1)+1, indexLU(i)
          WVAL= WVAL - AMAT(j)*X(itemLU(j))
        enddo
c$$$    WW(i,R)= WVAL
        RW(i)= WVAL
      enddo

      BNRM2= 0.d0
!$omp target teams distribute parallel do
!$omp& private(i) reduction(+:BNRM2)
      do i= 1, NP
        BNRM2= BNRM2 + B(i)**2
      enddo
      ITER = 0
!C===

      do iter= 1, MAXIT
!C
!C************************************************* Conjugate Gradient Iteration

!C
!C +----------------+
!C | {z}= [Minv]{r} |
!C +----------------+
!C===

!$omp target teams distribute parallel do
!$omp& private(i)
      do i= 1, NP
c$$$    WW(i,Z)= WW(i,R) * WW(i,DD)
        ZW(i)= RW(i) * DW(i)
      enddo
!C===
      
!C
!C +---------------+
!C | {RHO}= {r}{z} |
!C +---------------+
!C===
      RHO= 0.d0
!$omp target teams distribute parallel do
!$omp& private(i) reduction(+:RHO)
      do i= 1, NP
c$$$    RHO= RHO + WW(i,R)*WW(i,Z)
        RHO= RHO + RW(i)*ZW(i)
      enddo
!C===
!C
!C +-----------------------------+
!C | {p} = {z} if      ITER=1    |
!C | BETA= RHO / RHO1  otherwise |
!C +-----------------------------+
!C===
      if ( ITER.eq.1 ) then
!$omp target teams distribute parallel do
!$omp& private(i)
        do i= 1, NP
c$$$      WW(i,P)= WW(i,Z)
          PW(i)= ZW(i)
        enddo
      else
        BETA= RHO / RHO1
!$omp target teams distribute parallel do
!$omp& private(i)
         do i= 1, NP
c$$$       WW(i,P)= WW(i,Z) + BETA*WW(i,P)
           PW(i)= ZW(i) + BETA*PW(i)
         enddo
      endif
!C===

!C
!C +-------------+
!C | {q}= [A]{p} |
!C +-------------+
!C===   

!$omp target teams distribute parallel do
!$omp& private(i,j,WVAL)
      do i= 1, NP
c$$$    WVAL= D(i)*WW(i,P)
        WVAL= D(i)*PW(i)
        do j= indexLU(i-1)+1, indexLU(i)
c$$$      WVAL= WVAL + AMAT(j)*WW(itemLU(j),P)
          WVAL= WVAL + AMAT(j)*PW(itemLU(j))
        enddo
c$$$    WW(i,Q)= WVAL
        QW(i)= WVAL
      enddo
!C===

!C
!C +---------------------+
!C | ALPHA= RHO / {p}{q} |
!C +---------------------+
!C===
      C1= 0.d0
!$omp target teams distribute parallel do
!$omp& private(i) reduction(+:C1)
      do i= 1, NP
c$$$    C1= C1 + WW(i,P)*WW(i,Q)
        C1= C1 + PW(i)*QW(i)
      enddo
      ALPHA= RHO / C1
!C===

!C
!C +----------------------+
!C | {x}= {x} + ALPHA*{p} |
!C | {r}= {r} - ALPHA*{q} |
!C +----------------------+
!C===

!$omp target teams distribute parallel do
!$omp& private(i)
      do i= 1, NP
c$$$     X(i)  = X (i)   + ALPHA * WW(i,P)
         X(i)  = X (i)   + ALPHA * PW(i)
c$$$    WW(i,R)= WW(i,R) - ALPHA * WW(i,Q)
        RW(i)= RW(i) - ALPHA * QW(i)
      enddo

      DNRM2= 0.d0
!$omp target teams distribute parallel do
!$omp& private(i) reduction(+:DNRM2)
      do i= 1, NP
c$$$        DNRM2= DNRM2 + WW(i,R)**2
        DNRM2= DNRM2 + RW(i)**2
      enddo
      RESID= sqrt(DNRM2/BNRM2)

!C##### ITERATION HISTORY
#ifndef BENCHMARK_MODE
      write (*, 1000) ITER, RESID
 1000 format (i5, 1pe16.6)
#endif
! 1010   format (1pe16.6)
!C#####

      if ( RESID.le.TOL   ) exit
      if ( ITER .eq.MAXIT ) ERROR= -300

      RHO1 = RHO                                                             
      enddo
!C===

!C
!C-- INTERFACE data EXCHANGE
   30 continue

c$$$  deallocate (WW)
      deallocate (RW,ZW,QW,PW,DW)

      FLOP = dble(ITER)*(NP*14 + NPLU*2) + NP*3 + NPLU*2

!C グローバル ITERactual に実反復回数を保存 (CG パラメータの ITER がローカル shadow なので必要)
      ITERactual = ITER

      end subroutine        CG
      end module     solver_CG
