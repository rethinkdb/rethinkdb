// Boost.Polygon library voronoi_robust_fpt_test.cpp file

//          Copyright Andrii Sydorchuk 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org for updates, documentation, and revision history.

#include <cmath>
#include <ctime>
#include <vector>

#define BOOST_TEST_MODULE voronoi_robust_fpt_test
#include <boost/mpl/list.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/test/test_case_template.hpp>

#include <boost/polygon/detail/voronoi_ctypes.hpp>
#include <boost/polygon/detail/voronoi_robust_fpt.hpp>
using boost::polygon::detail::int32;
using boost::polygon::detail::uint32;
using boost::polygon::detail::int64;
using boost::polygon::detail::fpt64;
using boost::polygon::detail::efpt64;
using boost::polygon::detail::extended_int;
using boost::polygon::detail::extended_exponent_fpt;
using boost::polygon::detail::robust_fpt;
using boost::polygon::detail::robust_dif;
using boost::polygon::detail::robust_sqrt_expr;
using boost::polygon::detail::type_converter_fpt;
using boost::polygon::detail::type_converter_efpt;
using boost::polygon::detail::ulp_comparison;

typedef robust_fpt<double> rfpt_type;
typedef type_converter_fpt to_fpt_type;
typedef type_converter_efpt to_efpt_type;
type_converter_fpt to_fpt;

BOOST_AUTO_TEST_CASE(robust_fpt_constructors_test1) {
  rfpt_type a = rfpt_type();
  BOOST_CHECK_EQUAL(a.fpv(), 0.0);
  BOOST_CHECK_EQUAL(a.re(), 0.0);
  BOOST_CHECK_EQUAL(a.ulp(), 0);
}

BOOST_AUTO_TEST_CASE(robust_fpt_constructors_test2) {
  rfpt_type a(10.0, 1.0);
  BOOST_CHECK_EQUAL(a.fpv(), 10.0);
  BOOST_CHECK_EQUAL(a.re(), 1.0);
  BOOST_CHECK_EQUAL(a.ulp(), 1.0);
}

BOOST_AUTO_TEST_CASE(robust_fpt_constructors_test3) {
  rfpt_type a(10.0);
  BOOST_CHECK_EQUAL(a.fpv(), 10.0);
  BOOST_CHECK_EQUAL(a.re(), 0.0);
  BOOST_CHECK_EQUAL(a.ulp(), 0.0);
}

BOOST_AUTO_TEST_CASE(robust_fpt_constructors_test4) {
  rfpt_type a(10.0, 3.0);
  BOOST_CHECK_EQUAL(a.fpv(), 10.0);
  BOOST_CHECK_EQUAL(a.re(), 3.0);
  BOOST_CHECK_EQUAL(a.ulp(), 3.0);

  rfpt_type b(10.0, 2.75);
  BOOST_CHECK_EQUAL(b.fpv(), 10.0);
  BOOST_CHECK_EQUAL(b.re(), 2.75);
  BOOST_CHECK_EQUAL(b.ulp(), 2.75);
}

BOOST_AUTO_TEST_CASE(robust_fpt_sum_test1) {
  rfpt_type a(2.0, 5.0);
  rfpt_type b(3.0, 4.0);
  rfpt_type c = a + b;
  BOOST_CHECK_EQUAL(c.fpv(), 5.0);
  BOOST_CHECK_EQUAL(c.re(), 6.0);
  BOOST_CHECK_EQUAL(c.ulp(), 6.0);

  c += b;
  BOOST_CHECK_EQUAL(c.fpv(), 8.0);
  BOOST_CHECK_EQUAL(c.re(), 7.0);
  BOOST_CHECK_EQUAL(c.ulp(), 7.0);
}

BOOST_AUTO_TEST_CASE(robust_fpt_sum_test2) {
  rfpt_type a(3.0, 2.0);
  rfpt_type b(-2.0, 3.0);
  rfpt_type c = a + b;
  BOOST_CHECK_EQUAL(c.fpv(), 1.0);
  BOOST_CHECK_EQUAL(c.re(), 13.0);
  BOOST_CHECK_EQUAL(c.ulp(), 13.0);

  c += b;
  BOOST_CHECK_EQUAL(c.fpv(), -1.0);
  BOOST_CHECK_EQUAL(c.re(), 20.0);
  BOOST_CHECK_EQUAL(c.ulp(), 20.0);
}

