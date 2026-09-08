///
/// @file common/io.hpp
/// @author Yohei MIKI (Information Technology Center, The University of Tokyo)
/// @brief file IO for direct N-body simulation
///
/// @copyright Copyright (c) 2022 Yohei MIKI
///
/// The MIT License is applied to this software, see LICENSE.txt
///
#ifndef COMMON_IO_HPP
#define COMMON_IO_HPP

#include <sys/stat.h>   // mkdir, stat
#include <sys/types.h>  // mode_t

#include <fstream>  // std::ofstream
#include <iomanip>  // std::setprecision

#include "common/conservatives.hpp"
#include "common/type.hpp"

namespace io {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wglobal-constructors"
#pragma GCC diagnostic ignored "-Wexit-time-destructors"
static const std::string folder_log = "log/";  // folder for output log files
#pragma GCC diagnostic pop

// assume number of floating-point operations per interaction
// FP_M: 3 subtractions, 3 additions = 6 Flops
// FP_L: 3 FMA operations, 6 multiplications = 12 Flops
// FP_L: 1 reciprocal square root: assume as 4 Flops (estimation based on NVIDIA A100: 4 cycles for rsqrtf)
#ifdef CALCULATE_POTENTIAL
// FP_L: additional 1 multiplication for potential calculation
// FP_M: additional 1 addition for potential calculation
constexpr double FLOPS_PER_INTERACTION = 24.0;  // assumed number of floating-point operations per interaction
#else   // CALCULATE_POTENTIAL
constexpr double FLOPS_PER_INTERACTION = 22.0;  // assumed number of floating-point operations per interaction
#endif  // CALCULATE_POTENTIAL

///
/// @brief write the run log (log/<filename>_run.csv)
///
/// @param[in] exec name of the executed binary
/// @param[in] elapsed elapsed time of the simulation
/// @param[in] step number of steps
/// @param[in] num number of N-body particles
/// @param[in] filename name of the simulation
/// @param[in] dt time step of the simulation
/// @param[in] error error of conservatives in the simulation
///
static inline void write_log(const char *const exec, const double elapsed, const int32_t step, const type::int_idx num, const char *const filename
#ifndef BENCHMARK_MODE
#ifdef CALCULATE_POTENTIAL
                             ,
                             const type::fp_m dt,
                             const conservatives &error
#endif  // CALCULATE_POTENTIAL
#endif  // BENCHMARK_MODE
) {
  mkdir(folder_log.c_str(), 0755);  // 出力先が無いと黙って書き込みに失敗する。既存なら EEXIST で無視される
  const auto note = folder_log + filename + "_run.csv";
  struct stat note_stat {};
  const auto exist = (stat(note.c_str(), &note_stat) == 0);
  std::ofstream stats(note, std::ios::app);
  stats << std::scientific << std::setprecision(16);
  if (!exist) {
    stats << "exec,N,time[s],step,time_per_step[s],interactions_per_sec,Flop/s,FP_L,FP_M";
#if defined(CALCULATE_POTENTIAL) && !defined(BENCHMARK_MODE)
    stats << ",dt";
    stats << ",energy_error_worst,energy_error_final,virial_ratio_final";
#endif  // defined(CALCULATE_POTENTIAL) && !defined(BENCHMARK_MODE)
    stats << std::endl;
  }
  stats << exec;
  stats << "," << num;
  stats << "," << elapsed;
  stats << "," << step;
  stats << "," << elapsed / static_cast<double>(step);
  const auto N_pairs = static_cast<double>(num) * static_cast<double>(num) * static_cast<double>(step);
  const auto pairs_per_sec = N_pairs / elapsed;
  stats << "," << pairs_per_sec;
  stats << "," << pairs_per_sec * FLOPS_PER_INTERACTION;
  stats << "," << FP_L;
  stats << "," << FP_M;
#if defined(CALCULATE_POTENTIAL) && !defined(BENCHMARK_MODE)
  stats << "," << dt;
  stats << "," << error.get_energy_error_worst();
  stats << "," << error.get_energy_error_final();
  stats << "," << error.get_virial_ratio_final();
#endif  // defined(CALCULATE_POTENTIAL) && !defined(BENCHMARK_MODE)
  stats << std::endl;
  stats.close();
}

}  // namespace io

#endif  // COMMON_IO_HPP
