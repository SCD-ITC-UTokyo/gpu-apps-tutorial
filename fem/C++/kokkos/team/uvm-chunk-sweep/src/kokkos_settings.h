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

/***
	+-----------------+
	| Policy settings |
	+-----------------+
***/
	using team_policy = Kokkos::TeamPolicy<EXEC_SPACE>;
	using member_type = team_policy::member_type;

	//chunk size for TeamPolicy
#ifdef _CHUNK_SIZE
        static int CHUNK_SIZE = _CHUNK_SIZE;
#else
        static int CHUNK_SIZE = 512;
#endif

	//chunk size for TeamPolicy at mat_ass_main
#ifdef _CHUNK_SIZE_ASS
        static int CHUNK_SIZE_ASS = _CHUNK_SIZE_ASS;
#else
        static int CHUNK_SIZE_ASS = 32;
#endif

