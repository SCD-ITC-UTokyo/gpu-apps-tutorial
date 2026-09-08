///
/// @file util/timer.hpp
/// @author Yohei MIKI (Information Technology Center, The University of Tokyo)
/// @brief utility to measure elapsed time
///
/// @copyright Copyright (c) 2022 Yohei MIKI
///
/// The MIT License is applied to this software, see LICENSE.txt
///
#ifndef UTIL_TIMER_HPP
#define UTIL_TIMER_HPP

// boost::timer::cpu_timer から標準機能へ置き換え。
//   wall clock : std::chrono::steady_clock (cpu_timer の .wall と等価)
//   user CPU   : getrusage(RUSAGE_SELF) (cpu_timer の .user と等価)
// diffusion / fem も getrusage を使っており、3 アプリで計測方法が揃う。
#include <sys/resource.h>  // getrusage

#include <chrono>  // std::chrono::steady_clock

///
/// @brief utility tools
///
namespace util {
///
/// @brief measure elapsed time
///
class timer {
 public:
  ///
  /// @brief Construct a new timer object
  ///
  timer() = default;

  ///
  /// @brief read this.elapsed_wall
  ///
  /// @return this.elapsed_wall
  ///
  [[nodiscard]] inline auto get_elapsed_wall() const noexcept(true) {
    return (elapsed_wall);
  }

  ///
  /// @brief read this.elapsed_user
  ///
  /// @return this.elapsed_user
  ///
  [[nodiscard]] inline auto get_elapsed_user() const noexcept(true) {
    return (elapsed_user);
  }

  ///
  /// @brief start simple measurement
  ///
  inline void start() noexcept(true) {
    wall_start = std::chrono::steady_clock::now();
    user_start = get_user_time();
  }

  ///
  /// @brief stop simple measurement
  ///
  inline void stop() noexcept(true) {
    const auto wall_stop = std::chrono::steady_clock::now();
    elapsed_wall += std::chrono::duration<double>(wall_stop - wall_start).count();
    elapsed_user += get_user_time() - user_start;
  }

  ///
  /// @brief clear the previous measurement
  ///
  inline void clear() noexcept(true) {
    elapsed_wall = 0.0;
    elapsed_user = 0.0;
  }

 private:
  ///
  /// @brief user CPU time of this process in seconds
  ///
  static inline double get_user_time() noexcept(true) {
    struct rusage usage {};
    getrusage(RUSAGE_SELF, &usage);
    return (static_cast<double>(usage.ru_utime.tv_sec) + static_cast<double>(usage.ru_utime.tv_usec) * 1.0e-6);
  }

  std::chrono::steady_clock::time_point wall_start{};  // beginning of the current measurement
  double user_start = 0.0;                             // user CPU time at the beginning of the current measurement
  double elapsed_wall = 0.0;                           // elapsed time as wall clock time
  double elapsed_user = 0.0;                           // elapsed time as user CPU time
};
}  // namespace util

#endif  // UTIL_TIMER_HPP
