/***
	kokkos_settings.h
***/
#include <Kokkos_Core.hpp>

/***
	+---------------+
	| View settings |
	+---------------+
***/
  	template <typename T> using View1D = Kokkos::View<T*>;
  	template <typename T> using View2D = Kokkos::View<T**>;
  	template <typename T> using View3D = Kokkos::View<T***>;
  	template <typename T> using View4D = Kokkos::View<T****>;

/***
	+-----------------+
	| Policy settings |
	+-----------------+
***/
	using policy_1d = Kokkos::RangePolicy<>;

	//chunk size for RangePolicy
#ifdef _CHUNK_SIZE
        static int CHUNK_SIZE = _CHUNK_SIZE;
#else
        static int CHUNK_SIZE = 512;
#endif

	//chunk size for RangePolicy at mat_ass_main
#ifdef _CHUNK_SIZE_ASS
        static int CHUNK_SIZE_ASS = _CHUNK_SIZE_ASS;
#else
        static int CHUNK_SIZE_ASS = 32;
#endif
