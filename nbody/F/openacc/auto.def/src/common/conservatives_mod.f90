module conservatives
  use iso_c_binding
  implicit none

  interface
     function conservatives_constructor() bind(C)
       import :: c_ptr
       type(c_ptr) :: conservatives_constructor
     end function conservatives_constructor

     subroutine conservatives_destructor( ptr ) bind(C)
       import :: c_ptr
       type(c_ptr), value :: ptr
     end subroutine conservatives_destructor
  end interface
end module conservatives
