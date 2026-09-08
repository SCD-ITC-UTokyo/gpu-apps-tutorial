!!
!! common/cfg_mod.f90
!! 実行時オプションの解析。
!!
!! 以前は C++ の config クラス (boost::program_options) を iso_c_binding 経由で
!! 呼んでいたが、Fortran だけで完結させるため純 Fortran で実装し直した。
!! 受け付ける形式は C++ 版と同じ --key=value / --key value / --help。
!!
module cfg
  use type, only: dp, flt_pos, flt_vel, fp_m
  implicit none
  private
  public :: config_t, config_constructor, config_destructor, config_configure
  public :: config_get_file, config_get_eps, config_get_mass, config_get_radius, config_get_virial
  public :: config_get_num_min, config_get_num_max, config_get_num_bin, config_get_minimum_elapsed_time
  public :: config_get_num, config_get_ft, config_get_interval, config_get_dt

  type :: config_t
     character(len=32)  :: file       = "collapse"        !! 出力ファイル名
     real(flt_pos)      :: eps        = 1.5625e-2_flt_pos !! 軟化長 (Plummer softening)
     real(flt_pos)      :: M_tot      = 1.0_flt_pos       !! 系の全質量
     real(flt_pos)      :: radius     = 1.0_flt_pos       !! 初期球の半径
     real(flt_vel)      :: virial     = 0.2_flt_vel       !! 初期条件の virial 比
     !! BENCHMARK_MODE 用
     real(dp)           :: num_min    = 1024.0_dp
     real(dp)           :: num_max    = 4.0_dp * 1024.0_dp * 1024.0_dp
     integer            :: num_bin    = 13
     real(dp)           :: elapse_min = 1.0_dp
     !! 非 BENCHMARK_MODE 用
     integer            :: num        = 1031            !! 1024 より大きい最小の素数
     real(fp_m)         :: ft         = 10.0_fp_m
     real(fp_m)         :: interval   = 0.125_fp_m
     real(fp_m)         :: dt         = 7.8125e-3_fp_m
  end type config_t

contains

  function config_constructor() result( cfg_obj )
    type(config_t) :: cfg_obj
    type(config_t) :: fresh   !! 宣言により型定義の既定値で初期化される
    !! 関数結果変数の既定初期化は処理系によって適用されないため明示的に代入する
    cfg_obj = fresh
  end function config_constructor

  subroutine config_destructor( cfg_obj )
    type(config_t), intent(inout) :: cfg_obj
  end subroutine config_destructor

  !! --key=value / --key value / --help を解析する
  subroutine config_configure( cfg_obj, argc, argv, arglen )
    type(config_t),   intent(inout) :: cfg_obj
    integer,          intent(in)    :: argc, arglen
    character(len=arglen), intent(in) :: argv(0:argc)
    character(len=arglen) :: arg, key, val
    integer :: i, sep

    i = 1
    do while( i <= argc )
       arg = adjustl( argv(i) )
       if( trim(arg) == "--help" .or. trim(arg) == "-h" ) then
          call show_help()
          stop
       end if
       if( arg(1:2) /= "--" ) then
          write(*,'(a)') 'ERROR: unrecognised argument "'//trim(arg)//'"'
          call show_help()
          error stop 1
       end if
       key = arg(3:)
       sep = index( key, "=" )
       if( sep > 0 ) then
          val = key(sep+1:)
          key = key(1:sep-1)
       else if( i < argc ) then
          i = i + 1
          val = adjustl( argv(i) )
       else
          val = ""
       end if
       if( .not. assign_option( cfg_obj, trim(key), trim(val) ) ) then
          write(*,'(a)') 'ERROR: unrecognised option "--'//trim(key)//'"'
          call show_help()
          error stop 1
       end if
       i = i + 1
    end do
  end subroutine config_configure

  logical function assign_option( cfg_obj, key, val ) result( ok )
    type(config_t),   intent(inout) :: cfg_obj
    character(len=*), intent(in)    :: key, val
    real(dp) :: rv
    ok = .true.
    select case( key )
    case( "file" );       cfg_obj%file   = val
    case( "softening" );  read(val,*) rv; cfg_obj%eps    = real( rv, flt_pos )
    case( "mass" );       read(val,*) rv; cfg_obj%M_tot  = real( rv, flt_pos )
    case( "radius" );     read(val,*) rv; cfg_obj%radius = real( rv, flt_pos )
    case( "virial" );     read(val,*) rv; cfg_obj%virial = real( rv, flt_vel )
    case( "num_min" );    read(val,*) cfg_obj%num_min
    case( "num_max" );    read(val,*) cfg_obj%num_max
    case( "num_bin" );    read(val,*) cfg_obj%num_bin
    case( "elapse_min" ); read(val,*) cfg_obj%elapse_min
    case( "num" );        read(val,*) cfg_obj%num
    case( "finish" );     read(val,*) rv; cfg_obj%ft       = real( rv, fp_m )
    case( "interval" );   read(val,*) rv; cfg_obj%interval = real( rv, fp_m )
    case( "time_step" );  read(val,*) rv; cfg_obj%dt       = real( rv, fp_m )
    case default;         ok = .false.
    end select
  end function assign_option

  subroutine show_help()
    write(*,'(a)') "List of options:"
    write(*,'(a)') "  --file        Name of the output files (default: collapse)"
    write(*,'(a)') "  --softening   Softening length (Plummer softening)"
    write(*,'(a)') "  --mass        Total mass of the system"
    write(*,'(a)') "  --radius      Radius of the initial sphere"
    write(*,'(a)') "  --virial      Virial ratio of the initial condition"
