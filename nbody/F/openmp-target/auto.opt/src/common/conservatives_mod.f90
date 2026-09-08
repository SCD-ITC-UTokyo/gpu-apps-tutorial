!!
!! common/conservatives_mod.f90
!! 保存量 (エネルギー・運動量・virial 比) の保持。
!!
!! 以前は C++ の conservatives クラスを iso_c_binding 経由で扱っていたが、
!! Fortran だけで完結させるため純 Fortran の派生型に置き換えた。
!! 値の計算は C++ 版でもスナップショット出力 (HDF5) の中で行われており、
!! Fortran 側からは生成と受け渡ししか行っていない。HDF5 出力を廃止したため
!! BENCHMARK_MODE では使われない。
!!
module conservatives
  use type, only: fp_h
  implicit none
  private
  public :: conservatives_t, conservatives_constructor, conservatives_destructor

  type :: conservatives_t
     real(fp_h) :: energy_error_worst  = 0.0_fp_h  !! 計算中の最悪エネルギー誤差
     real(fp_h) :: energy_error_final  = 0.0_fp_h  !! 最終エネルギー誤差
     real(fp_h) :: virial_ratio_final  = 0.0_fp_h  !! 最終 virial 比
  end type conservatives_t

contains

  function conservatives_constructor() result( e )
    type(conservatives_t) :: e
    !! 関数結果変数の既定初期化は処理系によって適用されないため明示的に代入する
    e%energy_error_worst = 0.0_fp_h
    e%energy_error_final = 0.0_fp_h
    e%virial_ratio_final = 0.0_fp_h
  end function conservatives_constructor

  subroutine conservatives_destructor( e )
    type(conservatives_t), intent(inout) :: e
    e%energy_error_worst = 0.0_fp_h
    e%energy_error_final = 0.0_fp_h
    e%virial_ratio_final = 0.0_fp_h
  end subroutine conservatives_destructor

end module conservatives
