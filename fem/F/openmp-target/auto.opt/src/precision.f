      module precision
      implicit none
      integer, parameter :: sp = selected_real_kind(6) ! single precision, float.
      integer, parameter :: dp = selected_real_kind(15) ! double precision, double.
      integer, parameter :: qp = selected_real_kind(33) ! quadruple precision, quad.

      integer, parameter   ::  kint       =  4 
#if    FP == 32
      integer, parameter   ::  kreal      =  sp
#elif  FP == 64
      integer, parameter   ::  kreal      =  dp
#elif  FP == 128
      integer, parameter   ::  kreal      =  qp
#endif
      integer, parameter   ::  char_length= 64
      end module precision
