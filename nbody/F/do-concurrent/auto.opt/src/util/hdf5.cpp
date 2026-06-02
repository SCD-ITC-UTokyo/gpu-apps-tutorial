#include "hdf5.hpp"
#include "hdf5.h"

extern "C" {
  void util_hdf5_commit_datatype_vec3( void ){
    util::hdf5::commit_datatype_vec3();
  }
  void util_hdf5_commit_datatype_vec4( void ){
    util::hdf5::commit_datatype_vec4();
  }
  void util_hdf5_remove_datatype_vec3( void ){
    util::hdf5::remove_datatype_vec3();
  }
  void util_hdf5_remove_datatype_vec4( void ){
    util::hdf5::remove_datatype_vec4();
  }
}
