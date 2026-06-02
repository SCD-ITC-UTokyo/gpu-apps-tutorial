#ifndef COMMON_IO_H
#define COMMON_IO_H

#include "common/type.h"

#ifdef __cplusplus
extern "C" {
#endif
  void io_write_snapshot
  ( const int num, const position pos[], velocity vel[], const acceleration acc[],
    const char* file, const int snp_id, const fp_m time, void* error_ptr );
  void io_write_snapshot_F
  ( const int num, const position pos[], velocity vel[], const acceleration acc[],
    const char* fileF, const int filesizeF,
    const int snp_id, const fp_m time, void* error_ptr );

#if defined(CALCULATE_POTENTIAL) && !defined(BENCHMARK_MODE)
  void io_write_log
  ( const char* exec, const double elapsed, const int step, const int num,
    const char* file, const fp_m dt, const void* error_ptr );
  void io_write_log_F
  ( const char* execF, const int execsizeF, const double elapsed, const int step, const int num,
    const char* fileF, const int filesizeF, const fp_m dt, const void* error_ptr );
#else
  void io_write_log
  ( const char* exec, const double elapsed, const int step, const int num, const char* file );
  void io_write_log_F
  ( const char* execF, const int execsizeF, const double elapsed, const int step, const int num,
    const char* fileF, const int filesizeF );
#endif

#ifdef __cplusplus
}
#endif

#endif // COMMON_IO_H
