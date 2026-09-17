
#ifndef DIFFUSION_H
#define DIFFUSION_H
#include "misc.h"
#include <Kokkos_Core.hpp>


double diffusion3d(int nx, int ny, int nz, int mgn, flt dx, flt dy, flt dz, flt dt, flt kappa,
                   Kokkos::View<flt*> f, Kokkos::View<flt*> fn);
void init(int nx, int ny, int nz, int mgn, flt dx, flt dy, flt dz, Kokkos::View<flt*> f);
double err(double time, int nx, int ny, int nz, int mgn, flt dx, flt dy, flt dz, flt kappa, Kokkos::View<flt*> f);



#endif /* DIFFUSION_H */
