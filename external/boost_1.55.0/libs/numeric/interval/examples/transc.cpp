/* Boost example/transc.cpp
 * how to use an external library (GMP/MPFR in this case) in order to
 * get correct transcendental functions on intervals
 *
 * Copyright 2003 Guillaume Melquiond
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or
 * copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <boost/numeric/interval.hpp>
//extern "C" {
#include <gmp.h>
#include <mpfr.h>
//}
#include <iostream>

struct full_rounding:
  boost::numeric::interval_lib::rounded_arith_opp<double>
{
private:
  typedef int mpfr_func(mpfr_t, const __mpfr_struct*, mp_rnd_t);
  double invoke_mpfr(double x, mpfr_func f, mp_rnd_t r) {
    mpfr_t xx;
    mpfr_init_set_d(xx, x, GMP_RNDN);
    f(xx, xx, r);
    double res = mpfr_get_d(xx, r);
    mpfr_clear(xx);
    return res;
  }
public:
# define GENR_FUNC(name) \
  double name##_down(double x) { return invoke_mpfr(x, mpfr_##name, GMP_RNDD); } \
  double name##_up  (double x) { return invoke_mpfr(x, mpfr_##name, GMP_RNDU); }
  GENR_FUNC(exp)
  GENR_FUNC(log)
  GENR_FUNC(sin)
  GENR_FUNC(cos)
  GENR_FUNC(tan)
  GENR_FUNC(asin)
  GENR_FUNC(acos)
  GENR_FUNC(atan)
  GENR_FUNC(sinh)
  GENR_FUNC(cosh)
  GENR_FUNC(tanh)
  GENR_FUNC(asinh)
  GENR_FUNC(acosh)
  GENR_FUNC(atanh)
};

namespace dummy {
  using namespace boost;
  using namespace numeric;
  using namespace interval_lib;
  typedef save_state<full_rounding> R;
  typedef checking_strict<double> P;
  typedef interval<double, policies<R, P> > I;
};

typedef dummy::I I;

template<class os_t>
os_t& operator<<(os_t &os, const I &a) {
  os << '[' << a.lower() << ',' << a.upper() << ']';
  return os;
}

int main() {
  I x(0.5, 2.5);
  std::cout << "x = " << x << std::endl;
  std::cout.precision(16);
# define GENR_TEST(name) \
  std::cout << #name "(x) = " << name(x) << std::endl
  GENR_TEST(exp);
  GENR_TEST(log);
  GENR_TEST(sin);
  GENR_TEST(cos);
  GENR_TEST(tan);
  GENR_TEST(asin);
  GENR_TEST(acos);
  GENR_TEST(atan);
  GENR_TEST(sinh);
  GENR_TEST(cosh);
  GENR_TEST(tanh);
  GENR_TEST(asinh);
  GENR_TEST(acosh);
  GENR_TEST(atanh);
  return 0;
}
