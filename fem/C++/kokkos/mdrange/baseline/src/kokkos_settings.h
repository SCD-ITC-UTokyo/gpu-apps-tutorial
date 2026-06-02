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
