

#include <stdio.h>
#include <math.h>
#include <Kokkos_Core.hpp>

double diffusion3d(int nx, int ny, int nz, int mgn, float dx, float dy, float dz, float dt, float kappa,
                   Kokkos::View<float*, Kokkos::CudaUVMSpace> f, Kokkos::View<float*, Kokkos::CudaUVMSpace> fn)
{
    const float ce = kappa * dt / (dx * dx);
    const float cw = ce;
    const float cn = kappa * dt / (dy * dy);
    const float cs = cn;
    const float ct = kappa * dt / (dz * dz);
    const float cb = ct;

    const float cc = 1.0F - (ce + cw + cn + cs + ct + cb);

    using mdrange_policy = Kokkos::MDRangePolicy<Kokkos::Rank<3>>;
    Kokkos::parallel_for("diffusion3d", mdrange_policy({0,0,0}, {nz,ny,nx}),
      KOKKOS_LAMBDA(const int k, const int j, const int i) { 
        const int ix = nx * ny * (k + mgn) + nx * j + i;
        const int ip = i == nx - 1 ? ix : ix + 1;
        const int im = i == 0      ? ix : ix - 1;
        const int jp = j == ny - 1 ? ix : ix + nx;
        const int jm = j == 0      ? ix : ix - nx;
        const int kp = (k == nz - 1) ? ix : ix + nx*ny;
        const int km = (k == 0     ) ? ix : ix - nx*ny;

        fn(ix) = cc*f(ix) + ce*f(ip) + cw*f(im) + cn*f(jp) + cs*f(jm) + ct*f(kp) + cb*f(km);
      }
    );
    Kokkos::fence();

    return ((double)(nx * ny * nz) * 13.0);
}


void init(int nx, int ny, int nz, int mgn, float dx, float dy, float dz, Kokkos::View<float*, Kokkos::CudaUVMSpace> f)
{
    const float kx = 2.0F * (float)M_PI;
    const float ky = kx;
    const float kz = kx;

    using mdrange_policy = Kokkos::MDRangePolicy<Kokkos::Rank<3>>;
    Kokkos::parallel_for("init", mdrange_policy({0,0,0}, {nz+mgn-(-mgn), ny, nx}),
      KOKKOS_LAMBDA(const int kk, const int j, const int i) {
        const int k = kk -mgn;
        const int ix = nx * ny * (k + mgn) + nx * j + i;
        const float x = dx * ((float)i + 0.5F);
        const float y = dy * ((float)j + 0.5F);
        const float z = dz * ((float)k + 0.5F);

        f(ix) = 0.125F * (1.0F - cosf(kx * x)) * (1.0F - cosf(ky * y)) * (1.0F - cosf(kz * z));
      }
    );
    Kokkos::fence();
}

double err(double time, int nx, int ny, int nz, int mgn, float dx, float dy, float dz, float kappa, Kokkos::View<float*, Kokkos::CudaUVMSpace> f)
{
    const float kx = 2.0F * (float)M_PI;
    const float ky = kx;
    const float kz = kx;

    const float ax = expf(-kappa * (float)time * (kx * kx));
    const float ay = expf(-kappa * (float)time * (ky * ky));
    const float az = expf(-kappa * (float)time * (kz * kz));

    double ferr = 0.0;

    using mdrange_policy = Kokkos::MDRangePolicy<Kokkos::Rank<3>>;
    Kokkos::parallel_reduce("err", mdrange_policy({0,0,0}, {nz,ny,nx}),
      KOKKOS_LAMBDA(const int k, const int j, const int i, double& fsum){
        const int ix = nx * ny * (k + mgn) + nx * j + i;
        const float x = dx * ((float)i + 0.5F);
        const float y = dy * ((float)j + 0.5F);
        const float z = dz * ((float)k + 0.5F);

        const float f0 = 0.125F * (1.0F - ax * cosf(kx * x)) * (1.0F - ay * cosf(ky * y)) * (1.0F - az * cosf(kz * z));

        const double diff = (double)f(ix) - (double)f0;
        fsum += diff * diff;
      }, ferr
    );
    Kokkos::fence();

    return ferr;
}
