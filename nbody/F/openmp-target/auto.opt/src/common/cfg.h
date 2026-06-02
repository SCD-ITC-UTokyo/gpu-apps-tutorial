#ifndef COMMON_CFG_H
#define COMMON_CFG_H

#include "common/type.h"

#ifdef __cplusplus
extern "C" {
#endif

void* config_constructor( void );
void  config_destructor( void* ptr );
void  config_configure( void* ptr, int argc, char** argv );
void  config_configure_F( void* ptr, int argc, char* argv, int size );
void  config_get_file( void* ptr, char* file, int size );
void  config_get_file_F( void* ptr, char* file, int size );
void  config_get_eps( void* ptr, flt_pos* eps );
void  config_get_mass( void* ptr, flt_pos* mass );
void  config_get_radius( void* ptr, flt_pos* radius );
void  config_get_virial( void* ptr, flt_vel* virial );

#ifdef BENCHMARK_MODE
void  config_get_num_min( void* ptr, double* num_min );
void  config_get_num_max( void* ptr, double* num_max );
void  config_get_num_bin( void* ptr, int*    num_bin );
void  config_get_minimum_elapsed_time( void* ptr, double* minimum_elapsed );
#else
void  config_get_num( void* ptr, int* num );
void  config_get_ft( void* ptr, fp_m* ft );
void  config_get_interval( void* ptr, fp_m* interval );
void  config_get_dt( void* ptr, fp_m* dt );
#endif

#ifdef __cplusplus
}
#endif

#endif // COMMON_CFG_H
