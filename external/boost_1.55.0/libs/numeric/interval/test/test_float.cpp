/* Boost test/test_float.cpp
 * test arithmetic operations on a range of intervals
 *
 * Copyright 2003 Guillaume Melquiond
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or
 * copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <boost/numeric/interval.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/config.hpp>
#include "bugs.hpp"

/* All the following tests should be BOOST_CHECK; however, if a test fails,
   the probability is high that hundreds of other tests will fail, so it is
   replaced by BOOST_REQUIRE to avoid flooding the logs. */

template<class T, class F>
void test_unary() {
  typedef typename F::I I;
  for(I a(-10., -9.91); a.lower() <= 10.; a += 0.3) {
    if (!F::validate(a)) continue;
    I rI = F::f_I(a);
    T rT1 = F::f_T(a.lower()), rT2 = F::f_T(a.upper()),
      rT3 = F::f_T(median(a));
    BOOST_REQUIRE(in(rT1, rI));
    BOOST_REQUIRE(in(rT2, rI));
    BOOST_REQUIRE(in(rT3, rI));
  }
}

template<class T, class F>
void test_binary() {
  typedef typename F::I I;
  for(I a(-10., -9.91); a.lower() <= 10.; a += 0.3) {
    for(I b(-10., -9.91); b.lower() <= 10.; b += 0.3) {
      if (!F::validate(a, b)) continue;
      T al = a.lower(), au = a.upper(), bl = b.lower(), bu = b.upper();
      I rII = F::f_II(a, b);
      I rIT1 = F::f_IT(a, bl), rIT2 = F::f_IT(a, bu);
      I rTI1 = F::f_TI(al, b), rTI2 = F::f_TI(au, b);
      I rTT1 = F::f_TT(al, bl), rTT2 = F::f_TT(al, bu);
      I rTT3 = F::f_TT(au, bl), rTT4 = F::f_TT(au, bu);
      BOOST_REQUIRE(subset(rTT1, rIT1));
      BOOST_REQUIRE(subset(rTT3, rIT1));
      BOOST_REQUIRE(subset(rTT2, rIT2));
      BOOST_REQUIRE(subset(rTT4, rIT2));
      BOOST_REQUIRE(subset(rTT1, rTI1));
      BOOST_REQUIRE(subset(rTT2, rTI1));
      BOOST_REQUIRE(subset(rTT3, rTI2));
      BOOST_REQUIRE(subset(rTT4, rTI2));
      BOOST_REQUIRE(subset(rIT1, rII));
      BOOST_REQUIRE(subset(rIT2, rII));
      BOOST_REQUIRE(subset(rTI1, rII));
      BOOST_REQUIRE(subset(rTI2, rII));
    }
  }
}

#define new_unary_bunch(name, op, val) \
  template<class T> \
  struct name { \
    typedef boost::numeric::interval<T> I; \
    static I f_I(const I& a) { return op(a); } \
    static T f_T(const T& a) { return op(a); } \
    static bool validate(const I& a) { return val; } \
  }

//#ifndef BOOST_NO_STDC_NAMESPACE
using std::abs;
using std::sqrt;
//#endif

new_unary_bunch(bunch_pos, +, true);
new_unary_bunch(bunch_neg, -, true);
new_unary_bunch(bunch_sqrt, sqrt, a.lower() >= 0.);
new_unary_bunch(bunch_abs, abs, true);

template<class T>
void test_all_unaries() {
  BOOST_CHECKPOINT("pos");  test_unary<T, bunch_pos<T> >();
  BOOST_CHECKPOINT("neg");  test_unary<T, bunch_neg<T> >();
  BOOST_CHECKPOINT("sqrt"); test_unary<T, bunch_sqrt<T> >();
  BOOST_CHECKPOINT("abs");  test_unary<T, bunch_abs<T> >();
}

#define new_binary_bunch(name, op, val) \
  template<class T> \
  struct bunch_##name { \
    typedef boost::numeric::interval<T> I; \
    static I f_II(const I& a, const I& b) { return a op b; } \
    static I f_IT(const I& a, const T& b) { return a op b; } \
    static I f_TI(const T& a, const I& b) { return a op b; } \
    static I f_TT(const T& a, const T& b) \
    { return boost::numeric::interval_lib::name<I>(a,b); } \
    static bool validate(const I& a, const I& b) { return val; } \
  }

new_binary_bunch(add, +, true);
new_binary_bunch(sub, -, true);
new_binary_bunch(mul, *, true);
new_binary_bunch(div, /, !zero_in(b));

template<class T>
void test_all_binaries() {
  BOOST_CHECKPOINT("add"); test_binary<T, bunch_add<T> >();
  BOOST_CHECKPOINT("sub"); test_binary<T, bunch_sub<T> >();
  BOOST_CHECKPOINT("mul"); test_binary<T, bunch_mul<T> >();
  BOOST_CHECKPOINT("div"); test_binary<T, bunch_div<T> >();
}

int test_main(int, char *[]) {
  BOOST_CHECKPOINT("float tests");
  test_all_unaries<float> ();
  test_all_binaries<float> ();
  BOOST_CHECKPOINT("double tests");
  test_all_unaries<double>();
  test_all_binaries<double>();
  //BOOST_CHECKPOINT("long double tests");
  //test_all_unaries<long double>();
  //test_all_binaries<long double>();
# ifdef __BORLANDC__
  ::detail::ignore_warnings();
# endif
  return 0;
}
