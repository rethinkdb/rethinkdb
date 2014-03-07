//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//  Adaptation to Boost of the libcxx
//  Copyright 2010 Vicente J. Botet Escriba
//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

#ifndef REP_H
#define REP_H

#include <boost/config.hpp>

class Rep
{
public:
    int data_;
    BOOST_CONSTEXPR Rep() : data_() {}
    explicit BOOST_CONSTEXPR Rep(int i) : data_(i) {}

    BOOST_CONSTEXPR bool operator==(int i) const {return data_ == i;}
    BOOST_CONSTEXPR bool operator==(const Rep& r) const {return data_ == r.data_;}

    Rep& operator*=(Rep x) {data_ *= x.data_; return *this;}
    Rep& operator/=(Rep x) {data_ /= x.data_; return *this;}
};

#if 0
namespace std {

  template <>
  struct numeric_limits<Rep>
  {
    static BOOST_CONSTEXPR Rep max BOOST_PREVENT_MACRO_SUBSTITUTION ()
    {
      return Rep((std::numeric_limits<int>::max)());
    }

  };
}  // namespace std

namespace boost {
namespace chrono {
template <>
struct duration_values<Rep>
{
  static BOOST_CONSTEXPR Rep zero() {return Rep(0);}
  static BOOST_CONSTEXPR Rep max BOOST_PREVENT_MACRO_SUBSTITUTION ()
  {
    return Rep((std::numeric_limits<int>::max)());
  }

  static BOOST_CONSTEXPR Rep min BOOST_PREVENT_MACRO_SUBSTITUTION ()
  {
    return Rep(detail::numeric_limits<Rep>::lowest());
  }
};

}  // namespace chrono
}  // namespace boost
#endif
#endif  // REP_H
