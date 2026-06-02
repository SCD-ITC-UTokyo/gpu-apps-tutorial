
#ifndef DIFFUSION_H
#define DIFFUSION_H
#include <Kokkos_Core.hpp>


double diffusion3d(int nx, int ny, int nz, int mgn, float dx, float dy, float dz, float dt, float kappa,
                   Kokkos::View<float*, Kokkos::CudaUVMSpace> f, Kokkos::View<float*, Kokkos::CudaUVMSpace> fn);
void init(int nx, int ny, int nz, int mgn, float dx, float dy, float dz, Kokkos::View<float*, Kokkos::CudaUVMSpace> f);
double err(double time, int nx, int ny, int nz, int mgn, float dx, float dy, float dz, float kappa, Kokkos::View<float*, Kokkos::CudaUVMSpace> f);



#endif /* DIFFUSION_H */