BOOST_AUTO_TEST_CASE(robust_fpt_dif_test1) {
  rfpt_type a(2.0, 5.0);
  rfpt_type b(-3.0, 4.0);
  rfpt_type c = a - b;
  BOOST_CHECK_EQUAL(c.fpv(), 5.0);
  BOOST_CHECK_EQUAL(c.re(), 6.0);
  BOOST_CHECK_EQUAL(c.ulp(), 6.0);

  c -= b;
  BOOST_CHECK_EQUAL(c.fpv(), 8.0);
  BOOST_CHECK_EQUAL(c.re(), 7.0);
  BOOST_CHECK_EQUAL(c.ulp(), 7.0);
}

BOOST_AUTO_TEST_CASE(robust_fpt_dif_test2) {
  rfpt_type a(3.0, 2.0);
  rfpt_type b(2.0, 3.0);
  rfpt_type c = a - b;
  BOOST_CHECK_EQUAL(c.fpv(), 1.0);
  BOOST_CHECK_EQUAL(c.re(), 13.0);
  BOOST_CHECK_EQUAL(c.ulp(), 13.0);

  c -= b;
  BOOST_CHECK_EQUAL(c.fpv(), -1.0);
  BOOST_CHECK_EQUAL(c.re(), 20.0);
  BOOST_CHECK_EQUAL(c.ulp(), 20.0);
}

BOOST_AUTO_TEST_CASE(robust_fpt_mult_test3) {
  rfpt_type a(2.0, 3.0);
  rfpt_type b(4.0, 1.0);
  rfpt_type c = a * b;
  BOOST_CHECK_EQUAL(c.fpv(), 8.0);
  BOOST_CHECK_EQUAL(c.re(), 5.0);
  BOOST_CHECK_EQUAL(c.ulp(), 5.0);

  c *= b;
  BOOST_CHECK_EQUAL(c.fpv(), 32.0);
  BOOST_CHECK_EQUAL(c.re(), 7.0);
  BOOST_CHECK_EQUAL(c.ulp(), 7.0);
}

BOOST_AUTO_TEST_CASE(robust_fpt_div_test1) {
  rfpt_type a(2.0, 3.0);
  rfpt_type b(4.0, 1.0);
  rfpt_type c = a / b;
  BOOST_CHECK_EQUAL(c.fpv(), 0.5);
  BOOST_CHECK_EQUAL(c.re(), 5.0);
  BOOST_CHECK_EQUAL(c.ulp(), 5.0);

  c /= b;
  BOOST_CHECK_EQUAL(c.fpv(), 0.125);
  BOOST_CHECK_EQUAL(c.re(), 7.0);
  BOOST_CHECK_EQUAL(c.ulp(), 7.0);
}

BOOST_AUTO_TEST_CASE(robust_dif_constructors_test) {
  robust_dif<int> rd1;
  BOOST_CHECK_EQUAL(rd1.pos(), 0);
  BOOST_CHECK_EQUAL(rd1.neg(), 0);
  BOOST_CHECK_EQUAL(rd1.dif(), 0);

  robust_dif<int> rd2(1);
  BOOST_CHECK_EQUAL(rd2.pos(), 1);
  BOOST_CHECK_EQUAL(rd2.neg(), 0);
  BOOST_CHECK_EQUAL(rd2.dif(), 1);

  robust_dif<int> rd3(-1);
  BOOST_CHECK_EQUAL(rd3.pos(), 0);
  BOOST_CHECK_EQUAL(rd3.neg(), 1);
  BOOST_CHECK_EQUAL(rd3.dif(), -1);

  robust_dif<int> rd4(1, 2);
  BOOST_CHECK_EQUAL(rd4.pos(), 1);
  BOOST_CHECK_EQUAL(rd4.neg(), 2);
  BOOST_CHECK_EQUAL(rd4.dif(), -1);
}

