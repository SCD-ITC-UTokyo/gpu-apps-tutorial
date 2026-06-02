#include <boost/math/constants/constants.hpp>  // boost::math::constants::two_pi
#include "init.hpp"
#include "init.h"

extern "C"
{
  void init_set_uniform_sphere
  ( const int num, position pos[], velocity vel[],
    const flt_pos Mtot, const flt_pos rad, const flt_vel virial,
    const flt_vel newton )
  {
    init::set_uniform_sphere
      ( num, (type::position*)pos, (type::velocity*)vel,
	Mtot, rad, virial, newton );
  }
}
