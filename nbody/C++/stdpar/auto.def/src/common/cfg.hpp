///
/// @file common/cfg.hpp
/// @author Yohei MIKI (Information Technology Center, The University of Tokyo)
/// @brief configure direct N-body simulation
///
/// @copyright Copyright (c) 2022 Yohei MIKI
///
/// The MIT License is applied to this software, see LICENSE.txt
///
#ifndef COMMON_CFG_HPP
#define COMMON_CFG_HPP

// boost::program_options から自作パーサへ置き換え (外部ライブラリ依存を無くすため)。
// 受け付ける形式: --key=value / --key value / --help (-h)
// 未知のオプションはエラー終了する (boost::program_options と同じ挙動)。
#include <cstdlib>   // std::exit, std::stod, std::stoi, std::stoul
#include <iostream>  // std::cout, std::cerr
#include <string>    // std::string
#include <tuple>     // std::make_tuple, std::make_pair
#include <utility>   // std::pair
#include <vector>    // std::vector

#include "common/type.hpp"

class config {
 public:
  ///
  /// @brief Construct a new config object
  ///
  config() = default;

  void configure(const int32_t argc, const char *const *const argv) {
    for (int32_t i = 1; i < argc; i++) {
      std::string arg(argv[i]);
      if ((arg == "--help") || (arg == "-h")) {
        show_help();
        std::exit(EXIT_SUCCESS);
      }
      if (arg.compare(0, 2, "--") != 0) {
        std::cerr << "ERROR: unrecognised argument \"" << arg << "\"" << std::endl;
        show_help();
        std::exit(EXIT_FAILURE);
      }
      std::string key = arg.substr(2);
      std::string val;
      const auto sep = key.find('=');
      if (sep != std::string::npos) {
        val = key.substr(sep + 1);
        key = key.substr(0, sep);
      } else if ((i + 1) < argc) {
        val = argv[++i];
      }
      if (!assign(key, val)) {
        std::cerr << "ERROR: unrecognised option \"--" << key << "\"" << std::endl;
        show_help();
        std::exit(EXIT_FAILURE);
      }
    }
  }

 private:
  ///
  /// @brief list of the accepted options (name, description)
  ///
  [[nodiscard]] static inline auto option_list() {
    return std::vector<std::pair<const char *, const char *>>{
        {"file", "Name of the output files (default: collapse)"},
        {"softening", "Softening length (Plummer softening)"},
        {"mass", "Total mass of the system"},
        {"radius", "Radius of the initial sphere"},
        {"virial", "Virial ratio of the initial condition"},
#ifdef BENCHMARK_MODE
        {"num_min", "Minimum number of N-body particles"},
        {"num_max", "Maximum number of N-body particles"},
        {"num_bin", "Number of logarithmic grids about number of N-body particles"},
        {"elapse_min", "Minimum elapsed time for each measurement"},
#else   // BENCHMARK_MODE
        {"num", "Number of N-body particles"},
        {"finish", "Final time of the simulation"},
        {"interval", "Interval between snapshots"},
        {"time_step", "Time step in the simulation"},
#endif  // BENCHMARK_MODE
        {"help,h", "Help"}};
  }

  static inline void show_help() {
    std::cout << "List of options:" << std::endl;
    for (const auto &[name, desc] : option_list()) {
      std::cout << "  --" << name << "\n      " << desc << std::endl;
    }
  }

  ///
  /// @brief store one option; returns false when the key is unknown
  ///
  [[nodiscard]] inline bool assign(const std::string &key, const std::string &val) {
    if (key == "file") {
      file = val;
    } else if (key == "softening") {
      eps = static_cast<type::flt_pos>(std::stod(val));
    } else if (key == "mass") {
      M_tot = static_cast<type::flt_pos>(std::stod(val));
    } else if (key == "radius") {
      radius = static_cast<type::flt_pos>(std::stod(val));
    } else if (key == "virial") {
      virial = static_cast<type::flt_vel>(std::stod(val));
#ifdef BENCHMARK_MODE
    } else if (key == "num_min") {
      num_min = std::stod(val);
    } else if (key == "num_max") {
      num_max = std::stod(val);
    } else if (key == "num_bin") {
      num_bin = static_cast<int32_t>(std::stoi(val));
    } else if (key == "elapse_min") {
      elapse_min = std::stod(val);
#else   // BENCHMARK_MODE
    } else if (key == "num") {
      num = static_cast<type::int_idx>(std::stoul(val));
    } else if (key == "finish") {
      ft = static_cast<type::fp_m>(std::stod(val));
    } else if (key == "interval") {
      interval = static_cast<type::fp_m>(std::stod(val));
    } else if (key == "time_step") {
      dt = static_cast<type::fp_m>(std::stod(val));
#endif  // BENCHMARK_MODE
    } else {
      return (false);
    }
    return (true);
  }