BOOST_AUTO_TEST_CASE(robust_dif_operators_test1) {
  robust_dif<int> a(5, 2), b(1, 10);
  int dif_a = a.dif();
  int dif_b = b.dif();
  robust_dif<int> sum = a + b;
  robust_dif<int> dif = a - b;
  robust_dif<int> mult = a * b;
  robust_dif<int> umin = -a;
  BOOST_CHECK_EQUAL(sum.dif(), dif_a + dif_b);
  BOOST_CHECK_EQUAL(dif.dif(), dif_a - dif_b);
  BOOST_CHECK_EQUAL(mult.dif(), dif_a * dif_b);
  BOOST_CHECK_EQUAL(umin.dif(), -dif_a);
}

BOOST_AUTO_TEST_CASE(robust_dif_operators_test2) {
  robust_dif<int> a(5, 2);
  for (int b = -3; b <= 3; b += 6) {
    int dif_a = a.dif();
    int dif_b = b;
    robust_dif<int> sum = a + b;
    robust_dif<int> dif = a - b;
    robust_dif<int> mult = a * b;
    robust_dif<int> div = a / b;
    BOOST_CHECK_EQUAL(sum.dif(), dif_a + dif_b);
    BOOST_CHECK_EQUAL(dif.dif(), dif_a - dif_b);
    BOOST_CHECK_EQUAL(mult.dif(), dif_a * dif_b);
    BOOST_CHECK_EQUAL(div.dif(), dif_a / dif_b);
  }
}

BOOST_AUTO_TEST_CASE(robust_dif_operators_test3) {
  robust_dif<int> b(5, 2);
  for (int a = -3; a <= 3; a += 6) {
    int dif_a = a;
    int dif_b = b.dif();
    robust_dif<int> sum = a + b;
    robust_dif<int> dif = a - b;
    robust_dif<int> mult = a * b;
    BOOST_CHECK_EQUAL(sum.dif(), dif_a + dif_b);
    BOOST_CHECK_EQUAL(dif.dif(), dif_a - dif_b);
    BOOST_CHECK_EQUAL(mult.dif(), dif_a * dif_b);
  }
}

BOOST_AUTO_TEST_CASE(robust_dif_operators_test4) {
  std::vector< robust_dif<int> > a4(4, robust_dif<int>(5, 2));
  std::vector< robust_dif<int> > b4(4, robust_dif<int>(1, 2));
  std::vector< robust_dif<int> > c4 = a4;
  c4[0] += b4[0];
  c4[1] -= b4[1];
  c4[2] *= b4[2];
  BOOST_CHECK_EQUAL(c4[0].dif(), a4[0].dif() + b4[0].dif());
  BOOST_CHECK_EQUAL(c4[1].dif(), a4[1].dif() - b4[1].dif());
  BOOST_CHECK_EQUAL(c4[2].dif(), a4[2].dif() * b4[2].dif());
  a4[0] += b4[0].dif();
  a4[1] -= b4[1].dif();
  a4[2] *= b4[2].dif();
  a4[3] /= b4[3].dif();
  BOOST_CHECK_EQUAL(c4[0].dif(), a4[0].dif());
  BOOST_CHECK_EQUAL(c4[1].dif(), a4[1].dif());
  BOOST_CHECK_EQUAL(c4[2].dif(), a4[2].dif());
  BOOST_CHECK_EQUAL(c4[3].dif() / b4[3].dif(), a4[3].dif());
}

BOOST_AUTO_TEST_CASE(robust_sqrt_expr_test1) {
  robust_sqrt_expr<int32, fpt64, to_fpt_type> sqrt_expr;
  int32 A[1] = {10};
  int32 B[1] = {100};
  BOOST_CHECK_EQUAL(sqrt_expr.eval1(A, B), 100.0);
}

BOOST_AUTO_TEST_CASE(robust_sqrt_expr_test2) {
  robust_sqrt_expr<int32, fpt64, to_fpt_type> sqrt_expr;
  int32 A[2] = {10, 30};
  int32 B[2] = {400, 100};
  BOOST_CHECK_EQUAL(sqrt_expr.eval2(A, B), 500.0);
}

BOOST_AUTO_TEST_CASE(robust_sqrt_expr_test3) {
  robust_sqrt_expr<int32, fpt64, to_fpt_type> sqrt_expr;
  int32 A[2] = {10, -30};
  int32 B[2] = {400, 100};
  BOOST_CHECK_EQUAL(sqrt_expr.eval2(A, B), -100.0);
}

