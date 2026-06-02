#include <boost/filesystem.hpp>                // boost::filesystem
#include "io.hpp"
#include "io.h"

extern "C"
{
  void io_write_snapshot
  ( const int num, const position pos[], velocity vel[], const acceleration acc[],
    const char* file, const int snp_id, const fp_m time, void* error_ptr ){
    conservatives& error_obj = *(conservatives*)error_ptr;
    io::write_snapshot( num, (type::position*)pos, (type::velocity*)vel, (type::acceleration*)acc,
			file, snp_id, time, error_obj );
  }

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

  void io_write_snapshot_F
  ( const int num, const position pos[], velocity vel[], const acceleration acc[],
    const char* fileF, const int filesizeF, const int snp_id, const fp_m time, void* error_ptr ){
    char* const fileC = strdupF2C( fileF, filesizeF );
    conservatives& error_obj = *(conservatives*)error_ptr;
    io::write_snapshot( num, (type::position*)pos, (type::velocity*)vel, (type::acceleration*)acc,
			fileC, snp_id, time, error_obj );
  }

#if defined(CALCULATE_POTENTIAL) && !defined(BENCHMARK_MODE)
  void io_write_log
  ( const char* exec, const double elapsed, const int step, const int num,
    const char* file, const fp_m dt, const void* error_ptr ){
    conservatives& error_obj = *(conservatives*)error_ptr;
    
    io::write_log( exec, elapsed, step, num, file, dt, error_obj );
  }
  void io_write_log_F
  ( const char* execF, const int execsizeF, const double elapsed, const int step, const int num,
    const char* fileF, const int filesizeF, const fp_m dt, const void* error_ptr ){
    char* const execC = strdupF2C( execF, execsizeF );       
    char* const fileC = strdupF2C( fileF, filesizeF );   
    conservatives& error_obj = *(conservatives*)error_ptr;
    io::write_log( execC, elapsed, step, num, fileC, dt, error_obj );
  }
#else
  void io_write_log
  ( const char* exec, const double elapsed, const int step, const int num, const char* file ){
    io::write_log( exec, elapsed, step, num, file );
  }
  void io_write_log_F
  ( const char* execF, const int execsizeF, const double elapsed, const int step, const int num,
    const char* fileF, const int filesizeF ){
    char* const execC = strdupF2C( execF, execsizeF );   
    char* const fileC = strdupF2C( fileF, filesizeF );   
    io::write_log( execC, elapsed, step, num, fileC );
  }
#endif
}
