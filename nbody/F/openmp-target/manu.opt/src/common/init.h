#ifndef COMMON_INIT_H
#define COMMON_INIT_H

#include "common/type.h"

#ifdef __cplusplus
extern "C" {
#endif

void init_set_uniform_sphere
( const int num, position pos[], velocity vel[],
  const flt_pos Mtot, const flt_pos rad, const flt_vel virial, const flt_vel newton );

#ifdef __cplusplus
}
#endif

#endif // COMMON_INIT_H
