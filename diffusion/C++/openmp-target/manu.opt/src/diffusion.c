#include <stdio.h>
#include <math.h>
#include "misc.h"

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

    const int lnx = nx;
    const int lny = ny;
    const int lnz = nz + 2* mgn;
    const int ln  = lnx * lny * lnz;

#pragma omp target teams loop collapse(3) map(alloc: f[0:ln], fn[0:ln]) thread_limit(NTHREADS)
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
    
    return ((double)(nx * ny * nz) * 13.0);
}


void init(int nx, int ny, int nz, int mgn, flt dx, flt dy, flt dz, flt *f)
{
    const flt kx = 2.0F * (flt)M_PI;
    const flt ky = kx;
    const flt kz = kx;

    const int lnx = nx;
    const int lny = ny;
    const int lnz = nz + 2* mgn;
    const int ln  = lnx * lny * lnz;

#pragma omp target teams loop collapse(3) map(alloc: f[0:ln]) thread_limit(NTHREADS)
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
}

double err(double time, int nx, int ny, int nz, int mgn, flt dx, flt dy, flt dz, flt kappa, const flt *f)
{
    const flt kx = 2.0F * (flt)M_PI;
    const flt ky = kx;
    const flt kz = kx;

    const flt ax = expf(-kappa * (flt)time * (kx * kx));
    const flt ay = expf(-kappa * (flt)time * (ky * ky));
    const flt az = expf(-kappa * (flt)time * (kz * kz));

    double ferr = 0.0;

    const int lnx = nx;
    const int lny = ny;
    const int lnz = nz + 2* mgn;
    const int ln  = lnx * lny * lnz;

#pragma omp target teams loop collapse(3) map(alloc: f[0:ln]) reduction(+:ferr) thread_limit(NTHREADS)
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

    return ferr;
}
