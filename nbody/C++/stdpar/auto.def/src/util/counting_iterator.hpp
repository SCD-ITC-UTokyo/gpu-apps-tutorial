///
/// @file util/counting_iterator.hpp
/// @brief boost::iterators::counting_iterator の最小代替 (整数列を走る random access iterator)
///
/// std::execution::par を使う stdpar 実装は「0, 1, 2, ... と数える iterator」だけのために
/// Boost に依存していた。C++17 には相当物が無いため (C++20 の std::views::iota が該当)、
/// 必要な機能だけを実装した POD を用意して外部依存を無くす。
///
#ifndef UTIL_COUNTING_ITERATOR_HPP
#define UTIL_COUNTING_ITERATOR_HPP

#include <cstddef>   // std::ptrdiff_t
#include <iterator>  // std::random_access_iterator_tag

namespace util {
template <typename T>
class counting_iterator {
 public:
  using iterator_category = std::random_access_iterator_tag;
  using value_type = T;
  using difference_type = std::ptrdiff_t;
  using pointer = const T *;
  using reference = T;

  counting_iterator() = default;
  explicit counting_iterator(const T val) : val_(val) {}

  reference operator*() const { return (val_); }
  reference operator[](const difference_type n) const { return (static_cast<T>(val_ + n)); }

  counting_iterator &operator++() { ++val_; return (*this); }
  counting_iterator operator++(int) { auto tmp = *this; ++val_; return (tmp); }
  counting_iterator &operator--() { --val_; return (*this); }
  counting_iterator operator--(int) { auto tmp = *this; --val_; return (tmp); }
  counting_iterator &operator+=(const difference_type n) { val_ = static_cast<T>(val_ + n); return (*this); }
  counting_iterator &operator-=(const difference_type n) { val_ = static_cast<T>(val_ - n); return (*this); }

  friend counting_iterator operator+(counting_iterator it, const difference_type n) { it += n; return (it); }
  friend counting_iterator operator+(const difference_type n, counting_iterator it) { it += n; return (it); }
  friend counting_iterator operator-(counting_iterator it, const difference_type n) { it -= n; return (it); }
  friend difference_type operator-(const counting_iterator &a, const counting_iterator &b) {
    return (static_cast<difference_type>(a.val_) - static_cast<difference_type>(b.val_));
  }

  friend bool operator==(const counting_iterator &a, const counting_iterator &b) { return (a.val_ == b.val_); }
  friend bool operator!=(const counting_iterator &a, const counting_iterator &b) { return (a.val_ != b.val_); }
  friend bool operator<(const counting_iterator &a, const counting_iterator &b) { return (a.val_ < b.val_); }
  friend bool operator>(const counting_iterator &a, const counting_iterator &b) { return (a.val_ > b.val_); }
  friend bool operator<=(const counting_iterator &a, const counting_iterator &b) { return (a.val_ <= b.val_); }
  friend bool operator>=(const counting_iterator &a, const counting_iterator &b) { return (a.val_ >= b.val_); }

 private:
  T val_{};
};
}  // namespace util

#endif  // UTIL_COUNTING_ITERATOR_HPP
