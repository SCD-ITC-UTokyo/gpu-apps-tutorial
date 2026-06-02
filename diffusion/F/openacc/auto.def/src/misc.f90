module misc
  use iso_fortran_env, only: int64
  implicit none
  integer, parameter :: sp = selected_real_kind(6)  ! single precision, float.
  integer, parameter :: dp = selected_real_kind(15) ! double precision, double.
  integer, parameter :: qp = selected_real_kind(33) ! quadruple precision, quad.

#if    FP == 32
  integer, parameter :: fp = sp
#elif  FP == 64
  integer, parameter :: fp = dp
#elif  FP == 128
  integer, parameter :: fp = qp
#endif

  real(dp) :: t_s        ! kernel 計測 wall (omp_get_wtime ベース)
  real(dp) :: t_real_s   ! プログラム全体の wall 開始 (start_real_timer から計測)
  real(dp) :: t_cpu_s    ! CPU 時間開始 (cpu_time ベース、user+sys 合算近似)

contains

  subroutine swap(f, fn)
    real(fp),pointer,dimension(:,:,:),intent(inout) :: f,fn
    real(fp),pointer,dimension(:,:,:) :: ftmp

    ftmp => f
    f => fn
    fn => ftmp
  end subroutine swap

  subroutine start_timer()
    real(dp) :: omp_get_wtime
    t_s = omp_get_wtime()
  end subroutine start_timer

  double precision function get_elapsed_time()
    real(dp) :: omp_get_wtime
    get_elapsed_time = omp_get_wtime() - t_s
  end function get_elapsed_time

  ! プログラム冒頭で呼び、real_sec 計測の基準とする
  subroutine start_real_timer()
    real(dp) :: omp_get_wtime
    t_real_s = omp_get_wtime()
    call cpu_time(t_cpu_s)
  end subroutine start_real_timer

  double precision function get_real_time()
    real(dp) :: omp_get_wtime
    get_real_time = omp_get_wtime() - t_real_s
  end function get_real_time

  ! cpu_time() は user+sys を合算した値を返す (Fortran 標準では分離不可)
  ! 本教材では user_sec として出力、sys_sec は別途 0.0 を出力する
  double precision function get_cpu_time_used()
    real(dp) :: cpu_now
    call cpu_time(cpu_now)
    get_cpu_time_used = cpu_now - t_cpu_s
  end function get_cpu_time_used

end module misc
