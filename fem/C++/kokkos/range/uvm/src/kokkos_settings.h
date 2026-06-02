/***
	kokkos_settings.h
***/
#include <Kokkos_Core.hpp>

/***
	+-----------------+
	| Device settings |
	+-----------------+
***/
        using KOKKOS_DEV = Kokkos::Device<Kokkos::Cuda, Kokkos::CudaUVMSpace>;

/***
	+--------------------------+
	| Execution Space settings |
	+--------------------------+
***/
	using EXEC_SPACE = KOKKOS_DEV::execution_space;

/***
	+---------------+
	| View settings |
	+---------------+
***/
  	template <typename T> using View1D = Kokkos::View<T*, KOKKOS_DEV>;
  	template <typename T> using View2D = Kokkos::View<T**, KOKKOS_DEV>;
  	template <typename T> using View3D = Kokkos::View<T***, KOKKOS_DEV>;
  	template <typename T> using View4D = Kokkos::View<T****, KOKKOS_DEV>;