BOOST_AUTO_TEST_CASE(robust_sqrt_expr_test4) {
  robust_sqrt_expr<int32, fpt64, to_fpt_type> sqrt_expr;
  int32 A[3] = {10, 30, 20};
  int32 B[3] = {4, 1, 9};
  BOOST_CHECK_EQUAL(sqrt_expr.eval3(A, B), 110.0);
}

BOOST_AUTO_TEST_CASE(robust_sqrt_expr_test5) {
  robust_sqrt_expr<int32, fpt64, to_fpt_type> sqrt_expr;
  int32 A[3] = {10, 30, -20};
  int32 B[3] = {4, 1, 9};
  BOOST_CHECK_EQUAL(sqrt_expr.eval3(A, B), -10.0);
}

BOOST_AUTO_TEST_CASE(robust_sqrt_expr_test6) {
  robust_sqrt_expr<int32, fpt64, to_fpt_type> sqrt_expr;
  int32 A[4] = {10, 30, 20, 5};
  int32 B[4] = {4, 1, 9, 16};
  BOOST_CHECK_EQUAL(sqrt_expr.eval4(A, B), 130.0);
}

BOOST_AUTO_TEST_CASE(robust_sqrt_expr_test7) {
  robust_sqrt_expr<int32, fpt64, to_fpt_type> sqrt_expr;
  int32 A[4] = {10, 30, -20, -5};
  int32 B[4] = {4, 1, 9, 16};
  BOOST_CHECK_EQUAL(sqrt_expr.eval4(A, B), -30.0);
}

BOOST_AUTO_TEST_CASE(robust_sqrt_expr_test8) {
  typedef extended_int<16> eint512;
  robust_sqrt_expr<eint512, efpt64, to_efpt_type> sqrt_expr;
  int32 A[4] = {1000, 3000, -2000, -500};
  int32 B[4] = {400, 100, 900, 1600};
  eint512 AA[4], BB[4];
  for (std::size_t i = 0; i < 4; ++i) {
    AA[i] = A[i];
    BB[i] = B[i];
  }
  BOOST_CHECK_EQUAL(to_fpt(sqrt_expr.eval4(AA, BB)), -30000.0);
}

template <typename _int, typename _fpt>
class sqrt_expr_tester {
 public:
  static const std::size_t MX_SQRTS = 4;

  bool run() {
    static boost::mt19937 gen(static_cast<uint32>(time(NULL)));
    bool ret_val = true;
    for (std::size_t i = 0; i < MX_SQRTS; ++i) {
      a[i] = gen() & 1048575;
      int64 temp = gen() & 1048575;
      b[i] = temp * temp;
    }
    uint32 mask = (1 << MX_SQRTS);
    for (std::size_t i = 0; i < mask; i++) {
      fpt64 expected_val = 0.0;
      for (std::size_t j = 0; j < MX_SQRTS; j++) {
        if (i & (1 << j)) {
          A[j] = a[j];
          B[j] = b[j];
          expected_val += static_cast<fpt64>(a[j]) *
                          std::sqrt(static_cast<fpt64>(b[j]));
        } else {
          A[j] = -a[j];
          B[j] = b[j];
          expected_val -= static_cast<fpt64>(a[j]) *
                          std::sqrt(static_cast<fpt64>(b[j]));
        }
      }
      fpt64 received_val = to_fpt(sqrt_expr_.eval4(A, B));
      ret_val &= ulp_cmp(expected_val, received_val, 25) ==
                 ulp_comparison<fpt64>::EQUAL;
    }
    return ret_val;
  }

 private:
  robust_sqrt_expr<_int, _fpt, to_efpt_type> sqrt_expr_;
  ulp_comparison<fpt64> ulp_cmp;
  _int A[MX_SQRTS];
  _int B[MX_SQRTS];
  int64 a[MX_SQRTS];
  int64 b[MX_SQRTS];
};

BOOST_AUTO_TEST_CASE(mpz_sqrt_evaluator_test) {
  typedef extended_int<16> eint512;
  sqrt_expr_tester<eint512, efpt64> tester;
  for (int i = 0; i < 2000; ++i)
    BOOST_CHECK(tester.run());
}
