module init
  use type
  implicit none

  interface
     subroutine init_set_uniform_sphere( num, pos, vel, Mtot, rad, virial, newton ) bind(C)
       import :: c_int, position, velocity, flt_pos, flt_vel
       integer(c_int), value :: num
       type(position), intent(inout) :: pos(*)
       type(velocity), intent(inout) :: vel(*)
       real(flt_pos), value :: Mtot, rad
       real(flt_vel), value :: virial, newton
     end subroutine init_set_uniform_sphere
  end interface
end module init
