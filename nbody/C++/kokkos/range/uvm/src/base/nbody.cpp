///
/// @file nbody.cpp
/// @author Yohei MIKI (Information Technology Center, The University of Tokyo)
/// @brief sample implementation of direct N-body simulation (orbit integration: 2nd-order leapfrog scheme)
///
/// @copyright Copyright (c) 2022 Yohei MIKI
///
/// The MIT License is applied to this software, see LICENSE.txt
///

#include <unistd.h>

#include <cmath>                               // std::fma
#include <cstdint>                             // int32_t
#include <iostream>                            // std::cout
#include <string>                              // std::string
#include <type_traits>                         // std::remove_const_t

///
/// @brief Kokkosライブラリ設定
///
#include <Kokkos_Core.hpp>

#include "common/cfg.hpp"
#include "common/conservatives.hpp"
#include "common/init.hpp"
#include "common/io.hpp"
#include "common/type.hpp"
#include "util/macro.hpp"
#include "util/timer.hpp"

///
/// @brief Kokkos::View設定
///
using KVpos1D = Kokkos::View<type::position*, Kokkos::CudaUVMSpace>;
using KVvel1D = Kokkos::View<type::velocity*, Kokkos::CudaUVMSpace>;
using KVacc1D = Kokkos::View<type::acceleration*, Kokkos::CudaUVMSpace>;

constexpr type::flt_acc newton = AS_FLT_ACC(1.0);  // gravitational constant

///
/// @brief calculate gravitational acceleration
///
/// @param[in] Ni number of i-particles
/// @param[in] ipos position of i-particles
/// @param[out] iacc acceleration of i-particles
/// @param[in] Nj number of j-particles
/// @param[in] jpos position of j-particles
/// @param[in] eps2 square of softening length
/// @param[in] ivel velocity of i-particles
///
static inline void calc_acc(const type::int_idx Ni, KVpos1D ipos, KVacc1D iacc, const type::int_idx Nj, KVpos1D jpos, const type::flt_pos eps2) {
  Kokkos::parallel_for("calc_acc", Ni,
    KOKKOS_LAMBDA(std::remove_const_t<decltype(Ni)> i) {
      // initialization
      const auto pi = ipos(i);
      std::remove_reference_t<decltype(iacc(0))> ai = {AS_FLT_ACC(0.0), AS_FLT_ACC(0.0), AS_FLT_ACC(0.0), AS_FLT_ACC(0.0)};

      // force evaluation
      for (std::remove_const_t<decltype(Nj)> j = 0; j < Nj; j++) {
        // load j-particle
        const auto pj = jpos(j);

        // calculate particle-particle interaction
        const auto dx = pj.x - pi.x;
        const auto dy = pj.y - pi.y;
        const auto dz = pj.z - pi.z;
        const auto r2 = eps2 + dx * dx + dy * dy + dz * dz;
        const auto r_inv = AS_FP_L(1.0) / std::sqrt(r2);
        const auto r2_inv = r_inv * r_inv;
        const auto alp = pj.w * (r_inv * r2_inv);

        // force accumulation
        ai.x += alp * dx;
        ai.y += alp * dy;
        ai.z += alp * dz;

#ifdef CALCULATE_POTENTIAL
        // gravitational potential
        ai.w += alp * r2;
#endif  // CALCULATE_POTENTIAL
      }
      iacc(i) = ai;
    });
  Kokkos::fence();
}

#ifndef BENCHMARK_MODE
///
/// @brief finalize calculation of gravitational acceleration
///
/// @param[in] Ni number of i-particles
/// @param[in,out] acc acceleration of i-particles
/// @param[in] pos position of i-particles
/// @param[in] eps_inv inverse of softening length
///
static inline void trim_acc(const type::int_idx Ni, KVacc1D acc
#ifdef CALCULATE_POTENTIAL
                            ,
                            KVpos1D pos, const type::flt_acc eps_inv
#endif  // CALCULATE_POTENTIAL
) {
  auto l_newton = newton;
  Kokkos::parallel_for("trim_acc", Ni,
    KOKKOS_LAMBDA(std::remove_const_t<decltype(Ni)> i) {
      // initialization
      auto ai = acc(i);

      ai.x *= l_newton;
      ai.y *= l_newton;
      ai.z *= l_newton;

#ifdef CALCULATE_POTENTIAL
      ai.w = (-ai.w + eps_inv * (pos(i).w)) * l_newton;
#endif  // CALCULATE_POTENTIAL

      acc(i) = ai;
    });
  Kokkos::fence();
}

///
/// @brief integrate velocity
///
/// @param[in] num number of N-body particles
/// @param[in,out] vel velocity of N-body particles
/// @param[in] acc acceleration of N-body particles
/// @param[in] dt time step
///
static inline void kick(const type::int_idx num, KVvel1D vel, KVacc1D acc, const type::flt_vel dt) {
  Kokkos::parallel_for("kick", num,
    KOKKOS_LAMBDA(std::remove_const_t<decltype(num)> i) {
      // initialization
      auto vi = vel(i);
      const auto ai = acc(i);

      vi.x += dt * ai.x;
      vi.y += dt * ai.y;
      vi.z += dt * ai.z;

      vel(i) = vi;
    });
  Kokkos::fence();
}