#ifdef BENCHMARK_MODE
    write(*,'(a)') "  --num_min     Minimum number of N-body particles"
    write(*,'(a)') "  --num_max     Maximum number of N-body particles"
    write(*,'(a)') "  --num_bin     Number of logarithmic grids about number of N-body particles"
    write(*,'(a)') "  --elapse_min  Minimum elapsed time for each measurement"
#else
    write(*,'(a)') "  --num         Number of N-body particles"
    write(*,'(a)') "  --finish      Final time of the simulation"
    write(*,'(a)') "  --interval    Interval between snapshots"
    write(*,'(a)') "  --time_step   Time step in the simulation"
#endif
    write(*,'(a)') "  --help, -h    Help"
  end subroutine show_help

  subroutine config_get_file( c, v, n )
    type(config_t), intent(in) :: c
    integer, intent(in) :: n
    character(len=n), intent(out) :: v
    v = c%file
  end subroutine config_get_file

  subroutine config_get_eps( c, v )
    type(config_t), intent(in) :: c
    real(flt_pos), intent(out) :: v
    v = c%eps
  end subroutine config_get_eps

  subroutine config_get_mass( c, v )
    type(config_t), intent(in) :: c
    real(flt_pos), intent(out) :: v
    v = c%M_tot
  end subroutine config_get_mass

  subroutine config_get_radius( c, v )
    type(config_t), intent(in) :: c
    real(flt_pos), intent(out) :: v
    v = c%radius
  end subroutine config_get_radius

  subroutine config_get_virial( c, v )
    type(config_t), intent(in) :: c
    real(flt_vel), intent(out) :: v
    v = c%virial
  end subroutine config_get_virial

  subroutine config_get_num_min( c, v )
    type(config_t), intent(in) :: c
    real(dp), intent(out) :: v
    v = c%num_min
  end subroutine config_get_num_min

  subroutine config_get_num_max( c, v )
    type(config_t), intent(in) :: c
    real(dp), intent(out) :: v
    v = c%num_max
  end subroutine config_get_num_max

  subroutine config_get_num_bin( c, v )
    type(config_t), intent(in) :: c
    integer, intent(out) :: v
    v = c%num_bin
  end subroutine config_get_num_bin

  subroutine config_get_minimum_elapsed_time( c, v )
    type(config_t), intent(in) :: c
    real(dp), intent(out) :: v
    v = c%elapse_min
  end subroutine config_get_minimum_elapsed_time

  subroutine config_get_num( c, v )
    type(config_t), intent(in) :: c
    integer, intent(out) :: v
    v = c%num
  end subroutine config_get_num

  subroutine config_get_ft( c, v )
    type(config_t), intent(in) :: c
    real(fp_m), intent(out) :: v
    v = c%ft
  end subroutine config_get_ft

  subroutine config_get_interval( c, v )
    type(config_t), intent(in) :: c
    real(fp_m), intent(out) :: v
    v = c%interval
  end subroutine config_get_interval

  subroutine config_get_dt( c, v )
    type(config_t), intent(in) :: c
    real(fp_m), intent(out) :: v
    v = c%dt
  end subroutine config_get_dt

end module cfg