 public:
    // accessors
  ///
  /// @brief Get the file object
  ///
  /// @return name of the simulation
  ///
  [[nodiscard]] inline auto get_file() const noexcept(true) {
    return (file);
  }
  ///
  /// @brief Get the eps object
  ///
  /// @return softening length
  ///
  [[nodiscard]] constexpr auto get_eps() const noexcept(true) {
    return (eps);
  }
  ///
  /// @brief Get the mass object
  ///
  /// @return total mass of the system
  ///
  [[nodiscard]] constexpr auto get_mass() const noexcept(true) {
    return (M_tot);
  }
  ///
  /// @brief Get the radius object
  ///
  /// @return radius of the initial sphere
  ///
  [[nodiscard]] constexpr auto get_radius() const noexcept(true) {
    return (radius);
  }
  ///
  /// @brief Get the virial object
  ///
  /// @return Virial ratio of the initial condition
  ///
  [[nodiscard]] constexpr auto get_virial() const noexcept(true) {
    return (virial);
  }
  ///
  /// @brief Get the num object
  ///
  /// @return number of N-body particles
  ///
  [[nodiscard]] constexpr auto get_num() const noexcept(true) {
#ifdef BENCHMARK_MODE
    return (std::make_tuple(num_min, num_max, num_bin));
#else   // BENCHMARK_MODE
    return (num);
#endif  // BENCHMARK_MODE
  }
#ifdef BENCHMARK_MODE
  ///
  /// @brief Get the elapse_min object
  ///
  /// @return minimum elapsed time for each measurement
  ///
  [[nodiscard]] constexpr auto get_minimum_elapsed_time() const noexcept(true) {
    return (elapse_min);
  }
#else  // BENCHMARK_MODE
  ///
  /// @brief Get the time object
  ///
  /// @return final time of the simulation and interval between snapshots
  ///
  [[nodiscard]] constexpr auto get_time() const noexcept(true) {
    return (std::make_pair(ft, interval));
  }
  ///
  /// @brief Get the dt object
  ///
  /// @return time step in the simulation
  ///
  [[nodiscard]] constexpr auto get_dt() const noexcept(true) {
    return (dt);
  }
#endif  // BENCHMARK_MODE

 private:
  std::string file = "collapse";           // name of the simulation
  type::flt_pos eps = AS_FLT_POS(1.5625e-2);     // softening length
  type::flt_pos M_tot = AS_FLT_POS(1.0);   // total mass of the system
  type::flt_pos radius = AS_FLT_POS(1.0);  // radius of the initial sphere
  type::flt_vel virial = AS_FLT_VEL(0.2);  // Virial ratio of the initial condition
#ifdef BENCHMARK_MODE
  double num_min = 1024.0;     // minimum number of N-body particles
  double num_max = 4.0 * 1024.0 * 1024.0;     // maximum number of N-body particles
  int32_t num_bin = 13;      // number of logarithmic grids about number of N-body particles
  double elapse_min = 1.0;  // minimum elapsed time for each measurement
#else                       // BENCHMARK_MODE
  type::int_idx num = 1031U;              // number of N-body particles
  type::fp_m ft = AS_FP_M(10.0);        // final time of the simulation
  type::fp_m interval = AS_FP_M(0.125);  // interval between snapshots
  type::fp_m dt = AS_FP_M(7.8125e-3);  // time step in the simulation
#endif  // BENCHMARK_MODE
};

#endif  // COMMON_CFG_HPP
