module util_timer
  use iso_c_binding
  implicit none

  interface
     function util_timer_constructor() bind(C)
       import :: c_ptr
       type(c_ptr) :: util_timer_constructor
     end function util_timer_constructor

     subroutine util_timer_destructor( ptr ) bind(C)
       import :: c_ptr
       type(c_ptr), value :: ptr
     end subroutine util_timer_destructor

     subroutine util_timer_start( ptr ) bind(C)
       import :: c_ptr
       type(c_ptr), value :: ptr
     end subroutine util_timer_start

     subroutine util_timer_stop( ptr ) bind(C)
       import :: c_ptr
       type(c_ptr), value :: ptr
     end subroutine util_timer_stop

     subroutine util_timer_clear( ptr ) bind(C)
       import :: c_ptr
       type(c_ptr), value :: ptr
     end subroutine util_timer_clear

     subroutine util_timer_get_elapsed_wall( ptr, elapsed ) bind(C)
       import :: c_ptr, c_double
       type(c_ptr), value :: ptr
       real(c_double), intent(out) :: elapsed
     end subroutine util_timer_get_elapsed_wall
     
  end interface
end module util_timer
