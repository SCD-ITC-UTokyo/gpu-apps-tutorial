#ifndef COMMON_TYPE_H
#define COMMON_TYPE_H

// FP_L
#if   FP_L ==  32
typedef float       fp_l;
#elif FP_L ==  64
typedef double      fp_l;
#elif FP_L == 128
typedef long double fp_l;
#else // default
typedef float       fp_l;
#endif

// FP_M
#if   FP_M ==  32
typedef float       fp_m;
#elif FP_M ==  64
typedef double      fp_m;
#elif FP_M == 128
typedef long double fp_m;
#else // default
typedef double      fp_m;
#endif

// FP_H
#if   FP_H ==  32
typedef float       fp_h;
#elif FP_H ==  64
typedef double      fp_h;
#elif FP_H == 128
typedef long double fp_h;
#else // default
typedef long double fp_h;
#endif


typedef fp_m flt_pos;
typedef fp_m flt_vel;
typedef fp_m flt_acc;

typedef struct {
  flt_pos x, y, z, w;
} position;

typedef struct {
  flt_vel x, y, z;
  flt_vel pad;
} velocity;

typedef struct {
  flt_acc x, y, z, w;
} acceleration;

#endif  // COMMON_TYPE_H
