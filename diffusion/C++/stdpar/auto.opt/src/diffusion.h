#ifndef DIFFUSION_H
#define DIFFUSION_H

#include "misc.h"

double diffusion3d(int nx, int ny, int nz, int mgn, flt dx, flt dy, flt dz, flt dt, flt kappa,
                   const flt *f, flt *fn);
void init(int nx, int ny, int nz, int mgn, flt dx, flt dy, flt dz, flt *f);
double err(double time, int nx, int ny, int nz, int mgn, flt dx, flt dy, flt dz, flt kappa, const flt *f);

#endif /* DIFFUSION_H */