///
/// @brief integrate position
///
/// @param[in] num number of N-body particles
/// @param[in,out] pos position of N-body particles
/// @param[in] vel velocity of N-body particles
/// @param[in] dt time step
///
static inline void drift(const type::int_idx num, KVpos1D pos, KVvel1D vel, const type::flt_pos dt) {
  Kokkos::parallel_for("drift", num,
    KOKKOS_LAMBDA(std::remove_const_t<decltype(num)> i) {
      // initialization
      auto pi = pos(i);
      const auto vi = vel(i);

      pi.x += dt * vi.x;
      pi.y += dt * vi.y;
      pi.z += dt * vi.z;

      pos(i) = pi;
    });
  Kokkos::fence();
}

///
/// @brief integrate velocity in half-step backward
///
/// @param[in] num number of N-body particles
/// @param[in] vel_src velocity of N-body particles
/// @param[in] acc acceleration of N-body particles
/// @param[out] vel velocity of N-body particles
/// @param[in] dt time step
///
static inline void kick_backward_half(const type::int_idx num, KVvel1D vel_src, KVacc1D acc, KVvel1D vel, const type::flt_vel dt) {
  const auto dt_2 = AS_FLT_VEL(0.5) * dt;
  Kokkos::parallel_for("kick_backward_half", num,
    KOKKOS_LAMBDA(std::remove_const_t<decltype(num)> i) {
      // initialization
      auto vi = vel_src(i);
      const auto ai = acc(i);

      vi.x -= dt_2 * ai.x;
      vi.y -= dt_2 * ai.y;
      vi.z -= dt_2 * ai.z;

      vel(i) = vi;
    });
  Kokkos::fence();
}
#endif  // BENCHMARK_MODE

///
/// @brief allocate memory for N-body particles
///
/// @param[out] pos position of N-body particles
/// @param[out] vel velocity of N-body particles
/// @param[out] vel_tmp tentative array for velocity of N-body particles
/// @param[out] acc acceleration of N-body particles
/// @param[in] num number of N-body particles
///
static inline void allocate_Nbody_particles(KVpos1D pos, KVvel1D vel, KVvel1D vel_tmp, KVacc1D acc, const type::int_idx num) {
  const auto size = static_cast<size_t>(num);
  // zero-clear arrays (for safety of massless particles)
  constexpr std::remove_reference_t<decltype(pos(0))> p_zero = {AS_FLT_POS(0.0), AS_FLT_POS(0.0), AS_FLT_POS(0.0), AS_FLT_POS(0.0)};
  constexpr std::remove_reference_t<decltype(vel(0))> v_zero = {AS_FLT_VEL(0.0), AS_FLT_VEL(0.0), AS_FLT_VEL(0.0)};
  constexpr std::remove_reference_t<decltype(acc(0))> a_zero = {AS_FLT_ACC(0.0), AS_FLT_ACC(0.0), AS_FLT_ACC(0.0), AS_FLT_ACC(0.0)};
  // デバイス側で配列を初期化
  Kokkos::parallel_for("allocate_Nbody_particles", 
    size,
    KOKKOS_LAMBDA(std::remove_const_t<decltype(size)> i) {
    pos(i) = p_zero;
    vel(i) = v_zero;
    vel_tmp(i) = v_zero;
    acc(i) = a_zero;
  });
  Kokkos::fence();
}

///
/// @brief deallocate memory for N-body particles
///
/// @param[out] pos position of N-body particles
/// @param[out] vel velocity of N-body particles
/// @param[out] vel_tmp tentative array for velocity of N-body particles
/// @param[out] acc acceleration of N-body particles
///
static inline void release_Nbody_particles(KVpos1D pos, KVvel1D vel, KVvel1D vel_tmp, KVacc1D acc) {
}

