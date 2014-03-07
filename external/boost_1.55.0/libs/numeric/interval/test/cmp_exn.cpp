/* Boost test/cmp_exn.cpp
 * test policies with respect to exception throwing
 *
 * Copyright 2004 Guillaume Melquiond
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or
 * copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <boost/numeric/interval/interval.hpp>
#include <boost/numeric/interval/checking.hpp>
#include <boost/numeric/interval/compare.hpp>
#include <boost/numeric/interval/policies.hpp>
#include <boost/numeric/interval/compare/tribool.hpp>
#include <boost/test/test_tools.hpp>

struct my_checking
{
  static int nan()          { return -1; }
  static bool is_nan(int x) { return x < 0; }
  static int empty_lower()  { return -1; }
  static int empty_upper()  { return -1; }
  static bool is_empty(int l, int u) { return l == -1 && u == -1; }
};

struct empty_class {};

typedef boost::numeric::interval_lib::policies< empty_class, my_checking >
  my_policies;

typedef boost::numeric::interval<int, my_policies> I;

#define BOOST_C_EXN(e) \
  BOOST_CHECK_THROW(e, boost::numeric::interval_lib::comparison_error)

static void test_cer()
{
  I const a(I::empty()), b(1,2);
  int const c = 0, d = my_checking::nan();
  using namespace boost::numeric::interval_lib::compare::certain;

  BOOST_C_EXN(a < b);
  BOOST_C_EXN(a <= b);
  BOOST_C_EXN(a > b);
  BOOST_C_EXN(a >= b);
  BOOST_C_EXN(a == b);
  BOOST_C_EXN(a != b);
  BOOST_C_EXN(b < a);
  BOOST_C_EXN(b <= a);
  BOOST_C_EXN(b > a);
  BOOST_C_EXN(b >= a);
  BOOST_C_EXN(b == a);
  BOOST_C_EXN(b != a);

  BOOST_C_EXN(a < c);
  BOOST_C_EXN(a <= c);
  BOOST_C_EXN(a > c);
  BOOST_C_EXN(a >= c);
  BOOST_C_EXN(a == c);
  BOOST_C_EXN(a != c);
  BOOST_C_EXN(b < d);
  BOOST_C_EXN(b <= d);
  BOOST_C_EXN(b > d);
  BOOST_C_EXN(b >= d);
  BOOST_C_EXN(b == d);
  BOOST_C_EXN(b != d);
}

static void test_def()
{
  I const a(I::empty()), b(1,2);
  int const c = 0, d = my_checking::nan();

  BOOST_C_EXN(a < b);
  BOOST_C_EXN(a <= b);
  BOOST_C_EXN(a > b);
  BOOST_C_EXN(a >= b);
  BOOST_C_EXN(a == b);
  BOOST_C_EXN(a != b);
  BOOST_C_EXN(b < a);
  BOOST_C_EXN(b <= a);
  BOOST_C_EXN(b > a);
  BOOST_C_EXN(b >= a);
  BOOST_C_EXN(b == a);
  BOOST_C_EXN(b != a);

  BOOST_C_EXN(a < c);
  BOOST_C_EXN(a <= c);
  BOOST_C_EXN(a > c);
  BOOST_C_EXN(a >= c);
  BOOST_C_EXN(a == c);
  BOOST_C_EXN(a != c);
  BOOST_C_EXN(b < d);
  BOOST_C_EXN(b <= d);
  BOOST_C_EXN(b > d);
  BOOST_C_EXN(b >= d);
  BOOST_C_EXN(b == d);
  BOOST_C_EXN(b != d);
}

static void test_lex()
{
  I const a(I::empty()), b(1,2);
  int const c = 0, d = my_checking::nan();
  using namespace boost::numeric::interval_lib::compare::lexicographic;

  BOOST_C_EXN(a < b);
  BOOST_C_EXN(a <= b);
  BOOST_C_EXN(a > b);
  BOOST_C_EXN(a >= b);
  BOOST_C_EXN(a == b);
  BOOST_C_EXN(a != b);
  BOOST_C_EXN(b < a);
  BOOST_C_EXN(b <= a);
  BOOST_C_EXN(b > a);
  BOOST_C_EXN(b >= a);
  BOOST_C_EXN(b == a);
  BOOST_C_EXN(b != a);

  BOOST_C_EXN(a < c);
  BOOST_C_EXN(a <= c);
  BOOST_C_EXN(a > c);
  BOOST_C_EXN(a >= c);
  BOOST_C_EXN(a == c);
  BOOST_C_EXN(a != c);
  BOOST_C_EXN(b < d);
  BOOST_C_EXN(b <= d);
  BOOST_C_EXN(b > d);
  BOOST_C_EXN(b >= d);
  BOOST_C_EXN(b == d);
  BOOST_C_EXN(b != d);
}

static void test_pos()
{
  I const a(I::empty()), b(1,2);
  int const c = 0, d = my_checking::nan();
  using namespace boost::numeric::interval_lib::compare::possible;

  BOOST_C_EXN(a < b);
  BOOST_C_EXN(a <= b);
  BOOST_C_EXN(a > b);
  BOOST_C_EXN(a >= b);
  BOOST_C_EXN(a == b);
  BOOST_C_EXN(a != b);
  BOOST_C_EXN(b < a);
  BOOST_C_EXN(b <= a);
  BOOST_C_EXN(b > a);
  BOOST_C_EXN(b >= a);
  BOOST_C_EXN(b == a);
  BOOST_C_EXN(b != a);

  BOOST_C_EXN(a < c);
  BOOST_C_EXN(a <= c);
  BOOST_C_EXN(a > c);
  BOOST_C_EXN(a >= c);
  BOOST_C_EXN(a == c);
  BOOST_C_EXN(a != c);
  BOOST_C_EXN(b < d);
  BOOST_C_EXN(b <= d);
  BOOST_C_EXN(b > d);
  BOOST_C_EXN(b >= d);
  BOOST_C_EXN(b == d);
  BOOST_C_EXN(b != d);
}

static void test_set()
{
  I const a(I::empty()), b(1,2);
  int const c = 0;
  using namespace boost::numeric::interval_lib::compare::set;

  BOOST_C_EXN(a < c);
  BOOST_C_EXN(a <= c);
  BOOST_C_EXN(a > c);
  BOOST_C_EXN(a >= c);
  BOOST_C_EXN(a == c);
  BOOST_C_EXN(a != c);
  BOOST_C_EXN(b < c);
  BOOST_C_EXN(b <= c);
  BOOST_C_EXN(b > c);
  BOOST_C_EXN(b >= c);
  BOOST_C_EXN(b == c);
  BOOST_C_EXN(b != c);
}

static void test_tri()
{
  I const a(I::empty()), b(1,2);
  int const c = 0, d = my_checking::nan();
  using namespace boost::numeric::interval_lib::compare::tribool;

  BOOST_C_EXN(a < b);
  BOOST_C_EXN(a <= b);
  BOOST_C_EXN(a > b);
  BOOST_C_EXN(a >= b);
  BOOST_C_EXN(a == b);
  BOOST_C_EXN(a != b);
  BOOST_C_EXN(b < a);
  BOOST_C_EXN(b <= a);
  BOOST_C_EXN(b > a);
  BOOST_C_EXN(b >= a);
  BOOST_C_EXN(b == a);
  BOOST_C_EXN(b != a);

  BOOST_C_EXN(a < c);
  BOOST_C_EXN(a <= c);
  BOOST_C_EXN(a > c);
  BOOST_C_EXN(a >= c);
  BOOST_C_EXN(a == c);
  BOOST_C_EXN(a != c);
  BOOST_C_EXN(b < d);
  BOOST_C_EXN(b <= d);
  BOOST_C_EXN(b > d);
  BOOST_C_EXN(b >= d);
  BOOST_C_EXN(b == d);
  BOOST_C_EXN(b != d);
}

int test_main(int, char *[]) {
  test_cer();
  test_def();
  test_lex();
  test_pos();
  test_set();
  test_tri();

  return 0;
}
