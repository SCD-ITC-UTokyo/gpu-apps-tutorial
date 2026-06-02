module util_hdf5
  use iso_c_binding
  implicit none

  interface
     subroutine util_hdf5_commit_datatype_vec3() bind(C)
     end subroutine util_hdf5_commit_datatype_vec3
     subroutine util_hdf5_commit_datatype_vec4() bind(C)
     end subroutine util_hdf5_commit_datatype_vec4
     subroutine util_hdf5_remove_datatype_vec3() bind(C)
     end subroutine util_hdf5_remove_datatype_vec3
     subroutine util_hdf5_remove_datatype_vec4() bind(C)
     end subroutine util_hdf5_remove_datatype_vec4
  end interface
end module util_hdf5