int main(int argc, char** argv) {
  Kokkos::ScopeGuard guard(argc, argv);
  // record time-to-solution
  auto time_to_solution = util::timer();
  time_to_solution.start();

  // initialize the simulation
  auto cfg = config();
  cfg.configure(argc, argv);

  // read input parameters
  const auto file = cfg.get_file();
  const auto eps = cfg.get_eps();
  const auto M_tot = cfg.get_mass();
  const auto rad = cfg.get_radius();
  const auto virial = cfg.get_virial();
#ifdef BENCHMARK_MODE
  const auto [num_min, num_max, num_bin] = cfg.get_num();
#else   // BENCHMARK_MODE
  const auto num = cfg.get_num();
  const auto [ft, snapshot_interval] = cfg.get_time();
  const auto dt = cfg.get_dt();
#endif  // BENCHMARK_MODE
  const auto eps2 = eps * eps;

#ifndef BENCHMARK_MODE
  // initialize N-body simulation
  type::fp_m time = AS_FP_M(0.0);
  type::fp_m time_from_snapshot = AS_FP_M(0.0);
  int32_t step = 0;
  int32_t present = 0;
  auto previous = present;
  const auto snp_fin = static_cast<int32_t>(std::ceil(ft / snapshot_interval));
#ifdef CALCULATE_POTENTIAL
  const auto eps_inv = AS_FLT_ACC(1.0) / CAST2ACC(eps);
#endif  // CALCULATE_POTENTIAL
#endif  // BENCHMARK_MODE

#ifdef BENCHMARK_MODE
  const auto num_logbin = std::log2(num_max / num_min) / static_cast<double>((num_bin > 1) ? (num_bin - 1) : (num_bin));
  for (std::remove_const_t<decltype(num_bin)> i = 0; i < num_bin; i++) {
    const auto num = static_cast<type::int_idx>(std::nearbyint(num_min * std::exp2(static_cast<double>(i) * num_logbin)));
#endif  // BENCHMARK_MODE

    // memory allocation
    // ※配列宣言時はSeparateとUnifiedは共通
    KVpos1D pos("pos", num);
    KVvel1D vel("vel", num);
    KVvel1D vel_tmp("vel_tmp", num);
    KVacc1D acc("acc", num);
    allocate_Nbody_particles(pos, vel, vel_tmp, acc, num);

    // generate initial-condition
    init::set_uniform_sphere(num, pos.data(), vel.data(), M_tot, rad, virial, CAST2VEL(newton));

#ifndef BENCHMARK_MODE
    // record the conservation errors at the initial snapshot time
    calc_acc(num, pos, acc, num, pos, eps2);
    trim_acc(num, acc
#ifdef CALCULATE_POTENTIAL
             ,
             pos, eps_inv
#endif  // CALCULATE_POTENTIAL
    );
    auto error = conservatives();
    error.record(num, pos.data(), vel.data(), acc.data());

    // half-step integration for velocity
    kick(num, vel, acc, AS_FP_M(0.5) * dt);

    while (present < snp_fin) {
      step++;
      time_from_snapshot += dt;
      if (time_from_snapshot >= snapshot_interval) {
        present++;
      }
      DEBUG_PRINT(std::cout, "t = " << time + time_from_snapshot << ", step = " << step)

      // orbit integration (2nd-order leapfrog scheme)
      drift(num, pos, vel, dt);
      calc_acc(num, pos, acc, num, pos, eps2);
      trim_acc(num, acc
#ifdef CALCULATE_POTENTIAL
               ,
               pos, eps_inv
#endif  // CALCULATE_POTENTIAL
      );
      kick(num, vel, acc, dt);

      // record the conservation errors at every snapshot time
      if (present > previous) {
        previous = present;
        time_from_snapshot = AS_FP_M(0.0);
        time += snapshot_interval;
        kick_backward_half(num, vel, acc, vel_tmp, dt);
        error.record(num, pos.data(), vel_tmp.data(), acc.data());
      }
    }
#else   // BENCHMARK_MODE
  // launch benchmark
  auto timer = util::timer();
  timer.start();
  calc_acc(num, pos, acc, num, pos, eps2);
  timer.stop();
  auto elapsed = timer.get_elapsed_wall();
  int32_t iter = 1;

  // increase iteration counts if the measured time is too short
  const auto minimum_elapsed = cfg.get_minimum_elapsed_time();
  while (elapsed < minimum_elapsed) {
    // predict the iteration counts
    constexpr double booster = 1.25;  // additional safety-parameter to reduce rejection rate
    iter = static_cast<decltype(iter)>(std::exp2(std::ceil(std::log2(static_cast<double>(iter) * booster * minimum_elapsed / elapsed))));

    // re-execute the benchmark
    timer.clear();
    timer.start();
    for (decltype(iter) loop = 0; loop < iter; loop++) {
      calc_acc(num, pos, acc, num, pos, eps2);
    }
    timer.stop();
    elapsed = timer.get_elapsed_wall();
  }

  // finalize benchmark
  io::write_log(argv[0], elapsed, iter, num, file.c_str());
#endif  // BENCHMARK_MODE

    // memory deallocation
    release_Nbody_particles(pos, vel, vel_tmp, acc);

#ifdef BENCHMARK_MODE
  }
#else  // BENCHMARK_MODE

  time_to_solution.stop();
  io::write_log(argv[0], time_to_solution.get_elapsed_wall(), step, num, file.c_str()
#if defined(CALCULATE_POTENTIAL) && !defined(BENCHMARK_MODE)
                                                                             ,
                dt, error
#endif  // defined(CALCULATE_POTENTIAL) && !defined(BENCHMARK_MODE)
  );
#endif  // BENCHMARK_MODE

  return (0);
}
