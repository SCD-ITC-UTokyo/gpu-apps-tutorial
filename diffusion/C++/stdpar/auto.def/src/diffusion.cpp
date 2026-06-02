#include <stdio.h>
#include <math.h>
#include "misc.h"

#include <numeric>   // std::transform_reduce
#include <iterator>  // std::begin, std::end
#include <algorithm> // std::for_each
#include <execution> // std::execution::par
#include <boost/iterator/counting_iterator.hpp> // boost::iterators::counting_iterator

double diffusion3d(int nx, int ny, int nz, int mgn, flt dx, flt dy, flt dz, flt dt, flt kappa,
                   const flt *f, flt *fn)
{
    const flt ce = kappa * dt / (dx * dx);
    const flt cw = ce;
    const flt cn = kappa * dt / (dy * dy);
    const flt cs = cn;
    const flt ct = kappa * dt / (dz * dz);
    const flt cb = ct;

    const flt cc = 1.0F - (ce + cw + cn + cs + ct + cb);

    std::for_each_n
    ( std::execution::par, boost::iterators::counting_iterator<int32_t>(0), nx*ny*nz, [&](int iter)
    {
      int ix = iter + nx*ny*mgn;
      int k = (ix/(nx*ny)) - mgn;
      int j = (ix%(nx*ny)) / nx;
      int i = (ix%(nx*ny)) % nx;

      const int ip = i == nx - 1 ? ix : ix + 1;
      const int im = i == 0      ? ix : ix - 1;
      const int jp = j == ny - 1 ? ix : ix + nx;
      const int jm = j == 0      ? ix : ix - nx;
      const int kp = k == nz - 1 ? ix : ix + nx*ny;
      const int km = k == 0      ? ix : ix - nx*ny;

      fn[ix] = cc*f[ix] + ce*f[ip] + cw*f[im] + cn*f[jp] + cs*f[jm] + ct*f[kp] + cb*f[km];
    });

    /*
#pragma omp parallel for collapse(3)
    for(int k = 0; k < nz; k++) {
        for (int j = 0; j < ny; j++) {
            for (int i = 0; i < nx; i++) {
                const int ix = nx * ny * (k + mgn) + nx * j + i;
                const int ip = i == nx - 1 ? ix : ix + 1;
                const int im = i == 0      ? ix : ix - 1;
                const int jp = j == ny - 1 ? ix : ix + nx;
                const int jm = j == 0      ? ix : ix - nx;
                const int kp = (k == nz - 1) ? ix : ix + nx*ny;
                const int km = (k == 0     ) ? ix : ix - nx*ny;

                fn[ix] = cc*f[ix] + ce*f[ip] + cw*f[im] + cn*f[jp] + cs*f[jm] + ct*f[kp] + cb*f[km];
            }
        }
    }
     */

    return ((double)(nx * ny * nz) * 13.0);
}


void init(int nx, int ny, int nz, int mgn, flt dx, flt dy, flt dz, flt *f)
{
    const flt kx = 2.0F * (flt)M_PI;
    const flt ky = kx;
    const flt kz = kx;

    std::for_each_n
      ( std::execution::par, boost::iterators::counting_iterator<int32_t>(0), nx*ny*(nz+2*mgn), [&](int iter)
    {
      int ix = iter;
      int k = (ix/(nx*ny)) - mgn;
      int j = (ix%(nx*ny)) / nx;
      int i = (ix%(nx*ny)) % nx;

      const flt x = dx * ((flt)i + 0.5F);
      const flt y = dy * ((flt)j + 0.5F);
      const flt z = dz * ((flt)k + 0.5F);

      f[ix] = 0.125 * (1.0 - cos(kx * x)) * (1.0 - cos(ky * y)) * (1.0 - cos(kz * z));
    });

    /*
#pragma omp parallel for collapse(3)
    for(int k = -mgn; k < nz + mgn; k++) {
        for(int j = 0; j < ny; j++) {
            for(int i = 0; i < nx; i++) {
                const int ix = nx * ny * (k + mgn) + nx * j + i;
                const flt x = dx * ((flt)i + 0.5F);
                const flt y = dy * ((flt)j + 0.5F);
                const flt z = dz * ((flt)k + 0.5F);

                f[ix] = 0.125 * (1.0 - cos(kx * x)) * (1.0 - cos(ky * y)) * (1.0 - cos(kz * z));
            }
        }
    }
     */
}

double err(double time, int nx, int ny, int nz, int mgn, flt dx, flt dy, flt dz, flt kappa, const flt *f)
{
    const flt kx = 2.0F * (flt)M_PI;
    const flt ky = kx;
    const flt kz = kx;

    const flt ax = expf(-kappa * (flt)time * (kx * kx));
    const flt ay = expf(-kappa * (flt)time * (ky * ky));
    const flt az = expf(-kappa * (flt)time * (kz * kz));

    double ferr =
    std::transform_reduce
    ( std::execution::par,
      boost::iterators::counting_iterator<int32_t>(0),
      boost::iterators::counting_iterator<int32_t>(nx*ny*nz),
      0.0, std::plus<>(), [&](int iter)
    {
      int ix = iter + nx*ny*mgn;
      int k = (ix/(nx*ny)) - mgn;
      int j = (ix%(nx*ny)) / nx;
      int i = (ix%(nx*ny)) % nx;

      const flt x = dx * ((flt)i + 0.5F);
      const flt y = dy * ((flt)j + 0.5F);
      const flt z = dz * ((flt)k + 0.5F);

      const flt f0 = 0.125 * (1.0 - ax * cos(kx * x)) * (1.0 - ay * cos(ky * y)) * (1.0 - az * cos(kz * z));

      const double diff = (double)f[ix] - (double)f0;
      return diff * diff;
    });

    /*    
    double ferr = 0.0;
#pragma omp parallel for reduction(+:ferr) collapse(3)
    for(int k = 0; k < nz; k++) {
        for(int j = 0; j < ny; j++) {
            for(int i = 0; i < nx; i++) {
                const int ix = nx * ny * (k + mgn) + nx * j + i;
                const flt x = dx * ((flt)i + 0.5F);
                const flt y = dy * ((flt)j + 0.5F);
                const flt z = dz * ((flt)k + 0.5F);

                const flt f0 = 0.125 * (1.0 - ax * cos(kx * x)) * (1.0 - ay * cos(ky * y)) * (1.0 - az * cos(kz * z));

                const double diff = (double)f[ix] - (double)f0;
                ferr += diff * diff;
            }
        }
    }
    */
    
    return ferr;
}
