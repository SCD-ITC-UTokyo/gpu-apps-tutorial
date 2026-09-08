!!
!! common/init_mod.f90
!! 一様球の初期条件生成。
!!
!! 以前は C++ の init::set_uniform_sphere を iso_c_binding 経由で呼んでいたが、
!! Fortran だけで完結させるため純 Fortran で実装し直した。アルゴリズムは C++ 版と同一:
!!   半径 r = rad * U^(1/3)、方位は一様、速度は等方ガウス分布 (分散 sigma/sqrt(3))、
!!   最後に重心と全体速度を差し引く。
!!
!! 乱数は Fortran 組み込みの random_number を使う。C++ 版は std::random_device で
!! 毎回異なる種を引いていたため実行ごとに初期条件が変わったが、こちらは既定で
!! 決まった種を使い再現性を確保する。BENCHMARK_MODE の測定量 (O(N^2) の力計算時間)
!! は初期条件に依存しないため、測定結果への影響はない。
!!
module init
  use type, only: dp, position, velocity, flt_pos, flt_vel
  implicit none
  private
  public :: init_set_uniform_sphere

contains

  subroutine init_set_uniform_sphere( num, pos, vel, Mtot, rad, virial, newton )
    integer,             intent(in)    :: num
    type(position),      intent(inout) :: pos(num)
    type(velocity),      intent(inout) :: vel(num)
    real(flt_pos),       intent(in)    :: Mtot, rad
    real(flt_vel),       intent(in)    :: virial, newton

    real(flt_vel) :: sigma, sig1d
    real(flt_pos) :: mass, r_sph, prj, r_xy, theta
    real(flt_pos) :: cx, cy, cz, M_inv
    real(flt_vel) :: vx, vy, vz
    real(dp)      :: u1, u2, u3
    integer       :: i

    call seed_rng()

    sigma = sqrt( 1.2_flt_vel * newton * real(Mtot, flt_vel) * virial / real(rad, flt_vel) )
    sig1d = sigma / sqrt( 3.0_flt_vel )

    mass = Mtot / real( num, flt_pos )
    do i = 1, num
       call random_number( u1 )
       r_sph = rad * real( u1**(1.0_dp/3.0_dp), flt_pos )
       call random_number( u2 )
       prj = 2.0_flt_pos * real( u2, flt_pos ) - 1.0_flt_pos
       r_xy  = r_sph * sqrt( 1.0_flt_pos - prj*prj )
       call random_number( u3 )
       theta = real( 2.0_dp * acos(-1.0_dp), flt_pos ) * real( u3, flt_pos )

       pos(i)%x = r_xy * cos( theta )
       pos(i)%y = r_xy * sin( theta )
       pos(i)%z = r_sph * prj
       pos(i)%w = mass

       vel(i)%x = sig1d * gaussian()
       vel(i)%y = sig1d * gaussian()
       vel(i)%z = sig1d * gaussian()
       vel(i)%pad = 0.0_flt_vel
    end do

    !! 重心と全体速度を除去する
    cx = 0.0_flt_pos; cy = 0.0_flt_pos; cz = 0.0_flt_pos
    vx = 0.0_flt_vel; vy = 0.0_flt_vel; vz = 0.0_flt_vel
    do i = 1, num
       cx = cx + pos(i)%w * pos(i)%x
       cy = cy + pos(i)%w * pos(i)%y
       cz = cz + pos(i)%w * pos(i)%z
       vx = vx + real(pos(i)%w, flt_vel) * vel(i)%x
       vy = vy + real(pos(i)%w, flt_vel) * vel(i)%y
       vz = vz + real(pos(i)%w, flt_vel) * vel(i)%z
    end do
    M_inv = 1.0_flt_pos / Mtot
    cx = cx * M_inv; cy = cy * M_inv; cz = cz * M_inv
    vx = vx * real(M_inv, flt_vel); vy = vy * real(M_inv, flt_vel); vz = vz * real(M_inv, flt_vel)
    do i = 1, num
       pos(i)%x = pos(i)%x - cx
       pos(i)%y = pos(i)%y - cy
       pos(i)%z = pos(i)%z - cz
       vel(i)%x = vel(i)%x - vx
       vel(i)%y = vel(i)%y - vy
       vel(i)%z = vel(i)%z - vz
    end do
  end subroutine init_set_uniform_sphere

  !! 決まった種で乱数列を初期化する (再現性のため)
  subroutine seed_rng()
    integer              :: n, i
    integer, allocatable :: seed(:)
    call random_seed( size = n )
    allocate( seed(n) )
    do i = 1, n
       seed(i) = 20220101 + 37*i
    end do
    call random_seed( put = seed )
    deallocate( seed )
  end subroutine seed_rng

  !! Box-Muller 法による標準正規乱数
  real(flt_vel) function gaussian() result( g )
    real(dp) :: u1, u2
    call random_number( u1 )
    call random_number( u2 )
    if( u1 < tiny(u1) ) u1 = tiny(u1)
    g = real( sqrt( -2.0_dp * log(u1) ) * cos( 2.0_dp * acos(-1.0_dp) * u2 ), flt_vel )
  end function gaussian

end module init
