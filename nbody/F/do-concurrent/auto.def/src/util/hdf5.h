#ifndef UTIL_HDF5_H
#define UTIL_HDF5_H

#ifdef __cplusplus
extern "C" {
#endif

void util_hdf5_commit_datatype_vec3( void );
void util_hdf5_commit_datatype_vec4( void );
void util_hdf5_remove_datatype_vec3( void );
void util_hdf5_remove_datatype_vec4( void );

#ifdef __cplusplus
}
#endif

#endif // UTIL_HDF5_H
