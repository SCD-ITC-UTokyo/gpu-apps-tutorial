///
/// @file common/conservatives.hpp
/// @author Yohei MIKI (Information Technology Center, The University of Tokyo)
/// @brief evaluate conservatives in direct N-body simulation
///
/// @copyright Copyright (c) 2022 Yohei MIKI
///
/// The MIT License is applied to this software, see LICENSE.txt
///
#ifndef COMMON_CONSERVATIVES_HPP
#define COMMON_CONSERVATIVES_HPP

#include <type_traits>  // std::remove_const_t

#include "common/type.hpp"

class conservatives {
 public:
  ///
  /// @brief Construct a new conservatives object
  ///
  conservatives() = default;

  ///
  /// @brief record the conservation errors
  ///
  /// The snapshot output used to update the errors as a side effect; it was
  /// removed together with the HDF5 output, so this is now the entry point
  /// that keeps energy_error_worst / energy_error_final / virial_ratio_final
  /// (reported by io::write_log) up to date.
  ///
  /// @param[in] num number of N-body particles
  /// @param[in] pos position of N-body particles
  /// @param[in] vel velocity of N-body particles
  /// @param[in] acc acceleration of N-body particles
  ///
  inline void record(const type::int_idx num, const type::position *const pos, const type::velocity *const vel, const type::acceleration *const acc) {
    calc(num, pos, vel, acc);
  }

  // accessors
  ///
  /// @brief Get the energy error at the end of the simulation
  ///
  /// @return energy error at the end of the simulation
  ///
  [[nodiscard]] inline auto get_energy_error_final() const noexcept(true) {
    return (E_tot_err);
  }
  ///
  /// @brief Get the worst energy error of the simulation
  ///
  /// @return the worst energy error of the simulation
  ///
  [[nodiscard]] inline auto get_energy_error_worst() const noexcept(true) {
    return (E_tot_worst);
  }
  ///
  /// @brief Get the virial ratio at the end of the simulation
  ///
  /// @return virial ratio at the end of the simulation
  ///
  [[nodiscard]] inline auto get_virial_ratio_final() const noexcept(true) {
    return (virial);
  }

 private:
  ///
  /// @brief calculate conservatives
  ///
  /// @param[in] num number of N-body particles
  /// @param[in] pos position of N-body particles
  /// @param[in] vel velocity of N-body particles
  /// @param[in] acc acceleration of N-body particles
  ///
  inline void calc(const type::int_idx num, const type::position *const pos, const type::velocity *const vel, [[maybe_unused]] const type::acceleration *const acc) {
    // calculate energy and momentum
#ifdef CALCULATE_POTENTIAL
    E_kin = AS_FP_H(0.0);
    E_pot = AS_FP_H(0.0);
#endif  // CALCULATE_POTENTIAL
    px = AS_FP_H(0.0);
    py = AS_FP_H(0.0);
    pz = AS_FP_H(0.0);
    Lx = AS_FP_H(0.0);
    Ly = AS_FP_H(0.0);
    Lz = AS_FP_H(0.0);
#pragma omp parallel for reduction(+ : px, py, pz, Lx, Ly, Lz, E_kin, E_pot)
    for (std::remove_const_t<decltype(num)> ii = 0U; ii < num; ii++) {
      const auto pi = pos[ii];
      const auto xi = type::cast2fp_h(pi.x);
      const auto yi = type::cast2fp_h(pi.y);
      const auto zi = type::cast2fp_h(pi.z);
      const auto mi = type::cast2fp_h(pi.w);
      px += mi * xi;
      py += mi * yi;
      pz += mi * zi;

      const auto vi = vel[ii];
      const auto vx = type::cast2fp_h(vi.x);
      const auto vy = type::cast2fp_h(vi.y);
      const auto vz = type::cast2fp_h(vi.z);
      Lx += mi * (yi * vz - zi * vy);
      Ly += mi * (zi * vx - xi * vz);
      Lz += mi * (xi * vy - yi * vx);

#ifdef CALCULATE_POTENTIAL
      E_kin += mi * (vx * vx + vy * vy + vz * vz);
      E_pot += mi * type::cast2fp_h(acc[ii].w);
#endif  // CALCULATE_POTENTIAL
    }
#ifdef CALCULATE_POTENTIAL
    E_kin = AS_FP_H(0.5) * E_kin;
    E_pot = AS_FP_H(0.5) * E_pot;
    E_tot = E_kin + E_pot;
    virial = -E_kin / E_pot;
#endif  // CALCULATE_POTENTIAL

    if (initialized) {
      // evaluate error of conservatives
#ifdef CALCULATE_POTENTIAL
      E_tot_err = (E_tot * inv_E_tot_ini - AS_FP_H(1.0));
#endif  // CALCULATE_POTENTIAL
      px_err = px - px_ini;
      py_err = py - py_ini;
      pz_err = pz - pz_ini;
      Lx_err = Lx - Lx_ini;
      Ly_err = Ly - Ly_ini;
      Lz_err = Lz - Lz_ini;

// record the worst error
#ifdef CALCULATE_POTENTIAL
      if (std::abs(E_tot_err) > std::abs(E_tot_worst)) {
        E_tot_worst = E_tot_err;
      }
#endif  // CALCULATE_POTENTIAL
      if (std::abs(px_err) > std::abs(px_worst)) {
        px_worst = px_err;
      }
      if (std::abs(py_err) > std::abs(py_worst)) {
        py_worst = py_err;
      }
      if (std::abs(pz_err) > std::abs(pz_worst)) {
        pz_worst = pz_err;
      }
      if (std::abs(Lx_err) > std::abs(Lx_worst)) {
        Lx_worst = Lx_err;
      }
      if (std::abs(Ly_err) > std::abs(Ly_worst)) {
        Ly_worst = Ly_err;
      }
      if (std::abs(Lz_err) > std::abs(Lz_worst)) {
        Lz_worst = Lz_err;
      }
    } else {
#ifdef CALCULATE_POTENTIAL
      inv_E_tot_ini = AS_FP_H(1.0) / E_tot;
#endif  // CALCULATE_POTENTIAL
      px_ini = px;
      py_ini = py;
      pz_ini = pz;
      Lx_ini = Lx;
      Ly_ini = Ly;
      Lz_ini = Lz;
      initialized = true;
    }
  }

