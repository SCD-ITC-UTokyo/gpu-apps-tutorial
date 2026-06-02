module type
  use iso_c_binding
  implicit none
  
  integer, parameter :: sp = selected_real_kind(6)  ! single precision, float.
  integer, parameter :: dp = selected_real_kind(15) ! double precision, double.
  integer, parameter :: qp = selected_real_kind(33) ! quadruple precision, quad.

  !-- FP_L
#if   FP_L ==  32
  integer, parameter :: fp_l = c_float
#elif FP_L ==  64
  integer, parameter :: fp_l = c_double
#elif FP_L == 128
  integer, parameter :: fp_l = c_long_double
#else ! default
  integer, parameter :: fp_l = c_float
#endif

  !-- FP_M
#if   FP_M ==  32
  integer, parameter :: fp_m = c_float
#elif FP_M ==  64
  integer, parameter :: fp_m = c_double
#elif FP_M == 128
  integer, parameter :: fp_m = c_long_double
#else ! default
  integer, parameter :: fp_m = c_double
#endif

  !-- FP_H
#if   FP_H ==  32
  integer, parameter :: fp_h = c_float
#elif FP_H ==  64
  integer, parameter :: fp_h = c_double
#elif FP_H == 128
  integer, parameter :: fp_h = c_long_double
#else ! default
  integer, parameter :: fp_h = c_long_double
#endif

  integer, parameter :: flt_pos = fp_m, flt_vel = fp_m, flt_acc = fp_m

  type, bind(C) :: position
     real(flt_pos) :: x, y, z, w
  end type position

  type, bind(C) :: velocity
     real(flt_vel) :: x, y, z
     real(flt_vel) :: pad
  end type velocity

  type, bind(C) :: acceleration
     real(flt_acc) :: x, y, z, w
  end type acceleration
end module type
