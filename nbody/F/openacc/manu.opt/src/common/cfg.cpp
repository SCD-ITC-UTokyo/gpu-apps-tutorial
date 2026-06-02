#include <iostream>  // std::cout is required in cfg.hpp
#include <string>    // std::string is required in cfg.hpp
#include "cfg.hpp"
#include "cfg.h"

extern "C"
{
  // make a C string from a Fortran string.
  static inline char* strdupF2C( const char* strF, const int sizeF ){
    int len=sizeF;
    while( --len>0 ){ // find the last non white space.
      if( strF[len-1] != ' ' ) break;
    }
    char* const strC = new char [len+1];
    strC[len] = 0; // terminate by a null character.
    while( --len>=0 ){ // copy the string.
      strC[len] = strF[len];
    }
    return strC;
  }
  // copy a C++ string to a C string.
  char* strncpyCXX2C( char* strC, int sizeC, const std::string& strCXX ){
    const char* ptr = strCXX.c_str();
    int   len = strCXX.length();

    for( int i=0; i<sizeC; i++ ){
      // copy as C string terminated by null.
      strC[i] = i<len ? ptr[i] : 0;
    }
    strC[sizeC-1]=0;
    return strC;
  }
  // copy a C++ string to a Fotran string.
  char* strncpyCXX2F( char* strF, int sizeC, const std::string& strCXX ){
    const char* ptr = strCXX.c_str();
    int   len = strCXX.length();

    for( int i=0; i<sizeC; i++ ){
      // copy as fortran character(len=sizeC).
      strF[i] = i<len ? ptr[i] : ' ';
    }
    return strF;
  }

  void* config_constructor( void ){
    config* obj = new config();
    return obj;
  }
  void  config_destructor( void* ptr ){
    config* obj = (config*)ptr;
    delete obj;
  }
  void  config_configure( void* ptr, int argc, char** argv ){
    config* obj = (config*)ptr;
    obj->configure( argc, argv );
  }
  void  config_configure_F( void* ptr, int argcF, char* argvF, int sizeF ){
    const int argcC = argcF + 1;
    char** const argvC = new char* [argcC+1];

    for( int i=0; i<argcC; i++ ){
      const char* const argF = argvF + i*sizeF;
      char* const argC = strdupF2C( argF, sizeF );
      argvC[i] = argC;
    }
    argvC[argcC] = NULL;

    config* obj = (config*)ptr;
    obj->configure( argcC, argvC );
  }
  void  config_get_file( void* ptr, char* file, int size ){
    config* obj = (config*)ptr;
    strncpyCXX2C( file, size, obj->get_file() );
  }
  void  config_get_file_F( void* ptr, char* file, int size ){
    config* obj = (config*)ptr;
    strncpyCXX2F( file, size, obj->get_file() );
  }
  void  config_get_eps( void* ptr, flt_pos* eps ){
    config* obj = (config*)ptr;
    *eps = obj->get_eps();
  }
  void  config_get_mass( void* ptr, flt_pos* mass ){
    config* obj = (config*)ptr;
    *mass = obj->get_mass();
  }
  void  config_get_radius( void* ptr, flt_pos* radius ){
    config* obj = (config*)ptr;
    *radius = obj->get_radius();
  }
  void  config_get_virial( void* ptr, flt_vel* virial ){
    config* obj = (config*)ptr;
    *virial = obj->get_virial();
  }

#ifdef BENCHMARK_MODE
  void  config_get_num_min( void* ptr, double* num_min ){
    config* obj = (config*)ptr;
    *num_min = std::get<0>(obj->get_num());
  }
  void  config_get_num_max( void* ptr, double* num_max ){
    config* obj = (config*)ptr;
    *num_max = std::get<1>(obj->get_num());
  }
  void  config_get_num_bin( void* ptr, int* num_bin ){
    config* obj = (config*)ptr;
    *num_bin = std::get<2>(obj->get_num());
  }
  void  config_get_minimum_elapsed_time( void* ptr, double* minimum_elapsed ){
    config* obj = (config*)ptr;
    *minimum_elapsed = obj->get_minimum_elapsed_time();
  }
#else   // BENCHMARK_MODE  
  void  config_get_num( void* ptr, int* num ){
    config* obj = (config*)ptr;
    *num = obj->get_num();
  }
  void  config_get_ft( void* ptr, fp_m* ft ){
    config* obj = (config*)ptr;
    *ft = std::get<0>(obj->get_time());
  }
  void  config_get_interval( void* ptr, fp_m* interval ){
    config* obj = (config*)ptr;
    *interval = std::get<1>(obj->get_time());
  }
  void  config_get_dt( void* ptr, fp_m* dt ){
    config* obj = (config*)ptr;
    *dt = obj->get_dt();
  }
#endif
}
