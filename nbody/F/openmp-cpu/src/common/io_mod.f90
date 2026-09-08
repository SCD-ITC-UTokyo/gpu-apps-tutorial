!!
!! common/io_mod.f90
!! ログ出力。
!!
!! 以前は C++ の io::write_log を iso_c_binding 経由で呼んでいたが、
!! Fortran だけで完結させるため純 Fortran で実装し直した。
!! 出力先と列は C++ 版 (common/io.hpp) と同一で、log/<file>_run.csv に追記する。
!!   exec,N,time[s],step,time_per_step[s],interactions_per_sec,Flop/s,FP_L,FP_M
!! HDF5 によるスナップショット出力 (io_write_snapshot) は廃止したため持たない。
!!
module io
  use type, only: dp, fp_m
  use conservatives, only: conservatives_t
  implicit none
  private
  public :: io_write_log

  !! 1 相互作用あたりの浮動小数演算数。C++ 版 (common/io.hpp) と同じ値にすること。
  !!   FP_M: 3 減算 + 3 加算           =  6
  !!   FP_L: 3 FMA + 6 乗算            = 12
  !!   FP_L: 逆平方根 1 回 (A100 で 4) =  4
  !!   CALCULATE_POTENTIAL 有効時に +2
#ifdef CALCULATE_POTENTIAL
  real(dp), parameter :: FLOPS_PER_INTERACTION = 24.0_dp
#else
  real(dp), parameter :: FLOPS_PER_INTERACTION = 22.0_dp
#endif

contains

#if defined(CALCULATE_POTENTIAL) && !defined(BENCHMARK_MODE)
  subroutine io_write_log( exec, execsize, elapsed, step, num, file, filesize, dt, error )
    character(len=*),      intent(in) :: exec, file
    integer,               intent(in) :: execsize, filesize, step, num
    real(dp),              intent(in) :: elapsed
    real(fp_m),            intent(in) :: dt
    type(conservatives_t), intent(in) :: error
    call write_row( exec, file, elapsed, step, num, .true., dt, error )
  end subroutine io_write_log
#else
  subroutine io_write_log( exec, execsize, elapsed, step, num, file, filesize )
    character(len=*), intent(in) :: exec, file
    integer,          intent(in) :: execsize, filesize, step, num
    real(dp),         intent(in) :: elapsed
    type(conservatives_t)        :: dummy
    call write_row( exec, file, elapsed, step, num, .false., 0.0_fp_m, dummy )
  end subroutine io_write_log
#endif

  subroutine write_row( exec, file, elapsed, step, num, with_error, dt, error )
    character(len=*),      intent(in) :: exec, file
    real(dp),              intent(in) :: elapsed
    integer,               intent(in) :: step, num
    logical,               intent(in) :: with_error
    real(fp_m),            intent(in) :: dt
    type(conservatives_t), intent(in) :: error
    character(len=256) :: note
    real(dp) :: n_pairs, pairs_per_sec
    integer  :: unit_id, ios, cmdstat
    logical  :: exist

    !! 出力先ディレクトリが無いと open に失敗するため作成しておく
    call execute_command_line( "mkdir -p log", wait=.true., cmdstat=cmdstat )

    note = "log/" // trim(file) // "_run.csv"
    inquire( file=trim(note), exist=exist )
    open( newunit=unit_id, file=trim(note), position="append", action="write", iostat=ios )
    if( ios /= 0 ) return

    if( .not. exist ) then
       if( with_error ) then
          write(unit_id,'(a)') "exec,N,time[s],step,time_per_step[s],interactions_per_sec,Flop/s,FP_L,FP_M,"// &
               "dt,energy_error_worst,energy_error_final,virial_ratio_final"
       else
          write(unit_id,'(a)') "exec,N,time[s],step,time_per_step[s],interactions_per_sec,Flop/s,FP_L,FP_M"
       end if
    end if

    n_pairs       = real(num,dp) * real(num,dp) * real(step,dp)
    pairs_per_sec = n_pairs / elapsed

    if( with_error ) then
       write(unit_id,'(a,",",i0,",",es23.16,",",i0,3(",",es23.16),",",i0,",",i0,4(",",es23.16))') &
            trim(exec), num, elapsed, step, elapsed/real(step,dp), &
            pairs_per_sec, pairs_per_sec*FLOPS_PER_INTERACTION, FP_L, FP_M, &
            real(dt,dp), real(error%energy_error_worst,dp), &
            real(error%energy_error_final,dp), real(error%virial_ratio_final,dp)
    else
       write(unit_id,'(a,",",i0,",",es23.16,",",i0,3(",",es23.16),",",i0,",",i0)') &
            trim(exec), num, elapsed, step, elapsed/real(step,dp), &
            pairs_per_sec, pairs_per_sec*FLOPS_PER_INTERACTION, FP_L, FP_M
    end if
    close( unit_id )
  end subroutine write_row

end module io
