      subroutine OUTPUT_UCD 
      use pfem_util
      implicit none

      integer(kind=kint) :: i,ie
      integer(kind=kint) :: N0,N1
      integer(kind=kint) :: in1,in2,in3,in4,in5,in6,in7,in8
      real(kind=kreal)   :: XX,YY,ZZ
      character(len=6) :: ETYPE

!C
!C +----------+
!C | AVS file |
!C +----------+
!C===
        open (21 ,file= 'test.inp', status='unknown')

        N0= 0
        N1= 1

        write (21,'(10i8)')  NP, ICELTOT, N1, N0, N0
        do i= 1, NP
          XX= XYZ(i,1)
          YY= XYZ(i,2)
          ZZ= XYZ(i,3)
          write (21,'(i8,3(1pe16.6))') i, XX, YY, ZZ
        enddo
        do ie= 1, ICELTOT
          ETYPE= 'hex   '
          in1= ICELNOD(ie,1)
          in2= ICELNOD(ie,2)
          in3= ICELNOD(ie,3)
          in4= ICELNOD(ie,4)
          in5= ICELNOD(ie,5)
          in6= ICELNOD(ie,6)
          in7= ICELNOD(ie,7)
          in8= ICELNOD(ie,8)

          write (21,'(i8,i3,1x,a6,1x,8i8)')                             &
     &      ie, N1, ETYPE, in1, in2, in3, in4, in5, in6, in7, in8

        enddo

        write (21,'(10i3)')  N1, N1
        write (21,'(a  )') 'temperature,temperature'

        do i= 1, NP
          write (21,'(i8, 4(1pe16.6))')  i, X(i)
        enddo
        close (21)
!C===
      end subroutine output_ucd

