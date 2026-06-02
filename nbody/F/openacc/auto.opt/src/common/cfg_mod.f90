module cfg
  use type
  implicit none

  interface
     function config_constructor() bind(C)
       import :: c_ptr
       type(c_ptr) :: config_constructor
     end function config_constructor

     subroutine config_destructor( ptr ) bind(C)
       import :: c_ptr
       type(c_ptr), value :: ptr
     end subroutine config_destructor
     
     subroutine config_configure( ptr, argc, argv, arglen ) bind(C,name="config_configure_F")
       import :: c_ptr, c_int, c_char
       type(c_ptr), value :: ptr
       integer(c_int), value :: argc
       character(c_char), intent(in) :: argv(*)
       integer(c_int), value :: arglen
     end subroutine config_configure

     subroutine config_get_file( ptr, file, size ) bind(C,name="config_get_file_F")
       import :: c_ptr, c_char, c_int
       type(c_ptr), value :: ptr
       character(c_char), intent(out) :: file(*)
       integer(c_int), value :: size
     end subroutine config_get_file

     subroutine  config_get_eps( ptr, eps ) bind(C)
       import :: c_ptr, flt_pos
       type(c_ptr), value :: ptr
       real(flt_pos), intent(out) :: eps
     end subroutine config_get_eps
     
     subroutine config_get_mass( ptr, mass ) bind(C)
       import :: c_ptr, flt_pos
       type(c_ptr), value :: ptr
       real(flt_pos), intent(out) :: mass
     end subroutine config_get_mass
     
     subroutine config_get_radius( ptr, radius ) bind(C)
       import :: c_ptr, flt_pos
       type(c_ptr), value :: ptr
       real(flt_pos), intent(out) :: radius
     end subroutine config_get_radius

     subroutine config_get_virial( ptr, virial ) bind(C)
       import :: c_ptr, flt_vel
       type(c_ptr), value :: ptr
       real(flt_vel), intent(out) :: virial
     end subroutine config_get_virial

#ifdef BENCHMARK_MODE
     subroutine config_get_num_min( ptr, num_min ) bind(C)
       import :: c_ptr, c_double
       type(c_ptr), value :: ptr
       real(c_double), intent(out) :: num_min
     end subroutine config_get_num_min
     
     subroutine config_get_num_max( ptr, num_max ) bind(C)
       import :: c_ptr, c_double
       type(c_ptr), value :: ptr
       real(c_double), intent(out) :: num_max
     end subroutine config_get_num_max
     
     subroutine config_get_num_bin( ptr, num_bin ) bind(C)
       import :: c_ptr, c_int
       type(c_ptr), value :: ptr
       integer(c_int), intent(out) :: num_bin
     end subroutine config_get_num_bin
     
     subroutine config_get_minimum_elapsed_time( ptr, minimum_elapsed ) bind(C)
       import :: c_ptr, c_double
       type(c_ptr), value :: ptr
       real(c_double), intent(out) :: minimum_elapsed
     end subroutine config_get_minimum_elapsed_time
#else
     subroutine config_get_num( ptr, num ) bind(C)
       import :: c_ptr, c_int
       type(c_ptr), value :: ptr
       integer(c_int), intent(out) :: num
     end subroutine config_get_num
     
     subroutine config_get_ft( ptr, ft ) bind(C)
       import :: c_ptr, fp_m
       type(c_ptr), value :: ptr
       real(fp_m), intent(out) :: ft
     end subroutine config_get_ft
     
     subroutine config_get_interval( ptr, interval ) bind(C)
       import :: c_ptr, fp_m
       type(c_ptr), value :: ptr
       real(fp_m), intent(out) :: interval
     end subroutine config_get_interval
     
     subroutine config_get_dt( ptr, dt ) bind(C)
       import :: c_ptr, fp_m
       type(c_ptr), value :: ptr
       real(fp_m), intent(out) :: dt
     end subroutine config_get_dt
#endif
  end interface
end module cfg
