!!
!! util/timer_mod.f90
!! 経過時間の計測。
!!
!! 以前は C++ の util::timer (boost::timer::cpu_timer) を iso_c_binding 経由で
!! 呼んでいたが、Fortran だけで完結させるため純 Fortran で実装し直した。
!!   wall clock : system_clock (C++ 版の std::chrono::steady_clock と等価)
!!   user CPU   : cpu_time     (C++ 版の getrusage(ru_utime) と等価)
!!
module util_timer
  use iso_fortran_env, only: int64
  use type, only: dp
  implicit none
  private
  public :: timer_t, util_timer_constructor, util_timer_destructor
  public :: util_timer_start, util_timer_stop, util_timer_clear
  public :: util_timer_get_elapsed_wall, util_timer_get_elapsed_user

  type :: timer_t
     integer(int64) :: wall_start = 0_int64   !! 計測開始時の system_clock カウント
     real(dp)       :: user_start = 0.0_dp    !! 計測開始時の user CPU 時間
     real(dp)       :: elapsed_wall = 0.0_dp  !! 累積 wall clock 時間 [s]
     real(dp)       :: elapsed_user = 0.0_dp  !! 累積 user CPU 時間 [s]
  end type timer_t

contains

  function util_timer_constructor() result( t )
    type(timer_t) :: t
    call util_timer_clear( t )
  end function util_timer_constructor

  subroutine util_timer_destructor( t )
    type(timer_t), intent(inout) :: t
    call util_timer_clear( t )
  end subroutine util_timer_destructor

  subroutine util_timer_clear( t )
    type(timer_t), intent(inout) :: t
    t%wall_start   = 0_int64
    t%user_start   = 0.0_dp
    t%elapsed_wall = 0.0_dp
    t%elapsed_user = 0.0_dp
  end subroutine util_timer_clear

  subroutine util_timer_start( t )
    type(timer_t), intent(inout) :: t
    call system_clock( t%wall_start )
    call cpu_time( t%user_start )
  end subroutine util_timer_start

  subroutine util_timer_stop( t )
    type(timer_t), intent(inout) :: t
    integer(int64) :: wall_stop, rate
    real(dp)       :: user_stop
    call system_clock( wall_stop, rate )
    call cpu_time( user_stop )
    t%elapsed_wall = t%elapsed_wall + real( wall_stop - t%wall_start, dp ) / real( rate, dp )
    t%elapsed_user = t%elapsed_user + ( real(user_stop, dp) - t%user_start )
  end subroutine util_timer_stop

  subroutine util_timer_get_elapsed_wall( t, elapsed )
    type(timer_t), intent(in)  :: t
    real(dp),      intent(out) :: elapsed
    elapsed = t%elapsed_wall
  end subroutine util_timer_get_elapsed_wall

  subroutine util_timer_get_elapsed_user( t, elapsed )
    type(timer_t), intent(in)  :: t
    real(dp),      intent(out) :: elapsed
    elapsed = t%elapsed_user
  end subroutine util_timer_get_elapsed_user

end module util_timer
