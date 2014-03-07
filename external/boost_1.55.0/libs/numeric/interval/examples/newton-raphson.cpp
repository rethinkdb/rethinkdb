/* Boost example/newton-raphson.cpp
 * Newton iteration for intervals
 *
 * Copyright 2003 Guillaume Melquiond
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or
 * copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <boost/numeric/interval.hpp>
#include <vector>
#include <algorithm>
#include <utility>
#include <iostream>
#include <iomanip>

template <class I> I f(const I& x)
{ return x * (x - 1.) * (x - 2.) * (x - 3.) * (x - 4.); }
template <class I> I f_diff(const I& x)
{ return (((5. * x - 40.) * x + 105.) * x - 100.) * x + 24.; }

static const double max_width = 1e-10;
static const double alpha = 0.75;

using namespace boost;
using namespace numeric;
using namespace interval_lib;

// First method: no empty intervals

typedef interval<double> I1_aux;
typedef unprotect<I1_aux>::type I1;

std::vector<I1> newton_raphson(const I1& xs) {
  std::vector<I1> l, res;
  I1 vf, vd, x, x1, x2;
  l.push_back(xs);
  while (!l.empty()) {
    x = l.back();
    l.pop_back();
    bool x2_used;
    double xx = median(x);
    vf = f<I1>(xx);
    vd = f_diff<I1>(x);
    if (zero_in(vf) && zero_in(vd)) {
      x1 = I1::whole();
      x2_used = false;
    } else {
      x1 = xx - division_part1(vf, vd, x2_used);
      if (x2_used) x2 = xx - division_part2(vf, vd);
    }
    if (overlap(x1, x)) x1 = intersect(x, x1);
    else if (x2_used) { x1 = x2; x2_used = false; }
    else continue;
    if (x2_used) {
      if (overlap(x2, x)) x2 = intersect(x, x2);
      else x2_used = false;
    }
    if (x2_used && width(x2) > width(x1)) std::swap(x1, x2);
    if (!zero_in(f(x1))) {
      if (x2_used) { x1 = x2; x2_used = false; }
      else continue;
    }
    if (width(x1) < max_width) res.push_back(x1);
    else if (width(x1) > alpha * width(x)) {
      std::pair<I1, I1> p = bisect(x);
      if (zero_in(f(p.first))) l.push_back(p.first);
      x2 = p.second;
      x2_used = true;
    } else l.push_back(x1);
    if (x2_used && zero_in(f(x2))) {
      if (width(x2) < max_width) res.push_back(x2);
      else l.push_back(x2);
    }
  }
  return res;
}

// Second method: with empty intervals

typedef change_checking<I1_aux, checking_no_nan<double> >::type I2_aux;
typedef unprotect<I2_aux>::type I2;

std::vector<I2> newton_raphson(const I2& xs) {
  std::vector<I2> l, res;
  I2 vf, vd, x, x1, x2;
  l.push_back(xs);
  while (!l.empty()) {
    x = l.back();
    l.pop_back();
    double xx = median(x);
    vf = f<I2>(xx);
    vd = f_diff<I2>(x);
    if (zero_in(vf) && zero_in(vd)) {
      x1 = x;
      x2 = I2::empty();
    } else {
      bool x2_used;
      x1 = intersect(x, xx - division_part1(vf, vd, x2_used));
      x2 = intersect(x, xx - division_part2(vf, vd, x2_used));
    }
    if (width(x2) > width(x1)) std::swap(x1, x2);
    if (empty(x1) || !zero_in(f(x1))) {
      if (!empty(x2)) { x1 = x2; x2 = I2::empty(); }
      else continue;
    }
    if (width(x1) < max_width) res.push_back(x1);
    else if (width(x1) > alpha * width(x)) {
      std::pair<I2, I2> p = bisect(x);
      if (zero_in(f(p.first))) l.push_back(p.first);
      x2 = p.second;
    } else l.push_back(x1);
    if (!empty(x2) && zero_in(f(x2))) {
      if (width(x2) < max_width) res.push_back(x2);
      else l.push_back(x2);
    }
  }
  return res;
}

template<class T, class Policies>
std::ostream &operator<<(std::ostream &os,
                         const boost::numeric::interval<T, Policies> &x) {
  os << "[" << x.lower() << ", " << x.upper() << "]";
  return os;
}

int main() {
  {
    I1_aux::traits_type::rounding rnd;
    std::vector<I1> res = newton_raphson(I1(-1, 5.1));
    std::cout << "Results: " << std::endl << std::setprecision(12);
    for(std::vector<I1>::const_iterator i = res.begin(); i != res.end(); ++i)
      std::cout << "  " << *i << std::endl;
    std::cout << std::endl;
  }
  {
    I2_aux::traits_type::rounding rnd;
    std::vector<I2> res = newton_raphson(I2(-1, 5.1));
    std::cout << "Results: " << std::endl << std::setprecision(12);
    for(std::vector<I2>::const_iterator i = res.begin(); i != res.end(); ++i)
      std::cout << "  " << *i << std::endl;
    std::cout << std::endl;
  }
}
