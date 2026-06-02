

#include <stdio.h>
#include <math.h>
#include <Kokkos_Core.hpp>

double diffusion3d(int nx, int ny, int nz, int mgn, float dx, float dy, float dz, float dt, float kappa,
                   Kokkos::View<float*> f, Kokkos::View<float*> fn)
{
    const float ce = kappa * dt / (dx * dx);
    const float cw = ce;
    const float cn = kappa * dt / (dy * dy);
    const float cs = cn;
    const float ct = kappa * dt / (dz * dz);
    const float cb = ct;

    const float cc = 1.0F - (ce + cw + cn + cs + ct + cb);

    const int chunk = _CHUNK;

    using team_policy = Kokkos::TeamPolicy<>;
    using member_type = team_policy::member_type;

    const int league_size = (nz + chunk -1) / chunk;
    team_policy policy(league_size, Kokkos::AUTO);

    Kokkos::parallel_for("diffusion3d", policy,
      KOKKOS_LAMBDA(const member_type& team_member) {
        const int begin = team_member.league_rank() * chunk;
        const int end = (begin + chunk < nz) ? (begin + chunk) : nz;

        Kokkos::parallel_for(Kokkos::TeamThreadRange(team_member, begin, end),
          [&](const int k) {

            for (int j = 0; j < ny; j++) {
                for (int i = 0; i < nx; i++) {
                    const int ix = nx * ny * (k + mgn) + nx * j + i;
                    const int ip = i == nx - 1 ? ix : ix + 1;
                    const int im = i == 0      ? ix : ix - 1;
                    const int jp = j == ny - 1 ? ix : ix + nx;
                    const int jm = j == 0      ? ix : ix - nx;
                    const int kp = (k == nz - 1) ? ix : ix + nx*ny;
                    const int km = (k == 0     ) ? ix : ix - nx*ny;

                    fn(ix) = cc*f(ix) + ce*f(ip) + cw*f(im) + cn*f(jp) + cs*f(jm) + ct*f(kp) + cb*f(km);
                }
            }
          }
        );
    }
    );
    Kokkos::fence();

    return ((double)(nx * ny * nz) * 13.0);
}


void init(int nx, int ny, int nz, int mgn, float dx, float dy, float dz, Kokkos::View<float*> f)
{
    const float kx = 2.0F * (float)M_PI;
    const float ky = kx;
    const float kz = kx;

    const int chunk = _CHUNK;

    using team_policy = Kokkos::TeamPolicy<>;
    using member_type = team_policy::member_type;

    const int league_size = ((nz+mgn-(-mgn) + chunk -1)/chunk);
    team_policy policy(league_size, Kokkos::AUTO);

    Kokkos::parallel_for("init", policy,
      KOKKOS_LAMBDA(const member_type& team_member) {
        const int begin = team_member.league_rank() * chunk;
        const int end = (begin + chunk < (nz+mgn-(-mgn))) ? (begin + chunk) : (nz+mgn-(-mgn));

        Kokkos::parallel_for(Kokkos::TeamThreadRange(team_member, begin, end),
          [&](const int kk) {
        int k = kk - mgn;
        for(int j = 0; j < ny; j++) {
            for(int i = 0; i < nx; i++) {
                const int ix = nx * ny * (k + mgn) + nx * j + i;
                const float x = dx * ((float)i + 0.5F);
                const float y = dy * ((float)j + 0.5F);
                const float z = dz * ((float)k + 0.5F);

                f(ix) = 0.125F * (1.0F - cosf(kx * x)) * (1.0F - cosf(ky * y)) * (1.0F - cosf(kz * z));
            }
        }
      }
      );
    }
    );
    Kokkos::fence();
}

double err(double time, int nx, int ny, int nz, int mgn, float dx, float dy, float dz, float kappa, Kokkos::View<float*> f)
{
    const float kx = 2.0F * (float)M_PI;
    const float ky = kx;
    const float kz = kx;

    const float ax = expf(-kappa * (float)time * (kx * kx));
    const float ay = expf(-kappa * (float)time * (ky * ky));
    const float az = expf(-kappa * (float)time * (kz * kz));

    double ferr = 0.0;
    const int chunk = _CHUNK;

    using team_policy = Kokkos::TeamPolicy<>;
    using member_type = team_policy::member_type;

    const int league_size = (nz + chunk -1) / chunk;
    team_policy policy(league_size, Kokkos::AUTO);

    Kokkos::parallel_reduce("err", policy,
      KOKKOS_LAMBDA(const member_type& team_member, double& fsum){
        const int begin = team_member.league_rank() * chunk;
        const int end = (begin + chunk < nz) ? (begin + chunk) : nz;

        Kokkos::parallel_for(Kokkos::TeamThreadRange(team_member, begin, end),
          [&](const int k) {

        for(int j = 0; j < ny; j++) {
            for(int i = 0; i < nx; i++) {
                const int ix = nx * ny * (k + mgn) + nx * j + i;
                const float x = dx * ((float)i + 0.5F);
                const float y = dy * ((float)j + 0.5F);
                const float z = dz * ((float)k + 0.5F);

                const float f0 = 0.125F * (1.0F - ax * cosf(kx * x)) * (1.0F - ay * cosf(ky * y)) * (1.0F - az * cosf(kz * z));

                const double diff = (double)f(ix) - (double)f0;
                fsum += diff * diff;
            }
        }
      }
      );
      }, ferr
    );
    Kokkos::fence();

    return ferr;
}