  type::fp_h E_kin = AS_FP_H(0.0);          // kinetic energy
  type::fp_h E_pot = AS_FP_H(0.0);          // potential energy
  type::fp_h E_tot = AS_FP_H(0.0);          // total energy
  type::fp_h virial = AS_FP_H(0.0);         // virial ratio
  type::fp_h px = AS_FP_H(0.0);             // x-component of momentum
  type::fp_h py = AS_FP_H(0.0);             // y-component of momentum
  type::fp_h pz = AS_FP_H(0.0);             // z-component of momentum
  type::fp_h Lx = AS_FP_H(0.0);             // x-component of angular momentum
  type::fp_h Ly = AS_FP_H(0.0);             // y-component of angular momentum
  type::fp_h Lz = AS_FP_H(0.0);             // z-component of angular momentum
  type::fp_h inv_E_tot_ini = AS_FP_H(1.0);  // inverse of total energy at the initial step
  type::fp_h px_ini = AS_FP_H(0.0);         // x-component of momentum at the initial step
  type::fp_h py_ini = AS_FP_H(0.0);         // y-component of momentum at the initial step
  type::fp_h pz_ini = AS_FP_H(0.0);         // z-component of momentum at the initial step
  type::fp_h Lx_ini = AS_FP_H(0.0);         // x-component of angular momentum at the initial step
  type::fp_h Ly_ini = AS_FP_H(0.0);         // y-component of angular momentum at the initial step
  type::fp_h Lz_ini = AS_FP_H(0.0);         // z-component of angular momentum at the initial step
  type::fp_h E_tot_err = AS_FP_H(0.0);      // relative error for total energy
  type::fp_h px_err = AS_FP_H(0.0);         // absolute error for x-component of momentum
  type::fp_h py_err = AS_FP_H(0.0);         // absolute error for y-component of momentum
  type::fp_h pz_err = AS_FP_H(0.0);         // absolute error for z-component of momentum
  type::fp_h Lx_err = AS_FP_H(0.0);         // absolute error for x-component of angular momentum
  type::fp_h Ly_err = AS_FP_H(0.0);         // absolute error for y-component of angular momentum
  type::fp_h Lz_err = AS_FP_H(0.0);         // absolute error for z-component of angular momentum
  type::fp_h E_tot_worst = AS_FP_H(0.0);    // the worst relative error for total energy
  type::fp_h px_worst = AS_FP_H(0.0);       // the worst absolute error for x-component of momentum
  type::fp_h py_worst = AS_FP_H(0.0);       // the worst absolute error for y-component of momentum
  type::fp_h pz_worst = AS_FP_H(0.0);       // the worst absolute error for z-component of momentum
  type::fp_h Lx_worst = AS_FP_H(0.0);       // the worst absolute error for x-component of angular momentum
  type::fp_h Ly_worst = AS_FP_H(0.0);       // the worst absolute error for y-component of angular momentum
  type::fp_h Lz_worst = AS_FP_H(0.0);       // the worst absolute error for z-component of angular momentum
  bool initialized = false;                 // true if the conservatives are initialized
};

#endif  // COMMON_CONSERVATIVES_HPP
