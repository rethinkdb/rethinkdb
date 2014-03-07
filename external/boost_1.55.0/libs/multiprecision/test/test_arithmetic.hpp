///////////////////////////////////////////////////////////////
//  Copyright 2012 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_

#ifdef TEST_VLD
#include <vld.h>
#endif

#include <boost/math/special_functions/pow.hpp>
#include <boost/math/common_factor_rt.hpp>
#include "test.hpp"

template <class T>
struct is_boost_rational : public boost::mpl::false_{};

#ifdef BOOST_MSVC
// warning C4127: conditional expression is constant
#pragma warning(disable:4127)
#endif


bool isfloat(float){ return true; }
bool isfloat(double){ return true; }
bool isfloat(long double){ return true; }
template <class T> bool isfloat(T){ return false; }

namespace detail{

template<class tag, class Arg1, class Arg2, class Arg3, class Arg4>
typename boost::multiprecision::detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::result_type
   abs(boost::multiprecision::detail::expression<tag, Arg1, Arg2, Arg3, Arg4> const& v)
{
   typedef typename boost::multiprecision::detail::expression<tag, Arg1, Arg2, Arg3, Arg4>::result_type result_type;
   return v < 0 ? result_type(-v) : result_type(v);
}

}

template <class T>
struct is_twos_complement_integer : public boost::mpl::true_ {};

template <class T>
struct related_type
{
   typedef T type;
};

template <class Real, class Val>
void test_comparisons(Val, Val, const boost::mpl::false_)
{}

int normalize_compare_result(int r)
{
   return r > 0 ? 1 : r < 0 ? -1 : 0;
}

template <class Real, class Val>
void test_comparisons(Val a, Val b, const boost::mpl::true_)
{
   Real r1(a);
   Real r2(b);
   Real z(1);

   int cr = a < b ? -1 : a > b ? 1 : 0;

   BOOST_CHECK_EQUAL(r1 == r2, a == b);
   BOOST_CHECK_EQUAL(r1 != r2, a != b);
   BOOST_CHECK_EQUAL(r1 <= r2, a <= b);
   BOOST_CHECK_EQUAL(r1 < r2, a < b);
   BOOST_CHECK_EQUAL(r1 >= r2, a >= b);
   BOOST_CHECK_EQUAL(r1 > r2, a > b);

   BOOST_CHECK_EQUAL(r1 == b, a == b);
   BOOST_CHECK_EQUAL(r1 != b, a != b);
   BOOST_CHECK_EQUAL(r1 <= b, a <= b);
   BOOST_CHECK_EQUAL(r1 < b, a < b);
   BOOST_CHECK_EQUAL(r1 >= b, a >= b);
   BOOST_CHECK_EQUAL(r1 > b, a > b);

   BOOST_CHECK_EQUAL(a == r2, a == b);
   BOOST_CHECK_EQUAL(a != r2, a != b);
   BOOST_CHECK_EQUAL(a <= r2, a <= b);
   BOOST_CHECK_EQUAL(a < r2, a < b);
   BOOST_CHECK_EQUAL(a >= r2, a >= b);
   BOOST_CHECK_EQUAL(a > r2, a > b);

   BOOST_CHECK_EQUAL(r1*z == r2, a == b);
   BOOST_CHECK_EQUAL(r1*z != r2, a != b);
   BOOST_CHECK_EQUAL(r1*z <= r2, a <= b);
   BOOST_CHECK_EQUAL(r1*z < r2, a < b);
   BOOST_CHECK_EQUAL(r1*z >= r2, a >= b);
   BOOST_CHECK_EQUAL(r1*z > r2, a > b);

   BOOST_CHECK_EQUAL(r1 == r2*z, a == b);
   BOOST_CHECK_EQUAL(r1 != r2*z, a != b);
   BOOST_CHECK_EQUAL(r1 <= r2*z, a <= b);
   BOOST_CHECK_EQUAL(r1 < r2*z, a < b);
   BOOST_CHECK_EQUAL(r1 >= r2*z, a >= b);
   BOOST_CHECK_EQUAL(r1 > r2*z, a > b);

   BOOST_CHECK_EQUAL(r1*z == r2*z, a == b);
   BOOST_CHECK_EQUAL(r1*z != r2*z, a != b);
   BOOST_CHECK_EQUAL(r1*z <= r2*z, a <= b);
   BOOST_CHECK_EQUAL(r1*z < r2*z, a < b);
   BOOST_CHECK_EQUAL(r1*z >= r2*z, a >= b);
   BOOST_CHECK_EQUAL(r1*z > r2*z, a > b);

   BOOST_CHECK_EQUAL(r1*z == b, a == b);
   BOOST_CHECK_EQUAL(r1*z != b, a != b);
   BOOST_CHECK_EQUAL(r1*z <= b, a <= b);
   BOOST_CHECK_EQUAL(r1*z < b, a < b);
   BOOST_CHECK_EQUAL(r1*z >= b, a >= b);
   BOOST_CHECK_EQUAL(r1*z > b, a > b);

   BOOST_CHECK_EQUAL(a == r2*z, a == b);
   BOOST_CHECK_EQUAL(a != r2*z, a != b);
   BOOST_CHECK_EQUAL(a <= r2*z, a <= b);
   BOOST_CHECK_EQUAL(a < r2*z, a < b);
   BOOST_CHECK_EQUAL(a >= r2*z, a >= b);
   BOOST_CHECK_EQUAL(a > r2*z, a > b);

   BOOST_CHECK_EQUAL(normalize_compare_result(r1.compare(r2)), cr);
   BOOST_CHECK_EQUAL(normalize_compare_result(r2.compare(r1)), -cr);
   BOOST_CHECK_EQUAL(normalize_compare_result(r1.compare(b)), cr);
   BOOST_CHECK_EQUAL(normalize_compare_result(r2.compare(a)), -cr);
}

template <class Real, class Exp>
void test_conditional(Real v, Exp e)
{
   //
   // Verify that Exp is usable in Boolean contexts, and has the same value as v:
   //
   if(e)
   {
      BOOST_CHECK(v);
   }
   else
   {
      BOOST_CHECK(!v);
   }
   if(!e)
   {
      BOOST_CHECK(!v);
   }
   else
   {
      BOOST_CHECK(v);
   }
}

template <class Real>
void test_complement(Real a, Real b, Real c, const boost::mpl::true_&)
{
   int i = 1020304;
   int j = 56789123;
   int sign_mask = ~0;
   if(std::numeric_limits<Real>::is_signed)
   {
      BOOST_CHECK_EQUAL(~a ,  (~i & sign_mask));
      c = a & ~b;
      BOOST_CHECK_EQUAL(c ,  (i & (~j & sign_mask)));
      c = ~(a | b);
      BOOST_CHECK_EQUAL(c ,  (~(i | j) & sign_mask));
   }
   else
   {
      BOOST_CHECK_EQUAL((~a & a) ,  0);
   }
}

template <class Real>
void test_complement(Real, Real, Real, const boost::mpl::false_&)
{
}

template <class Real, class T>
void test_integer_ops(const T&){}

template <class Real>
void test_rational(const boost::mpl::true_&)
{
   Real a(2);
   a /= 3;
   BOOST_CHECK_EQUAL(numerator(a) ,  2);
   BOOST_CHECK_EQUAL(denominator(a) ,  3);
   Real b(4);
   b /= 6;
   BOOST_CHECK_EQUAL(a ,  b);

   //
   // Check IO code:
   //
   std::stringstream ss;
   ss << a;
   ss >> b;
   BOOST_CHECK_EQUAL(a, b);
}

template <class Real>
void test_rational(const boost::mpl::false_&)
{
   Real a(2);
   a /= 3;
   BOOST_CHECK_EQUAL(numerator(a) ,  2);
   BOOST_CHECK_EQUAL(denominator(a) ,  3);
   Real b(4);
   b /= 6;
   BOOST_CHECK_EQUAL(a ,  b);

   BOOST_CHECK_THROW(Real(a / 0), std::overflow_error);
   BOOST_CHECK_THROW(Real("3.14"), std::runtime_error);
   b = Real("2/3");
   BOOST_CHECK_EQUAL(a, b);
   //
   // Check IO code:
   //
   std::stringstream ss;
   ss << a;
   ss >> b;
   BOOST_CHECK_EQUAL(a, b);
}

template <class Real>
void test_integer_ops(const boost::mpl::int_<boost::multiprecision::number_kind_rational>&)
{
   test_rational<Real>(is_boost_rational<Real>());
}

template <class Real>
void test_signed_integer_ops(const boost::mpl::true_&)
{
   Real a(20);
   Real b(7);
   Real c(5);
   BOOST_CHECK_EQUAL(-a % c ,  0);
   BOOST_CHECK_EQUAL(-a % b ,  -20 % 7);
   BOOST_CHECK_EQUAL(-a % -b ,  -20 % -7);
   BOOST_CHECK_EQUAL(a % -b ,  20 % -7);
   BOOST_CHECK_EQUAL(-a % 7 ,  -20 % 7);
   BOOST_CHECK_EQUAL(-a % -7 ,  -20 % -7);
   BOOST_CHECK_EQUAL(a % -7 ,  20 % -7);
   BOOST_CHECK_EQUAL(-a % 7u ,  -20 % 7);
   BOOST_CHECK_EQUAL(-a % a ,  0);
   BOOST_CHECK_EQUAL(-a % 5 ,  0);
   BOOST_CHECK_EQUAL(-a % -5 ,  0);
   BOOST_CHECK_EQUAL(a % -5 ,  0);

   b = -b;
   BOOST_CHECK_EQUAL(a % b ,  20 % -7);
   a = -a;
   BOOST_CHECK_EQUAL(a % b ,  -20 % -7);
   BOOST_CHECK_EQUAL(a % -7 ,  -20 % -7);
   b = 7;
   BOOST_CHECK_EQUAL(a % b ,  -20 % 7);
   BOOST_CHECK_EQUAL(a % 7 ,  -20 % 7);
   BOOST_CHECK_EQUAL(a % 7u ,  -20 % 7);

   a = 20;
   a %= b;
   BOOST_CHECK_EQUAL(a ,  20 % 7);
   a = -20;
   a %= b;
   BOOST_CHECK_EQUAL(a ,  -20 % 7);
   a = 20;
   a %= -b;
   BOOST_CHECK_EQUAL(a ,  20 % -7);
   a = -20;
   a %= -b;
   BOOST_CHECK_EQUAL(a ,  -20 % -7);
   a = 5;
   a %= b - a;
   BOOST_CHECK_EQUAL(a ,  5 % (7-5));
   a = -20;
   a %= 7;
   BOOST_CHECK_EQUAL(a ,  -20 % 7);
   a = 20;
   a %= -7;
   BOOST_CHECK_EQUAL(a ,  20 % -7);
   a = -20;
   a %= -7;
   BOOST_CHECK_EQUAL(a ,  -20 % -7);
#ifndef BOOST_NO_LONG_LONG
   a = -20;
   a %= 7uLL;
   BOOST_CHECK_EQUAL(a ,  -20 % 7);
   a = 20;
   a %= -7LL;
   BOOST_CHECK_EQUAL(a ,  20 % -7);
   a = -20;
   a %= -7LL;
   BOOST_CHECK_EQUAL(a ,  -20 % -7);
#endif
   a = 400;
   b = 45;
   BOOST_CHECK_EQUAL(gcd(a, -45) ,  boost::math::gcd(400, 45));
   BOOST_CHECK_EQUAL(lcm(a, -45) ,  boost::math::lcm(400, 45));
   BOOST_CHECK_EQUAL(gcd(-400, b) ,  boost::math::gcd(400, 45));
   BOOST_CHECK_EQUAL(lcm(-400, b) ,  boost::math::lcm(400, 45));
   a = -20;
   BOOST_CHECK_EQUAL(abs(a) ,  20);
   BOOST_CHECK_EQUAL(abs(-a) ,  20);
   BOOST_CHECK_EQUAL(abs(+a) ,  20);
   a = 20;
   BOOST_CHECK_EQUAL(abs(a) ,  20);
   BOOST_CHECK_EQUAL(abs(-a) ,  20);
   BOOST_CHECK_EQUAL(abs(+a) ,  20);
   a = -400;
   b = 45;
   BOOST_CHECK_EQUAL(gcd(a, b) ,  boost::math::gcd(-400, 45));
   BOOST_CHECK_EQUAL(lcm(a, b) ,  boost::math::lcm(-400, 45));
   BOOST_CHECK_EQUAL(gcd(a, 45) ,  boost::math::gcd(-400, 45));
   BOOST_CHECK_EQUAL(lcm(a, 45) ,  boost::math::lcm(-400, 45));
   BOOST_CHECK_EQUAL(gcd(-400, b) ,  boost::math::gcd(-400, 45));
   BOOST_CHECK_EQUAL(lcm(-400, b) ,  boost::math::lcm(-400, 45));
   Real r;
   divide_qr(a, b, c, r);
   BOOST_CHECK_EQUAL(c ,  a / b);
   BOOST_CHECK_EQUAL(r ,  a % b);
   BOOST_CHECK_EQUAL(integer_modulus(a, 57) ,  abs(a % 57));
   b = -57;
   divide_qr(a, b, c, r);
   BOOST_CHECK_EQUAL(c ,  a / b);
   BOOST_CHECK_EQUAL(r ,  a % b);
   BOOST_CHECK_EQUAL(integer_modulus(a, -57) ,  abs(a % -57));
   a = 458;
   divide_qr(a, b, c, r);
   BOOST_CHECK_EQUAL(c ,  a / b);
   BOOST_CHECK_EQUAL(r ,  a % b);
   BOOST_CHECK_EQUAL(integer_modulus(a, -57) ,  abs(a % -57));
}
template <class Real>
void test_signed_integer_ops(const boost::mpl::false_&)
{
}

template <class Real>
void test_integer_ops(const boost::mpl::int_<boost::multiprecision::number_kind_integer>&)
{
   test_signed_integer_ops<Real>(boost::mpl::bool_<std::numeric_limits<Real>::is_signed>());

   Real a(20);
   Real b(7);
   Real c(5);
   BOOST_CHECK_EQUAL(a % b ,  20 % 7);
   BOOST_CHECK_EQUAL(a % 7 ,  20 % 7);
   BOOST_CHECK_EQUAL(a % 7u ,  20 % 7);
   BOOST_CHECK_EQUAL(a % a ,  0);
   BOOST_CHECK_EQUAL(a % c ,  0);
   BOOST_CHECK_EQUAL(a % 5 ,  0);
   a = a % (b + 0);
   BOOST_CHECK_EQUAL(a ,  20 % 7);
   a = 20;
   c = (a + 2) % (a - 1);
   BOOST_CHECK_EQUAL(c ,  22 % 19);
   c = 5;
   a = b % (a - 15);
   BOOST_CHECK_EQUAL(a ,  7 % 5);
   a = 20;

   a = 20;
   a %= 7;
   BOOST_CHECK_EQUAL(a ,  20 % 7);
#ifndef BOOST_NO_LONG_LONG
   a = 20;
   a %= 7uLL;
   BOOST_CHECK_EQUAL(a ,  20 % 7);
#endif
   a = 20;
   BOOST_CHECK_EQUAL(++a ,  21);
   BOOST_CHECK_EQUAL(--a ,  20);
   BOOST_CHECK_EQUAL(a++ ,  20);
   BOOST_CHECK_EQUAL(a ,  21);
   BOOST_CHECK_EQUAL(a-- ,  21);
   BOOST_CHECK_EQUAL(a ,  20);
   a = 2000;
   a <<= 20;
   BOOST_CHECK_EQUAL(a ,  2000L << 20);
   a >>= 20;
   BOOST_CHECK_EQUAL(a ,  2000);
   a <<= 20u;
   BOOST_CHECK_EQUAL(a ,  2000L << 20);
   a >>= 20u;
   BOOST_CHECK_EQUAL(a ,  2000);
   BOOST_CHECK_THROW(a <<= -20, std::out_of_range);
   BOOST_CHECK_THROW(a >>= -20, std::out_of_range);
   BOOST_CHECK_THROW(Real(a << -20), std::out_of_range);
   BOOST_CHECK_THROW(Real(a >> -20), std::out_of_range);
#ifndef BOOST_NO_LONG_LONG
   if(sizeof(long long) > sizeof(std::size_t))
   {
      // extreme values should trigger an exception:
      BOOST_CHECK_THROW(a >>= (1uLL << (sizeof(long long) * CHAR_BIT - 2)), std::out_of_range);
      BOOST_CHECK_THROW(a <<= (1uLL << (sizeof(long long) * CHAR_BIT - 2)), std::out_of_range);
      BOOST_CHECK_THROW(a >>= -(1LL << (sizeof(long long) * CHAR_BIT - 2)), std::out_of_range);
      BOOST_CHECK_THROW(a <<= -(1LL << (sizeof(long long) * CHAR_BIT - 2)), std::out_of_range);
      BOOST_CHECK_THROW(a >>= (1LL << (sizeof(long long) * CHAR_BIT - 2)), std::out_of_range);
      BOOST_CHECK_THROW(a <<= (1LL << (sizeof(long long) * CHAR_BIT - 2)), std::out_of_range);
      // Unless they fit within range:
      a = 2000L;
      BOOST_CHECK_EQUAL((a <<= 20uLL) ,  (2000L << 20));
      a = 2000;
      BOOST_CHECK_EQUAL((a <<= 20LL)  ,  (2000L << 20));

      BOOST_CHECK_THROW(Real(a >> (1uLL << (sizeof(long long) * CHAR_BIT - 2))), std::out_of_range);
      BOOST_CHECK_THROW(Real(a <<= (1uLL << (sizeof(long long) * CHAR_BIT - 2))), std::out_of_range);
      BOOST_CHECK_THROW(Real(a >>= -(1LL << (sizeof(long long) * CHAR_BIT - 2))), std::out_of_range);
      BOOST_CHECK_THROW(Real(a <<= -(1LL << (sizeof(long long) * CHAR_BIT - 2))), std::out_of_range);
      BOOST_CHECK_THROW(Real(a >>= (1LL << (sizeof(long long) * CHAR_BIT - 2))), std::out_of_range);
      BOOST_CHECK_THROW(Real(a <<= (1LL << (sizeof(long long) * CHAR_BIT - 2))), std::out_of_range);
      // Unless they fit within range:
      a = 2000L;
      BOOST_CHECK_EQUAL(Real(a << 20uLL) ,  (2000L << 20));
      a = 2000;
      BOOST_CHECK_EQUAL(Real(a << 20LL)  ,  (2000L << 20));
   }
#endif
   a = 20;
   b = a << 20;
   BOOST_CHECK_EQUAL(b ,  (20 << 20));
   b = a >> 2;
   BOOST_CHECK_EQUAL(b ,  (20 >> 2));
   b = (a + 2) << 10;
   BOOST_CHECK_EQUAL(b ,  (22 << 10));
   b = (a + 3) >> 3;
   BOOST_CHECK_EQUAL(b ,  (23 >> 3));
   //
   // Bit fiddling:
   //
   int i = 1020304;
   int j = 56789123;
   int k = 4523187;
   a = i;
   b = j;
   c = a;
   c &= b;
   BOOST_CHECK_EQUAL(c ,  (i & j));
   c = a;
   c &= j;
   BOOST_CHECK_EQUAL(c ,  (i & j));
   c = a;
   c &= a + b;
   BOOST_CHECK_EQUAL(c ,  (i & (i + j)));
   BOOST_CHECK_EQUAL((a & b) ,  (i & j));
   c = k;
   a = a & (b + k);
   BOOST_CHECK_EQUAL(a ,  (i & (j + k)));
   a = i;
   a = (b + k) & a;
   BOOST_CHECK_EQUAL(a ,  (i & (j + k)));
   a = i;
   c = a & b & k;
   BOOST_CHECK_EQUAL(c ,  (i&j&k));
   c = a;
   c &= (c+b);
   BOOST_CHECK_EQUAL(c ,  (i & (i+j)));
   c = a & (b | 1);
   BOOST_CHECK_EQUAL(c ,  (i & (j | 1)));

   test_complement<Real>(a, b, c, typename is_twos_complement_integer<Real>::type());

   a = i;
   b = j;
   c = a;
   c |= b;
   BOOST_CHECK_EQUAL(c ,  (i | j));
   c = a;
   c |= j;
   BOOST_CHECK_EQUAL(c ,  (i | j));
   c = a;
   c |= a + b;
   BOOST_CHECK_EQUAL(c ,  (i | (i + j)));
   BOOST_CHECK_EQUAL((a | b) ,  (i | j));
   c = k;
   a = a | (b + k);
   BOOST_CHECK_EQUAL(a ,  (i | (j + k)));
   a = i;
   a = (b + k) | a;
   BOOST_CHECK_EQUAL(a ,  (i | (j + k)));
   a = i;
   c = a | b | k;
   BOOST_CHECK_EQUAL(c ,  (i|j|k));
   c = a;
   c |= (c + b);
   BOOST_CHECK_EQUAL(c ,  (i | (i+j)));
   c = a | (b | 1);
   BOOST_CHECK_EQUAL(c ,  (i | (j | 1)));

   a = i;
   b = j;
   c = a;
   c ^= b;
   BOOST_CHECK_EQUAL(c ,  (i ^ j));
   c = a;
   c ^= j;
   BOOST_CHECK_EQUAL(c ,  (i ^ j));
   c = a;
   c ^= a + b;
   BOOST_CHECK_EQUAL(c ,  (i ^ (i + j)));
   BOOST_CHECK_EQUAL((a ^ b) ,  (i ^ j));
   c = k;
   a = a ^ (b + k);
   BOOST_CHECK_EQUAL(a ,  (i ^ (j + k)));
   a = i;
   a = (b + k) ^ a;
   BOOST_CHECK_EQUAL(a ,  (i ^ (j + k)));
   a = i;
   c = a ^ b ^ k;
   BOOST_CHECK_EQUAL(c ,  (i^j^k));
   c = a;
   c ^= (c + b);
   BOOST_CHECK_EQUAL(c ,  (i ^ (i+j)));
   c = a ^ (b | 1);
   BOOST_CHECK_EQUAL(c ,  (i ^ (j | 1)));

   a = i;
   b = j;
   c = k;
   //
   // Non-member functions:
   //
   a = 400;
   b = 45;
   BOOST_CHECK_EQUAL(gcd(a, b) ,  boost::math::gcd(400, 45));
   BOOST_CHECK_EQUAL(lcm(a, b) ,  boost::math::lcm(400, 45));
   BOOST_CHECK_EQUAL(gcd(a, 45) ,  boost::math::gcd(400, 45));
   BOOST_CHECK_EQUAL(lcm(a, 45) ,  boost::math::lcm(400, 45));
   BOOST_CHECK_EQUAL(gcd(a, 45u) ,  boost::math::gcd(400, 45));
   BOOST_CHECK_EQUAL(lcm(a, 45u) ,  boost::math::lcm(400, 45));
   BOOST_CHECK_EQUAL(gcd(400, b) ,  boost::math::gcd(400, 45));
   BOOST_CHECK_EQUAL(lcm(400, b) ,  boost::math::lcm(400, 45));
   BOOST_CHECK_EQUAL(gcd(400u, b) ,  boost::math::gcd(400, 45));
   BOOST_CHECK_EQUAL(lcm(400u, b) ,  boost::math::lcm(400, 45));

   //
   // Conditionals involving 2 arg functions:
   //
   test_conditional(Real(gcd(a, b)), gcd(a, b));

   Real r;
   divide_qr(a, b, c, r);
   BOOST_CHECK_EQUAL(c ,  a / b);
   BOOST_CHECK_EQUAL(r ,  a % b);
   divide_qr(a + 0, b, c, r);
   BOOST_CHECK_EQUAL(c ,  a / b);
   BOOST_CHECK_EQUAL(r ,  a % b);
   divide_qr(a, b+0, c, r);
   BOOST_CHECK_EQUAL(c ,  a / b);
   BOOST_CHECK_EQUAL(r ,  a % b);
   divide_qr(a+0, b+0, c, r);
   BOOST_CHECK_EQUAL(c ,  a / b);
   BOOST_CHECK_EQUAL(r ,  a % b);
   BOOST_CHECK_EQUAL(integer_modulus(a, 57) ,  a % 57);
   for(unsigned i = 0; i < 20; ++i)
   {
      if(std::numeric_limits<Real>::is_specialized && (!std::numeric_limits<Real>::is_bounded || ((int)i * 17 < std::numeric_limits<Real>::digits)))
      {
         BOOST_CHECK_EQUAL(lsb(Real(1) << (i * 17)) ,  i * 17);
         BOOST_CHECK_EQUAL(msb(Real(1) << (i * 17)) ,  i * 17);
         BOOST_CHECK(bit_test(Real(1) << (i * 17), i * 17));
         BOOST_CHECK(!bit_test(Real(1) << (i * 17), i * 17 + 1));
         if(i)
         {
            BOOST_CHECK(!bit_test(Real(1) << (i * 17), i * 17 - 1));
         }
         Real zero(0);
         BOOST_CHECK(bit_test(bit_set(zero, i * 17), i * 17));
         zero = 0;
         BOOST_CHECK_EQUAL(bit_flip(zero, i*17) ,  Real(1) << i * 17);
         zero = Real(1) << i * 17;
         BOOST_CHECK_EQUAL(bit_flip(zero, i * 17) ,  0);
         zero = Real(1) << i * 17;
         BOOST_CHECK_EQUAL(bit_unset(zero, i * 17) ,  0);
      }
   }
   //
   // pow, powm:
   //
   BOOST_CHECK_EQUAL(pow(Real(3), 4) ,  81);
   BOOST_CHECK_EQUAL(pow(Real(3) + Real(0), 4) ,  81);
   BOOST_CHECK_EQUAL(powm(Real(3), Real(4), Real(13)) ,  81 % 13);
   BOOST_CHECK_EQUAL(powm(Real(3), Real(4), 13) ,  81 % 13);
   BOOST_CHECK_EQUAL(powm(Real(3), Real(4), Real(13) + 0) ,  81 % 13);
   BOOST_CHECK_EQUAL(powm(Real(3), Real(4) + 0, Real(13)) ,  81 % 13);
   BOOST_CHECK_EQUAL(powm(Real(3), Real(4) + 0, 13) ,  81 % 13);
   BOOST_CHECK_EQUAL(powm(Real(3), Real(4) + 0, Real(13) + 0) ,  81 % 13);
   BOOST_CHECK_EQUAL(powm(Real(3), 4 + 0, Real(13)) ,  81 % 13);
   BOOST_CHECK_EQUAL(powm(Real(3), 4 + 0, 13) ,  81 % 13);
   BOOST_CHECK_EQUAL(powm(Real(3), 4 + 0, Real(13) + 0) ,  81 % 13);
   BOOST_CHECK_EQUAL(powm(Real(3) + 0, Real(4), Real(13)) ,  81 % 13);
   BOOST_CHECK_EQUAL(powm(Real(3) + 0, Real(4), 13) ,  81 % 13);
   BOOST_CHECK_EQUAL(powm(Real(3) + 0, Real(4), Real(13) + 0) ,  81 % 13);
   BOOST_CHECK_EQUAL(powm(Real(3) + 0, Real(4) + 0, Real(13)) ,  81 % 13);
   BOOST_CHECK_EQUAL(powm(Real(3) + 0, Real(4) + 0, 13) ,  81 % 13);
   BOOST_CHECK_EQUAL(powm(Real(3) + 0, Real(4) + 0, Real(13) + 0) ,  81 % 13);
   BOOST_CHECK_EQUAL(powm(Real(3) + 0, 4 + 0, Real(13)) ,  81 % 13);
   BOOST_CHECK_EQUAL(powm(Real(3) + 0, 4 + 0, 13) ,  81 % 13);
   BOOST_CHECK_EQUAL(powm(Real(3) + 0, 4 + 0, Real(13) + 0) ,  81 % 13);
   //
   // Conditionals involving 3 arg functions:
   //
   test_conditional(Real(powm(Real(3), Real(4), Real(13))), powm(Real(3), Real(4), Real(13)));

   //
   // Things that are expected errors:
   //
   BOOST_CHECK_THROW(Real("3.14"), std::runtime_error);
   BOOST_CHECK_THROW(Real("3L"), std::runtime_error);
   BOOST_CHECK_THROW(Real(Real(20) / 0u), std::overflow_error);
   //
   // Extra tests added for full coverage:
   //
   a = 20;
   b = 7;
   c = 20 % b;
   BOOST_CHECK_EQUAL(c ,  (20 % 7));
   c = 20 % (b + 0);
   BOOST_CHECK_EQUAL(c ,  (20 % 7));
   c = a & 10;
   BOOST_CHECK_EQUAL(c ,  (20 & 10));
   c = 10 & a;
   BOOST_CHECK_EQUAL(c ,  (20 & 10));
   c = (a + 0) & (b + 0);
   BOOST_CHECK_EQUAL(c ,  (20 & 7));
   c = 10 & (a + 0);
   BOOST_CHECK_EQUAL(c ,  (20 & 10));
   c = 10 | a;
   BOOST_CHECK_EQUAL(c ,  (20 | 10));
   c = (a + 0) | (b + 0);
   BOOST_CHECK(c == (20 | 7))
   c = 20 | (b + 0);
   BOOST_CHECK_EQUAL(c ,  (20 | 7));
   c = a ^ 7;
   BOOST_CHECK_EQUAL(c ,  (20 ^ 7));
   c = 20 ^ b;
   BOOST_CHECK_EQUAL(c ,  (20 ^ 7));
   c = (a + 0) ^ (b + 0);
   BOOST_CHECK_EQUAL(c ,  (20 ^ 7));
   c = 20 ^ (b + 0);
   BOOST_CHECK_EQUAL(c ,  (20 ^ 7));
}

template <class Real, class T>
void test_float_funcs(const T&){}

template <class Real>
void test_float_funcs(const boost::mpl::true_&)
{
   if(boost::multiprecision::is_interval_number<Real>::value)
      return;
   //
   // Test variable reuse in function calls, see https://svn.boost.org/trac/boost/ticket/8326
   //
   Real a(2), b(10);
   a = pow(a, b);
   BOOST_CHECK_EQUAL(a, 1024);
   a = 2;
   b = pow(a, b);
   BOOST_CHECK_EQUAL(b, 1024);
   b = 10;
   a = pow(a, 10);
   BOOST_CHECK_EQUAL(a, 1024);
   a = -2;
   a = abs(a);
   BOOST_CHECK_EQUAL(a, 2);
   a = -2;
   a = fabs(a);
   BOOST_CHECK_EQUAL(a, 2);
   a = 2.5;
   a = floor(a);
   BOOST_CHECK_EQUAL(a, 2);
   a = 2.5;
   a = ceil(a);
   BOOST_CHECK_EQUAL(a, 3);
   a = 2.5;
   a = trunc(a);
   BOOST_CHECK_EQUAL(a, 2);
   a = 2.25;
   a = round(a);
   BOOST_CHECK_EQUAL(a, 2);
   a = 2;
   a = ldexp(a, 1);
   BOOST_CHECK_EQUAL(a, 4);
   int i;
   a = frexp(a, &i);
   BOOST_CHECK_EQUAL(a, 0.5);

   Real tol = std::numeric_limits<Real>::epsilon() * 3;
   a = 4;
   a = sqrt(a);
   BOOST_CHECK_CLOSE_FRACTION(a, 2, tol);
   a = 3;
   a = exp(a);
   BOOST_CHECK_CLOSE_FRACTION(a, Real(exp(Real(3))), tol);
   a = 3;
   a = log(a);
   BOOST_CHECK_CLOSE_FRACTION(a, Real(log(Real(3))), tol);
   a = 3;
   a = log10(a);
   BOOST_CHECK_CLOSE_FRACTION(a, Real(log10(Real(3))), tol);

   a = 0.5;
   a = sin(a);
   BOOST_CHECK_CLOSE_FRACTION(a, Real(sin(Real(0.5))), tol);
   a = 0.5;
   a = cos(a);
   BOOST_CHECK_CLOSE_FRACTION(a, Real(cos(Real(0.5))), tol);
   a = 0.5;
   a = tan(a);
   BOOST_CHECK_CLOSE_FRACTION(a, Real(tan(Real(0.5))), tol);
   a = 0.5;
   a = asin(a);
   BOOST_CHECK_CLOSE_FRACTION(a, Real(asin(Real(0.5))), tol);
   a = 0.5;
   a = acos(a);
   BOOST_CHECK_CLOSE_FRACTION(a, Real(acos(Real(0.5))), tol);
   a = 0.5;
   a = atan(a);
   BOOST_CHECK_CLOSE_FRACTION(a, Real(atan(Real(0.5))), tol);
   a = 0.5;
   a = sinh(a);
   BOOST_CHECK_CLOSE_FRACTION(a, Real(sinh(Real(0.5))), tol);
   a = 0.5;
   a = cosh(a);
   BOOST_CHECK_CLOSE_FRACTION(a, Real(cosh(Real(0.5))), tol);
   a = 0.5;
   a = tanh(a);
   BOOST_CHECK_CLOSE_FRACTION(a, Real(tanh(Real(0.5))), tol);
   a = 4;
   b = 2;
   a = fmod(a, b);
   BOOST_CHECK_CLOSE_FRACTION(a, Real(fmod(Real(4), Real(2))), tol);
   a = 4;
   b = fmod(a, b);
   BOOST_CHECK_CLOSE_FRACTION(b, Real(fmod(Real(4), Real(2))), tol);
   b = 2;
   a = atan2(a, b);
   BOOST_CHECK_CLOSE_FRACTION(a, Real(atan2(Real(4), Real(2))), tol);
   a = 4;
   b = atan2(a, b);
   BOOST_CHECK_CLOSE_FRACTION(b, Real(atan2(Real(4), Real(2))), tol);
}

template <class Real, class T>
void test_float_ops(const T&){}

template <class Real>
void test_float_ops(const boost::mpl::int_<boost::multiprecision::number_kind_floating_point>&)
{
   BOOST_CHECK_EQUAL(abs(Real(2)) ,  2);
   BOOST_CHECK_EQUAL(abs(Real(-2)) ,  2);
   BOOST_CHECK_EQUAL(fabs(Real(2)) ,  2);
   BOOST_CHECK_EQUAL(fabs(Real(-2)) ,  2);
   BOOST_CHECK_EQUAL(floor(Real(5) / 2) ,  2);
   BOOST_CHECK_EQUAL(ceil(Real(5) / 2) ,  3);
   BOOST_CHECK_EQUAL(floor(Real(-5) / 2) ,  -3);
   BOOST_CHECK_EQUAL(ceil(Real(-5) / 2) ,  -2);
   BOOST_CHECK_EQUAL(trunc(Real(5) / 2) ,  2);
   BOOST_CHECK_EQUAL(trunc(Real(-5) / 2) ,  -2);
   //
   // ldexp and frexp, these pretty much have to be implemented by each backend:
   //
   BOOST_CHECK_EQUAL(ldexp(Real(2), 5) ,  64);
   BOOST_CHECK_EQUAL(ldexp(Real(2), -5) ,  Real(2) / 32);
   Real v(512);
   int exponent;
   Real r = frexp(v, &exponent);
   BOOST_CHECK_EQUAL(r ,  0.5);
   BOOST_CHECK_EQUAL(exponent ,  10);
   BOOST_CHECK_EQUAL(v ,  512);
   v = 1 / v;
   r = frexp(v, &exponent);
   BOOST_CHECK_EQUAL(r ,  0.5);
   BOOST_CHECK_EQUAL(exponent ,  -8);
   typedef typename Real::backend_type::exponent_type e_type;
   BOOST_CHECK_EQUAL(ldexp(Real(2), e_type(5)) ,  64);
   BOOST_CHECK_EQUAL(ldexp(Real(2), e_type(-5)) ,  Real(2) / 32);
   v = 512;
   e_type exp2;
   r = frexp(v, &exp2);
   BOOST_CHECK_EQUAL(r ,  0.5);
   BOOST_CHECK_EQUAL(exp2 ,  10);
   BOOST_CHECK_EQUAL(v ,  512);
   v = 1 / v;
   r = frexp(v, &exp2);
   BOOST_CHECK_EQUAL(r ,  0.5);
   BOOST_CHECK_EQUAL(exp2 ,  -8);
   //
   // pow and exponent:
   //
   v = 3.25;
   r = pow(v, 0);
   BOOST_CHECK_EQUAL(r ,  1);
   r = pow(v, 1);
   BOOST_CHECK_EQUAL(r ,  3.25);
   r = pow(v, 2);
   BOOST_CHECK_EQUAL(r ,  boost::math::pow<2>(3.25));
   r = pow(v, 3);
   BOOST_CHECK_EQUAL(r ,  boost::math::pow<3>(3.25));
   r = pow(v, 4);
   BOOST_CHECK_EQUAL(r ,  boost::math::pow<4>(3.25));
   r = pow(v, 5);
   BOOST_CHECK_EQUAL(r ,  boost::math::pow<5>(3.25));
   r = pow(v, 6);
   BOOST_CHECK_EQUAL(r ,  boost::math::pow<6>(3.25));
   r = pow(v, 25);
   BOOST_CHECK_EQUAL(r ,  boost::math::pow<25>(Real(3.25)));
   //
   // Things that are expected errors:
   //
   BOOST_CHECK_THROW(Real("3.14L"), std::runtime_error);
   if(std::numeric_limits<Real>::is_specialized)
   {
      if(std::numeric_limits<Real>::has_infinity)
      {
         BOOST_CHECK((boost::math::isinf)(Real(20) / 0u));
      }
      else
      {
         BOOST_CHECK_THROW(Real(Real(20) / 0u), std::overflow_error);
      }
   }

   test_float_funcs<Real>(boost::mpl::bool_<std::numeric_limits<Real>::is_specialized>());
}

template <class T>
struct lexical_cast_target_type
{
   typedef typename boost::mpl::if_<
      boost::is_signed<T>,
      boost::intmax_t,
      typename boost::mpl::if_<
         boost::is_unsigned<T>,
         boost::uintmax_t,
         T
      >::type
   >::type type;
};

template <class Real, class Num>
void test_negative_mixed_minmax(boost::mpl::true_ const&)
{
   if(!std::numeric_limits<Real>::is_bounded || (std::numeric_limits<Real>::digits >= std::numeric_limits<Num>::digits))
   {
      Real mx1((std::numeric_limits<Num>::max)() - 1);
      ++mx1;
      Real mx2((std::numeric_limits<Num>::max)());
      BOOST_CHECK_EQUAL(mx1, mx2);
      mx1 = (std::numeric_limits<Num>::max)() - 1;
      ++mx1;
      mx2 = (std::numeric_limits<Num>::max)();
      BOOST_CHECK_EQUAL(mx1, mx2);

      if(!std::numeric_limits<Real>::is_bounded || (std::numeric_limits<Real>::digits > std::numeric_limits<Num>::digits))
      {
         Real mx3((std::numeric_limits<Num>::min)() + 1);
         --mx3;
         Real mx4((std::numeric_limits<Num>::min)());
         BOOST_CHECK_EQUAL(mx3, mx4);
         mx3 = (std::numeric_limits<Num>::min)() + 1;
         --mx3;
         mx4 = (std::numeric_limits<Num>::min)();
         BOOST_CHECK_EQUAL(mx3, mx4);
      }
   }
}
template <class Real, class Num>
void test_negative_mixed_minmax(boost::mpl::false_ const&)
{
}

template <class Real, class Num>
void test_negative_mixed(boost::mpl::true_ const&)
{
   typedef typename lexical_cast_target_type<Num>::type target_type;
   typedef typename boost::mpl::if_<
         boost::is_convertible<Num, Real>,
         typename boost::mpl::if_c<boost::is_integral<Num>::value && (sizeof(Num) < sizeof(int)), int, Num>::type,
         Real
      >::type cast_type;
   typedef typename boost::mpl::if_<
         boost::is_convertible<Num, Real>,
         Num,
         Real
      >::type simple_cast_type;
   std::cout << "Testing mixed arithmetic with type: " << typeid(Real).name()  << " and " << typeid(Num).name() << std::endl;
   static const int left_shift = std::numeric_limits<Num>::digits - 1;
   Num n1 = -static_cast<Num>(1uLL << ((left_shift < 63) && (left_shift > 0) ? left_shift : 10));
   Num n2 = -1;
   Num n3 = 0;
   Num n4 = -20;
   Num n5 = -8;

   test_comparisons<Real>(n1, n2, boost::is_convertible<Num, Real>());
   test_comparisons<Real>(n1, n3, boost::is_convertible<Num, Real>());
   test_comparisons<Real>(n3, n1, boost::is_convertible<Num, Real>());
   test_comparisons<Real>(n2, n1, boost::is_convertible<Num, Real>());
   test_comparisons<Real>(n1, n1, boost::is_convertible<Num, Real>());
   test_comparisons<Real>(n3, n3, boost::is_convertible<Num, Real>());

   // Default construct:
   BOOST_CHECK_EQUAL(Real(n1) ,  static_cast<cast_type>(n1));
   BOOST_CHECK_EQUAL(Real(n2) ,  static_cast<cast_type>(n2));
   BOOST_CHECK_EQUAL(Real(n3) ,  static_cast<cast_type>(n3));
   BOOST_CHECK_EQUAL(Real(n4) ,  static_cast<cast_type>(n4));
   BOOST_CHECK_EQUAL(static_cast<cast_type>(n1) ,  Real(n1));
   BOOST_CHECK_EQUAL(static_cast<cast_type>(n2) ,  Real(n2));
   BOOST_CHECK_EQUAL(static_cast<cast_type>(n3) ,  Real(n3));
   BOOST_CHECK_EQUAL(static_cast<cast_type>(n4) ,  Real(n4));
   BOOST_CHECK_EQUAL(Real(n1).template convert_to<Num>() ,  n1);
   BOOST_CHECK_EQUAL(Real(n2).template convert_to<Num>() ,  n2);
   BOOST_CHECK_EQUAL(Real(n3).template convert_to<Num>() ,  n3);
   BOOST_CHECK_EQUAL(Real(n4).template convert_to<Num>() ,  n4);
#ifndef BOOST_NO_CXX11_EXPLICIT_CONVERSION_OPERATORS
   BOOST_CHECK_EQUAL(static_cast<Num>(Real(n1)) ,  n1);
   BOOST_CHECK_EQUAL(static_cast<Num>(Real(n2)) ,  n2);
   BOOST_CHECK_EQUAL(static_cast<Num>(Real(n3)) ,  n3);
   BOOST_CHECK_EQUAL(static_cast<Num>(Real(n4)) ,  n4);
#endif
#if defined(TEST_MPFR)
   Num tol = 10 * std::numeric_limits<Num>::epsilon();
#else
   Num tol = 0;
#endif
   std::ios_base::fmtflags f = boost::is_floating_point<Num>::value ? std::ios_base::scientific : std::ios_base::fmtflags(0);
   if(std::numeric_limits<target_type>::digits <= std::numeric_limits<Real>::digits)
   {
      BOOST_CHECK_CLOSE(n1, boost::lexical_cast<target_type>(Real(n1).str(0, f)), tol);
   }
   BOOST_CHECK_CLOSE(n2, boost::lexical_cast<target_type>(Real(n2).str(0, f)), 0);
   BOOST_CHECK_CLOSE(n3, boost::lexical_cast<target_type>(Real(n3).str(0, f)), 0);
   BOOST_CHECK_CLOSE(n4, boost::lexical_cast<target_type>(Real(n4).str(0, f)), 0);
   // Assignment:
   Real r(0);
   BOOST_CHECK(r != static_cast<cast_type>(n1));
   r = static_cast<simple_cast_type>(n1);
   BOOST_CHECK_EQUAL(r ,  static_cast<cast_type>(n1));
   r = static_cast<simple_cast_type>(n2);
   BOOST_CHECK_EQUAL(r ,  static_cast<cast_type>(n2));
   r = static_cast<simple_cast_type>(n3);
   BOOST_CHECK_EQUAL(r ,  static_cast<cast_type>(n3));
   r = static_cast<simple_cast_type>(n4);
   BOOST_CHECK_EQUAL(r ,  static_cast<cast_type>(n4));
   // Addition:
   r = static_cast<simple_cast_type>(n2);
   BOOST_CHECK_EQUAL(r + static_cast<simple_cast_type>(n4) ,  static_cast<cast_type>(n2 + n4));
   BOOST_CHECK_EQUAL(Real(r + static_cast<simple_cast_type>(n4)) ,  static_cast<cast_type>(n2 + n4));
   r += static_cast<simple_cast_type>(n4);
   BOOST_CHECK_EQUAL(r ,  static_cast<cast_type>(n2 + n4));
   // subtraction:
   r = static_cast<simple_cast_type>(n4);
   BOOST_CHECK_EQUAL(r - static_cast<simple_cast_type>(n5) ,  static_cast<cast_type>(n4 - n5));
   BOOST_CHECK_EQUAL(Real(r - static_cast<simple_cast_type>(n5)) ,  static_cast<cast_type>(n4 - n5));
   r -= static_cast<simple_cast_type>(n5);
   BOOST_CHECK_EQUAL(r ,  static_cast<cast_type>(n4 - n5));
   // Multiplication:
   r = static_cast<simple_cast_type>(n2);
   BOOST_CHECK_EQUAL(r * static_cast<simple_cast_type>(n4) ,  static_cast<cast_type>(n2 * n4));
   BOOST_CHECK_EQUAL(Real(r * static_cast<simple_cast_type>(n4)) ,  static_cast<cast_type>(n2 * n4));
   r *= static_cast<simple_cast_type>(n4);
   BOOST_CHECK_EQUAL(r ,  static_cast<cast_type>(n2 * n4));
   // Division:
   r = static_cast<simple_cast_type>(n1);
   BOOST_CHECK_EQUAL(r / static_cast<simple_cast_type>(n5) ,  static_cast<cast_type>(n1 / n5));
   BOOST_CHECK_EQUAL(Real(r / static_cast<simple_cast_type>(n5)) ,  static_cast<cast_type>(n1 / n5));
   r /= static_cast<simple_cast_type>(n5);
   BOOST_CHECK_EQUAL(r ,  static_cast<cast_type>(n1 / n5));
   //
   // Extra cases for full coverage:
   //
   r = Real(n4) + static_cast<simple_cast_type>(n5);
   BOOST_CHECK_EQUAL(r ,  static_cast<cast_type>(n4 + n5));
   r = static_cast<simple_cast_type>(n4) + Real(n5);
   BOOST_CHECK_EQUAL(r ,  static_cast<cast_type>(n4 + n5));
   r = Real(n4) - static_cast<simple_cast_type>(n5);
   BOOST_CHECK_EQUAL(r ,  static_cast<cast_type>(n4 - n5));
   r = static_cast<simple_cast_type>(n4) - Real(n5);
   BOOST_CHECK_EQUAL(r ,  static_cast<cast_type>(n4 - n5));
   r = static_cast<simple_cast_type>(n4) * Real(n5);
   BOOST_CHECK_EQUAL(r ,  static_cast<cast_type>(n4 * n5));
   r = static_cast<cast_type>(4 * n4) / Real(4);
   BOOST_CHECK_EQUAL(r ,  static_cast<cast_type>(n4));

   Real a, b, c;
   a = 20;
   b = 30;
   c = -a + b;
   BOOST_CHECK_EQUAL(c ,  10);
   c = b + -a;
   BOOST_CHECK_EQUAL(c ,  10);
   n4 = 30;
   c = -a + static_cast<cast_type>(n4);
   BOOST_CHECK_EQUAL(c ,  10);
   c = static_cast<cast_type>(n4) + -a;
   BOOST_CHECK_EQUAL(c ,  10);
   c = -a + -b;
   BOOST_CHECK_EQUAL(c ,  -50);
   n4 = 4;
   c = -(a + b) + static_cast<cast_type>(n4);
   BOOST_CHECK_EQUAL(c ,  -50+4);
   n4 = 50;
   c = (a + b) - static_cast<cast_type>(n4);
   BOOST_CHECK_EQUAL(c ,  0);
   c = (a + b) - static_cast<cast_type>(n4);
   BOOST_CHECK_EQUAL(c ,  0);
   c = a - -(b + static_cast<cast_type>(n4));
   BOOST_CHECK_EQUAL(c ,  20 - -(30 + 50));
   c = -(b + static_cast<cast_type>(n4)) - a;
   BOOST_CHECK_EQUAL(c ,  -(30 + 50) - 20);
   c = a - -b;
   BOOST_CHECK_EQUAL(c ,  50);
   c = -a - b;
   BOOST_CHECK_EQUAL(c ,  -50);
   c = -a - static_cast<cast_type>(n4);
   BOOST_CHECK_EQUAL(c ,  -20 - 50);
   c = static_cast<cast_type>(n4) - -a;
   BOOST_CHECK_EQUAL(c ,  50 + 20);
   c = -(a + b) - Real(n4);
   BOOST_CHECK_EQUAL(c ,  -(20 + 30) - 50);
   c = static_cast<cast_type>(n4) - (a + b);
   BOOST_CHECK_EQUAL(c ,  0);
   c = (a + b) * static_cast<cast_type>(n4);
   BOOST_CHECK_EQUAL(c ,  50 * 50);
   c = static_cast<cast_type>(n4) * (a + b);
   BOOST_CHECK_EQUAL(c ,  50 * 50);
   c = a * -(b + static_cast<cast_type>(n4));
   BOOST_CHECK_EQUAL(c ,  20 * -(30 + 50));
   c = -(b + static_cast<cast_type>(n4)) * a;
   BOOST_CHECK_EQUAL(c ,  20 * -(30 + 50));
   c = a * -b;
   BOOST_CHECK_EQUAL(c ,  20 * -30);
   c = -a * b;
   BOOST_CHECK_EQUAL(c ,  20 * -30);
   c = -a * static_cast<cast_type>(n4);
   BOOST_CHECK_EQUAL(c ,  -20 * 50);
   c = static_cast<cast_type>(n4) * -a;
   BOOST_CHECK_EQUAL(c ,  -20 * 50);
   c = -(a + b) + a;
   BOOST_CHECK(-50 + 20);
   c = static_cast<cast_type>(n4) - (a + b);
   BOOST_CHECK_EQUAL(c ,  0);
   Real d = 10;
   c = (a + b) / d;
   BOOST_CHECK_EQUAL(c ,  5);
   c = (a + b) / (d + 0);
   BOOST_CHECK_EQUAL(c ,  5);
   c = (a + b) / static_cast<cast_type>(n4);
   BOOST_CHECK_EQUAL(c ,  1);
   c = static_cast<cast_type>(n4) / (a + b);
   BOOST_CHECK_EQUAL(c ,  1);
   d = 50;
   c = d / -(a + b);
   BOOST_CHECK_EQUAL(c ,  -1);
   c = -(a + b) / d;
   BOOST_CHECK_EQUAL(c ,  -1);
   d = 2;
   c = a / -d;
   BOOST_CHECK_EQUAL(c ,  20 / -2);
   c = -a / d;
   BOOST_CHECK_EQUAL(c ,  20 / -2);
   d = 50;
   c = -d / static_cast<cast_type>(n4);
   BOOST_CHECK_EQUAL(c ,  -1);
   c = static_cast<cast_type>(n4) / -d;
   BOOST_CHECK_EQUAL(c ,  -1);
   c = static_cast<cast_type>(n4) + a;
   BOOST_CHECK_EQUAL(c ,  70);
   c = static_cast<cast_type>(n4) - a;
   BOOST_CHECK_EQUAL(c ,  30);
   c = static_cast<cast_type>(n4) * a;
   BOOST_CHECK_EQUAL(c ,  50 * 20);

   n1 = -2;
   n2 = -3;
   n3 = -4;
   a = static_cast<cast_type>(n1);
   b = static_cast<cast_type>(n2);
   c = static_cast<cast_type>(n3);
   d = a + b * c;
   BOOST_CHECK_EQUAL(d ,  -2 + -3 * -4);
   d = static_cast<cast_type>(n1) + b * c;
   BOOST_CHECK_EQUAL(d ,  -2 + -3 * -4);
   d = a + static_cast<cast_type>(n2) * c;
   BOOST_CHECK_EQUAL(d ,  -2 + -3 * -4);
   d = a + b * static_cast<cast_type>(n3);
   BOOST_CHECK_EQUAL(d ,  -2 + -3 * -4);
   d = static_cast<cast_type>(n1) + static_cast<cast_type>(n2) * c;
   BOOST_CHECK_EQUAL(d ,  -2 + -3 * -4);
   d = static_cast<cast_type>(n1) + b * static_cast<cast_type>(n3);
   BOOST_CHECK_EQUAL(d ,  -2 + -3 * -4);
   a += static_cast<cast_type>(n2) * c;
   BOOST_CHECK_EQUAL(a ,  -2 + -3 * -4);
   a = static_cast<cast_type>(n1);
   a += b * static_cast<cast_type>(n3);
   BOOST_CHECK_EQUAL(a ,  -2 + -3 * -4);
   a = static_cast<cast_type>(n1);

   d = b * c + a;
   BOOST_CHECK_EQUAL(d ,  -2 + -3 * -4);
   d = b * c + static_cast<cast_type>(n1);
   BOOST_CHECK_EQUAL(d ,  -2 + -3 * -4);
   d = static_cast<cast_type>(n2) * c + a;
   BOOST_CHECK_EQUAL(d ,  -2 + -3 * -4);
   d = b * static_cast<cast_type>(n3) + a;
   BOOST_CHECK_EQUAL(d ,  -2 + -3 * -4);
   d = static_cast<cast_type>(n2) * c + static_cast<cast_type>(n1);
   BOOST_CHECK_EQUAL(d ,  -2 + -3 * -4);
   d = b * static_cast<cast_type>(n3) + static_cast<cast_type>(n1);
   BOOST_CHECK_EQUAL(d ,  -2 + -3 * -4);

   a = -20;
   d = a - b * c;
   BOOST_CHECK_EQUAL(d ,  -20 - -3 * -4);
   n1 = -20;
   d = static_cast<cast_type>(n1) - b * c;
   BOOST_CHECK_EQUAL(d ,  -20 - -3 * -4);
   d = a - static_cast<cast_type>(n2) * c;
   BOOST_CHECK_EQUAL(d ,  -20 - -3 * -4);
   d = a - b * static_cast<cast_type>(n3);
   BOOST_CHECK_EQUAL(d ,  -20 - -3 * -4);
   d = static_cast<cast_type>(n1) - static_cast<cast_type>(n2) * c;
   BOOST_CHECK_EQUAL(d ,  -20 - -3 * -4);
   d = static_cast<cast_type>(n1) - b * static_cast<cast_type>(n3);
   BOOST_CHECK_EQUAL(d ,  -20 - -3 * -4);
   a -= static_cast<cast_type>(n2) * c;
   BOOST_CHECK_EQUAL(a ,  -20 - -3 * -4);
   a = static_cast<cast_type>(n1);
   a -= b * static_cast<cast_type>(n3);
   BOOST_CHECK_EQUAL(a ,  -20 - -3 * -4);

   a = -2;
   d = b * c - a;
   BOOST_CHECK_EQUAL(d ,  -3 * -4 - -2);
   n1 = -2;
   d = b * c - static_cast<cast_type>(n1);
   BOOST_CHECK_EQUAL(d ,  -3 * -4 - -2);
   d = static_cast<cast_type>(n2) * c - a;
   BOOST_CHECK_EQUAL(d ,  -3 * -4 - -2);
   d = b * static_cast<cast_type>(n3) - a;
   BOOST_CHECK_EQUAL(d ,  -3 * -4 - -2);
   d = static_cast<cast_type>(n2) * c - static_cast<cast_type>(n1);
   BOOST_CHECK_EQUAL(d ,  -3 * -4 - -2);
   d = b * static_cast<cast_type>(n3) - static_cast<cast_type>(n1);
   BOOST_CHECK_EQUAL(d ,  -3 * -4 - -2);
   //
   // Conversion from min and max values:
   //
   test_negative_mixed_minmax<Real, Num>(boost::mpl::bool_<std::numeric_limits<Real>::is_integer && std::numeric_limits<Num>::is_integer>());
}

template <class Real, class Num>
void test_negative_mixed(boost::mpl::false_ const&)
{
}

template <class Real, class Num>
void test_mixed(const boost::mpl::false_&)
{
}

template <class Real, class Num>
void test_mixed(const boost::mpl::true_&)
{
   typedef typename lexical_cast_target_type<Num>::type target_type;
   typedef typename boost::mpl::if_<
         boost::is_convertible<Num, Real>,
         typename boost::mpl::if_c<boost::is_integral<Num>::value && (sizeof(Num) < sizeof(int)), int, Num>::type,
         Real
      >::type cast_type;
   typedef typename boost::mpl::if_<
         boost::is_convertible<Num, Real>,
         Num,
         Real
      >::type simple_cast_type;

   if(std::numeric_limits<Real>::is_specialized && std::numeric_limits<Real>::is_bounded && std::numeric_limits<Real>::digits < std::numeric_limits<Num>::digits)
      return;

   std::cout << "Testing mixed arithmetic with type: " << typeid(Real).name()  << " and " << typeid(Num).name() << std::endl;
   static const int left_shift = std::numeric_limits<Num>::digits - 1;
   Num n1 = static_cast<Num>(1uLL << ((left_shift < 63) && (left_shift > 0) ? left_shift : 10));
   Num n2 = 1;
   Num n3 = 0;
   Num n4 = 20;
   Num n5 = 8;

   test_comparisons<Real>(n1, n2, boost::is_convertible<Num, Real>());
   test_comparisons<Real>(n1, n3, boost::is_convertible<Num, Real>());
   test_comparisons<Real>(n1, n1, boost::is_convertible<Num, Real>());
   test_comparisons<Real>(n3, n1, boost::is_convertible<Num, Real>());
   test_comparisons<Real>(n2, n1, boost::is_convertible<Num, Real>());
   test_comparisons<Real>(n3, n3, boost::is_convertible<Num, Real>());

   // Default construct:
   BOOST_CHECK_EQUAL(Real(n1) ,  static_cast<cast_type>(n1));
   BOOST_CHECK_EQUAL(Real(n2) ,  static_cast<cast_type>(n2));
   BOOST_CHECK_EQUAL(Real(n3) ,  static_cast<cast_type>(n3));
   BOOST_CHECK_EQUAL(Real(n4) ,  static_cast<cast_type>(n4));
   BOOST_CHECK_EQUAL(Real(n1).template convert_to<Num>() ,  n1);
   BOOST_CHECK_EQUAL(Real(n2).template convert_to<Num>() ,  n2);
   BOOST_CHECK_EQUAL(Real(n3).template convert_to<Num>() ,  n3);
   BOOST_CHECK_EQUAL(Real(n4).template convert_to<Num>() ,  n4);
#ifndef BOOST_NO_CXX11_EXPLICIT_CONVERSION_OPERATORS
   BOOST_CHECK_EQUAL(static_cast<Num>(Real(n1)) ,  n1);
   BOOST_CHECK_EQUAL(static_cast<Num>(Real(n2)) ,  n2);
   BOOST_CHECK_EQUAL(static_cast<Num>(Real(n3)) ,  n3);
   BOOST_CHECK_EQUAL(static_cast<Num>(Real(n4)) ,  n4);
#endif
   BOOST_CHECK_EQUAL(static_cast<cast_type>(n1) ,  Real(n1));
   BOOST_CHECK_EQUAL(static_cast<cast_type>(n2) ,  Real(n2));
   BOOST_CHECK_EQUAL(static_cast<cast_type>(n3) ,  Real(n3));
   BOOST_CHECK_EQUAL(static_cast<cast_type>(n4) ,  Real(n4));
#if defined(TEST_MPFR)
   Num tol = 10 * std::numeric_limits<Num>::epsilon();
#else
   Num tol = 0;
#endif
   std::ios_base::fmtflags f = boost::is_floating_point<Num>::value ? std::ios_base::scientific : std::ios_base::fmtflags(0);
   if(std::numeric_limits<target_type>::digits <= std::numeric_limits<Real>::digits)
   {
      BOOST_CHECK_CLOSE(n1, boost::lexical_cast<target_type>(Real(n1).str(0, f)), tol);
   }
   BOOST_CHECK_CLOSE(n2, boost::lexical_cast<target_type>(Real(n2).str(0, f)), 0);
   BOOST_CHECK_CLOSE(n3, boost::lexical_cast<target_type>(Real(n3).str(0, f)), 0);
   BOOST_CHECK_CLOSE(n4, boost::lexical_cast<target_type>(Real(n4).str(0, f)), 0);
   // Assignment:
   Real r(0);
   BOOST_CHECK(r != static_cast<cast_type>(n1));
   r = static_cast<simple_cast_type>(n1);
   BOOST_CHECK_EQUAL(r ,  static_cast<cast_type>(n1));
   r = static_cast<simple_cast_type>(n2);
   BOOST_CHECK_EQUAL(r ,  static_cast<cast_type>(n2));
   r = static_cast<simple_cast_type>(n3);
   BOOST_CHECK_EQUAL(r ,  static_cast<cast_type>(n3));
   r = static_cast<simple_cast_type>(n4);
   BOOST_CHECK_EQUAL(r ,  static_cast<cast_type>(n4));
   // Addition:
   r = static_cast<simple_cast_type>(n2);
   BOOST_CHECK_EQUAL(r + static_cast<simple_cast_type>(n4) ,  static_cast<cast_type>(n2 + n4));
   BOOST_CHECK_EQUAL(Real(r + static_cast<simple_cast_type>(n4)) ,  static_cast<cast_type>(n2 + n4));
   r += static_cast<simple_cast_type>(n4);
   BOOST_CHECK_EQUAL(r ,  static_cast<cast_type>(n2 + n4));
   // subtraction:
   r = static_cast<simple_cast_type>(n4);
   BOOST_CHECK_EQUAL(r - static_cast<simple_cast_type>(n5) ,  static_cast<cast_type>(n4 - n5));
   BOOST_CHECK_EQUAL(Real(r - static_cast<simple_cast_type>(n5)) ,  static_cast<cast_type>(n4 - n5));
   r -= static_cast<simple_cast_type>(n5);
   BOOST_CHECK_EQUAL(r ,  static_cast<cast_type>(n4 - n5));
   // Multiplication:
   r = static_cast<simple_cast_type>(n2);
   BOOST_CHECK_EQUAL(r * static_cast<simple_cast_type>(n4) ,  static_cast<cast_type>(n2 * n4));
   BOOST_CHECK_EQUAL(Real(r * static_cast<simple_cast_type>(n4)) ,  static_cast<cast_type>(n2 * n4));
   r *= static_cast<simple_cast_type>(n4);
   BOOST_CHECK_EQUAL(r ,  static_cast<cast_type>(n2 * n4));
   // Division:
   r = static_cast<simple_cast_type>(n1);
   BOOST_CHECK_EQUAL(r / static_cast<simple_cast_type>(n5) ,  static_cast<cast_type>(n1 / n5));
   BOOST_CHECK_EQUAL(Real(r / static_cast<simple_cast_type>(n5)) ,  static_cast<cast_type>(n1 / n5));
   r /= static_cast<simple_cast_type>(n5);
   BOOST_CHECK_EQUAL(r ,  static_cast<cast_type>(n1 / n5));
   //
   // special cases for full coverage:
   //
   r = static_cast<simple_cast_type>(n5) + Real(n4);
   BOOST_CHECK_EQUAL(r ,  static_cast<cast_type>(n4 + n5));
   r = static_cast<simple_cast_type>(n4) - Real(n5);
   BOOST_CHECK_EQUAL(r ,  static_cast<cast_type>(n4 - n5));
   r = static_cast<simple_cast_type>(n4) * Real(n5);
   BOOST_CHECK_EQUAL(r ,  static_cast<cast_type>(n4 * n5));
   r = static_cast<cast_type>(4 * n4) / Real(4);
   BOOST_CHECK_EQUAL(r ,  static_cast<cast_type>(n4));

   typedef boost::mpl::bool_<
      (!std::numeric_limits<Num>::is_specialized || std::numeric_limits<Num>::is_signed)
      && (!std::numeric_limits<Real>::is_specialized || std::numeric_limits<Real>::is_signed)> signed_tag;

   test_negative_mixed<Real, Num>(signed_tag());

   n1 = 2;
   n2 = 3;
   n3 = 4;
   Real a(n1), b(n2), c(n3), d;
   d = a + b * c;
   BOOST_CHECK_EQUAL(d ,  2 + 3 * 4);
   d = static_cast<cast_type>(n1) + b * c;
   BOOST_CHECK_EQUAL(d ,  2 + 3 * 4);
   d = a + static_cast<cast_type>(n2) * c;
   BOOST_CHECK_EQUAL(d ,  2 + 3 * 4);
   d = a + b * static_cast<cast_type>(n3);
   BOOST_CHECK_EQUAL(d ,  2 + 3 * 4);
   d = static_cast<cast_type>(n1) + static_cast<cast_type>(n2) * c;
   BOOST_CHECK_EQUAL(d ,  2 + 3 * 4);
   d = static_cast<cast_type>(n1) + b * static_cast<cast_type>(n3);
   BOOST_CHECK_EQUAL(d ,  2 + 3 * 4);
   a += static_cast<cast_type>(n2) * c;
   BOOST_CHECK_EQUAL(a ,  2 + 3 * 4);
   a = static_cast<cast_type>(n1);
   a += b * static_cast<cast_type>(n3);
   BOOST_CHECK_EQUAL(a ,  2 + 3 * 4);
   a = static_cast<cast_type>(n1);

   d = b * c + a;
   BOOST_CHECK_EQUAL(d ,  2 + 3 * 4);
   d = b * c + static_cast<cast_type>(n1);
   BOOST_CHECK_EQUAL(d ,  2 + 3 * 4);
   d = static_cast<cast_type>(n2) * c + a;
   BOOST_CHECK_EQUAL(d ,  2 + 3 * 4);
   d = b * static_cast<cast_type>(n3) + a;
   BOOST_CHECK_EQUAL(d ,  2 + 3 * 4);
   d = static_cast<cast_type>(n2) * c + static_cast<cast_type>(n1);
   BOOST_CHECK_EQUAL(d ,  2 + 3 * 4);
   d = b * static_cast<cast_type>(n3) + static_cast<cast_type>(n1);
   BOOST_CHECK_EQUAL(d ,  2 + 3 * 4);

   a = 20;
   d = a - b * c;
   BOOST_CHECK_EQUAL(d ,  20 - 3 * 4);
   n1 = 20;
   d = static_cast<cast_type>(n1) - b * c;
   BOOST_CHECK_EQUAL(d ,  20 - 3 * 4);
   d = a - static_cast<cast_type>(n2) * c;
   BOOST_CHECK_EQUAL(d ,  20 - 3 * 4);
   d = a - b * static_cast<cast_type>(n3);
   BOOST_CHECK_EQUAL(d ,  20 - 3 * 4);
   d = static_cast<cast_type>(n1) - static_cast<cast_type>(n2) * c;
   BOOST_CHECK_EQUAL(d ,  20 - 3 * 4);
   d = static_cast<cast_type>(n1) - b * static_cast<cast_type>(n3);
   BOOST_CHECK_EQUAL(d ,  20 - 3 * 4);
   a -= static_cast<cast_type>(n2) * c;
   BOOST_CHECK_EQUAL(a ,  20 - 3 * 4);
   a = static_cast<cast_type>(n1);
   a -= b * static_cast<cast_type>(n3);
   BOOST_CHECK_EQUAL(a ,  20 - 3 * 4);

   a = 2;
   d = b * c - a;
   BOOST_CHECK_EQUAL(d ,  3 * 4 - 2);
   n1 = 2;
   d = b * c - static_cast<cast_type>(n1);
   BOOST_CHECK_EQUAL(d ,  3 * 4 - 2);
   d = static_cast<cast_type>(n2) * c - a;
   BOOST_CHECK_EQUAL(d ,  3 * 4 - 2);
   d = b * static_cast<cast_type>(n3) - a;
   BOOST_CHECK_EQUAL(d ,  3 * 4 - a);
   d = static_cast<cast_type>(n2) * c - static_cast<cast_type>(n1);
   BOOST_CHECK_EQUAL(d ,  3 * 4 - 2);
   d = b * static_cast<cast_type>(n3) - static_cast<cast_type>(n1);
   BOOST_CHECK_EQUAL(d ,  3 * 4 - 2);
}

template <class Real>
void test_members(Real)
{
   //
   // Test sign and zero functions:
   //
   Real a = 20;
   Real b = 30;
   BOOST_CHECK(a.sign() > 0);
   BOOST_CHECK(!a.is_zero());
   if(std::numeric_limits<Real>::is_signed)
   {
      a = -20;
      BOOST_CHECK(a.sign() < 0);
      BOOST_CHECK(!a.is_zero());
   }
   a = 0;
   BOOST_CHECK_EQUAL(a.sign() ,  0);
   BOOST_CHECK(a.is_zero());

   a = 20;
   b = 30;
   a.swap(b);
   BOOST_CHECK_EQUAL(a ,  30);
   BOOST_CHECK_EQUAL(b ,  20);
}

template <class Real>
void test_members(boost::rational<Real>)
{
}

template <class Real>
void test_signed_ops(const boost::mpl::true_&)
{
   Real a(8);
   Real b(64);
   Real c(500);
   Real d(1024);
   Real ac;
   BOOST_CHECK_EQUAL(-a ,  -8);
   ac = a;
   ac = ac - b;
   BOOST_CHECK_EQUAL(ac ,  8 - 64);
   ac = a;
   ac -= a + b;
   BOOST_CHECK_EQUAL(ac ,  -64);
   ac = a;
   ac -= b - a;
   BOOST_CHECK_EQUAL(ac ,  16 - 64);
   ac = -a;
   BOOST_CHECK_EQUAL(ac ,  -8);
   ac = a;
   ac -= -a;
   BOOST_CHECK_EQUAL(ac ,  16);
   ac = a;
   ac += -a;
   BOOST_CHECK_EQUAL(ac ,  0);
   ac = b;
   ac /= -a;
   BOOST_CHECK_EQUAL(ac ,  -8);
   ac = a;
   ac *= -a;
   BOOST_CHECK_EQUAL(ac ,  -64);
   ac = a + -b;
   BOOST_CHECK_EQUAL(ac ,  8 - 64);
   ac = -a + b;
   BOOST_CHECK_EQUAL(ac ,  -8+64);
   ac = -a + -b;
   BOOST_CHECK_EQUAL(ac ,  -72);
   ac = a + - + -b; // lots of unary operators!!
   BOOST_CHECK_EQUAL(ac ,  72);
   test_conditional(Real(-a), -a);
}
template <class Real>
void test_signed_ops(const boost::mpl::false_&)
{
}

template <class Real>
void test()
{
#if !defined(NO_MIXED_OPS) && !defined(SLOW_COMPILER)
   boost::multiprecision::is_number<Real> tag;
   test_mixed<Real, unsigned char>(tag);
   test_mixed<Real, signed char>(tag);
   test_mixed<Real, char>(tag);
   test_mixed<Real, short>(tag);
   test_mixed<Real, unsigned short>(tag);
   test_mixed<Real, int>(tag);
   test_mixed<Real, unsigned int>(tag);
   test_mixed<Real, long>(tag);
   test_mixed<Real, unsigned long>(tag);
#ifdef BOOST_HAS_LONG_LONG
   test_mixed<Real, long long>(tag);
   test_mixed<Real, unsigned long long>(tag);
#endif
   test_mixed<Real, float>(tag);
   test_mixed<Real, double>(tag);
   test_mixed<Real, long double>(tag);

   typedef typename related_type<Real>::type related_type;
   boost::mpl::bool_<boost::multiprecision::is_number<Real>::value && !boost::is_same<related_type, Real>::value>  tag2;

   test_mixed<Real, related_type>(tag2);

#endif
   //
   // Integer only functions:
   //
   test_integer_ops<Real>(typename boost::multiprecision::number_category<Real>::type());
   //
   // Real number only functions:
   //
   test_float_ops<Real>(typename boost::multiprecision::number_category<Real>::type());
   //
   // Test basic arithmetic:
   //
   Real a(8);
   Real b(64);
   Real c(500);
   Real d(1024);
   BOOST_CHECK_EQUAL(a + b ,  72);
   a += b;
   BOOST_CHECK_EQUAL(a ,  72);
   BOOST_CHECK_EQUAL(a - b ,  8);
   a -= b;
   BOOST_CHECK_EQUAL(a ,  8);
   BOOST_CHECK_EQUAL(a * b ,  8*64L);
   a *= b;
   BOOST_CHECK_EQUAL(a ,  8*64L);
   BOOST_CHECK_EQUAL(a / b ,  8);
   a /= b;
   BOOST_CHECK_EQUAL(a ,  8);
   Real ac(a);
   BOOST_CHECK_EQUAL(ac ,  a);
   ac = a * c;
   BOOST_CHECK_EQUAL(ac ,  8*500L);
   ac = 8*500L;
   ac = ac + b + c;
   BOOST_CHECK_EQUAL(ac ,  8*500L+64+500);
   ac = a;
   ac = b + c + ac;
   BOOST_CHECK_EQUAL(ac ,  8+64+500);
   ac = ac - b + c;
   BOOST_CHECK_EQUAL(ac ,  8+64+500-64+500);
   ac = a;
   ac = b + c - ac;
   BOOST_CHECK_EQUAL(ac ,  -8+64+500);
   ac = a;
   ac = ac * b;
   BOOST_CHECK_EQUAL(ac ,  8*64);
   ac = a;
   ac *= b * ac;
   BOOST_CHECK_EQUAL(ac ,  8*8*64);
   ac = b;
   ac = ac / a;
   BOOST_CHECK_EQUAL(ac ,  64/8);
   ac = b;
   ac /= ac / a;
   BOOST_CHECK_EQUAL(ac ,  64 / (64/8));
   ac = a;
   ac = b + ac * a;
   BOOST_CHECK_EQUAL(ac ,  64 * 2);
   ac = a;
   ac = b - ac * a;
   BOOST_CHECK_EQUAL(ac ,  0);
   ac = a;
   ac = b * (ac + a);
   BOOST_CHECK_EQUAL(ac ,  64 * (16));
   ac = a;
   ac = b / (ac * 1);
   BOOST_CHECK_EQUAL(ac ,  64 / 8);
   ac = a;
   ac = ac + b;
   BOOST_CHECK_EQUAL(ac ,  8 + 64);
   ac = a;
   ac = a + ac;
   BOOST_CHECK_EQUAL(ac ,  16);
   ac = a;
   ac = a - ac;
   BOOST_CHECK_EQUAL(ac ,  0);
   ac = a;
   ac += a + b;
   BOOST_CHECK_EQUAL(ac ,  80);
   ac = a;
   ac += b + a;
   BOOST_CHECK_EQUAL(ac ,  80);
   ac = +a;
   BOOST_CHECK_EQUAL(ac ,  8);
   ac = 8;
   ac = a * ac;
   BOOST_CHECK_EQUAL(ac ,  8*8);
   ac = a;
   ac = a;
   ac += +a;
   BOOST_CHECK_EQUAL(ac ,  16);
   ac = a;
   ac += b - a;
   BOOST_CHECK_EQUAL(ac ,  8 + 64-8);
   ac = a;
   ac += b*c;
   BOOST_CHECK_EQUAL(ac ,  8 + 64 * 500);
   ac = a;
   ac = a;
   ac -= +a;
   BOOST_CHECK_EQUAL(ac ,  0);
   ac = a;
   if(std::numeric_limits<Real>::is_signed || is_twos_complement_integer<Real>::value)
   {
      ac = a;
      ac -= c - b;
      BOOST_CHECK_EQUAL(ac ,  8 - (500-64));
      ac = a;
      ac -= b*c;
      BOOST_CHECK_EQUAL(ac ,  8 - 500*64);
   }
   ac = a;
   ac += ac * b;
   BOOST_CHECK_EQUAL(ac ,  8 + 8 * 64);
   if(std::numeric_limits<Real>::is_signed || is_twos_complement_integer<Real>::value)
   {
      ac = a;
      ac -= ac * b;
      BOOST_CHECK_EQUAL(ac ,  8 - 8 * 64);
   }
   ac = a * 8;
   ac *= +a;
   BOOST_CHECK_EQUAL(ac ,  64 * 8);
   ac = a;
   ac *= b * c;
   BOOST_CHECK_EQUAL(ac ,  8 * 64 * 500);
   ac = a;
   ac *= b / a;
   BOOST_CHECK_EQUAL(ac ,  8 * 64 / 8);
   ac = a;
   ac *= b + c;
   BOOST_CHECK_EQUAL(ac ,  8 * (64 + 500));
   ac = b;
   ac /= +a;
   BOOST_CHECK_EQUAL(ac ,  8);
   ac = b;
   ac /= b / a;
   BOOST_CHECK_EQUAL(ac ,  64 / (64/8));
   ac = b;
   ac /= a + Real(0);
   BOOST_CHECK_EQUAL(ac ,  8);
   //
   // simple tests with immediate values, these calls can be optimised in many backends:
   //
   ac = a + b;
   BOOST_CHECK_EQUAL(ac ,  72);
   ac = a + +b;
   BOOST_CHECK_EQUAL(ac ,  72);
   ac = +a + b;
   BOOST_CHECK_EQUAL(ac ,  72);
   ac = +a + +b;
   BOOST_CHECK_EQUAL(ac ,  72);
   ac = a;
   ac = b / ac;
   BOOST_CHECK_EQUAL(ac ,  b / a);
   //
   // Comparisons:
   //
   BOOST_CHECK_EQUAL((a == b) ,  false);
   BOOST_CHECK_EQUAL((a != b) ,  true);
   BOOST_CHECK_EQUAL((a <= b) ,  true);
   BOOST_CHECK_EQUAL((a < b) ,  true);
   BOOST_CHECK_EQUAL((a >= b) ,  false);
   BOOST_CHECK_EQUAL((a > b) ,  false);

   BOOST_CHECK_EQUAL((a+b == b) ,  false);
   BOOST_CHECK_EQUAL((a+b != b) ,  true);
   BOOST_CHECK_EQUAL((a+b >= b) ,  true);
   BOOST_CHECK_EQUAL((a+b > b) ,  true);
   BOOST_CHECK_EQUAL((a+b <= b) ,  false);
   BOOST_CHECK_EQUAL((a+b < b) ,  false);

   BOOST_CHECK_EQUAL((a == b+a) ,  false);
   BOOST_CHECK_EQUAL((a != b+a) ,  true);
   BOOST_CHECK_EQUAL((a <= b+a) ,  true);
   BOOST_CHECK_EQUAL((a < b+a) ,  true);
   BOOST_CHECK_EQUAL((a >= b+a) ,  false);
   BOOST_CHECK_EQUAL((a > b+a) ,  false);

   BOOST_CHECK_EQUAL((a+b == b+a) ,  true);
   BOOST_CHECK_EQUAL((a+b != b+a) ,  false);
   BOOST_CHECK_EQUAL((a+b <= b+a) ,  true);
   BOOST_CHECK_EQUAL((a+b < b+a) ,  false);
   BOOST_CHECK_EQUAL((a+b >= b+a) ,  true);
   BOOST_CHECK_EQUAL((a+b > b+a) ,  false);

   BOOST_CHECK_EQUAL((8 == b+a) ,  false);
   BOOST_CHECK_EQUAL((8 != b+a) ,  true);
   BOOST_CHECK_EQUAL((8 <= b+a) ,  true);
   BOOST_CHECK_EQUAL((8 < b+a) ,  true);
   BOOST_CHECK_EQUAL((8 >= b+a) ,  false);
   BOOST_CHECK_EQUAL((8 > b+a) ,  false);
   BOOST_CHECK_EQUAL((800 == b+a) ,  false);
   BOOST_CHECK_EQUAL((800 != b+a) ,  true);
   BOOST_CHECK_EQUAL((800 >= b+a) ,  true);
   BOOST_CHECK_EQUAL((800 > b+a) ,  true);
   BOOST_CHECK_EQUAL((800 <= b+a) ,  false);
   BOOST_CHECK_EQUAL((800 < b+a) ,  false);
   BOOST_CHECK_EQUAL((72 == b+a) ,  true);
   BOOST_CHECK_EQUAL((72 != b+a) ,  false);
   BOOST_CHECK_EQUAL((72 <= b+a) ,  true);
   BOOST_CHECK_EQUAL((72 < b+a) ,  false);
   BOOST_CHECK_EQUAL((72 >= b+a) ,  true);
   BOOST_CHECK_EQUAL((72 > b+a) ,  false);

   test_members(a);
   //
   // Use in Boolean context:
   //
   a = 0;
   if(a)
   {
      BOOST_ERROR("Unexpected non-zero result");
   }
   if(!a){}
   else
   {
      BOOST_ERROR("Unexpected zero result");
   }
   b = 2;
   if(!b)
   {
      BOOST_ERROR("Unexpected zero result");
   }
   if(b){}
   else
   {
      BOOST_ERROR("Unexpected non-zero result");
   }
   if(a && b)
   {
      BOOST_ERROR("Unexpected zero result");
   }
   if(!(a || b))
   {
      BOOST_ERROR("Unexpected zero result");
   }
   if(a + b){}
   else
   {
      BOOST_ERROR("Unexpected zero result");
   }
   if(b - 2)
   {
      BOOST_ERROR("Unexpected non-zero result");
   }
   //
   // Test iostreams:
   //
   std::stringstream ss;
   a = 20;
   b = 2;
   ss << a;
   ss >> c;
   BOOST_CHECK_EQUAL(a ,  c);
   ss.clear();
   ss << a + b;
   ss >> c;
   BOOST_CHECK_EQUAL(c ,  22);
   BOOST_CHECK_EQUAL(c ,  a + b);
   //
   // More cases for complete code coverage:
   //
   a = 20;
   b = 30;
   swap(a, b);
   BOOST_CHECK_EQUAL(a ,  30);
   BOOST_CHECK_EQUAL(b ,  20);
   a = 20;
   b = 30;
   std::swap(a, b);
   BOOST_CHECK_EQUAL(a ,  30);
   BOOST_CHECK_EQUAL(b ,  20);
   a = 20;
   b = 30;
   a = a + b * 2;
   BOOST_CHECK_EQUAL(a ,  20 + 30 * 2);
   a = 100;
   a = a - b * 2;
   BOOST_CHECK_EQUAL(a ,  100 - 30 * 2);
   a = 20;
   a = a * (b + 2);
   BOOST_CHECK_EQUAL(a ,  20 * (32));
   a = 20;
   a = (b + 2) * a;
   BOOST_CHECK_EQUAL(a ,  20 * (32));
   a = 90;
   b = 2;
   a = a / (b + 0);
   BOOST_CHECK_EQUAL(a ,  45);
   a = 20;
   b = 30;
   c = (a * b) + 22;
   BOOST_CHECK_EQUAL(c ,  20 * 30 + 22);
   c = 22 + (a * b);
   BOOST_CHECK_EQUAL(c ,  20 * 30 + 22);
   c = 10;
   ac = a + b * c;
   BOOST_CHECK_EQUAL(ac ,  20 + 30 * 10);
   ac = b * c + a;
   BOOST_CHECK_EQUAL(ac ,  20 + 30 * 10);
   a = a + b * c;
   BOOST_CHECK_EQUAL(a ,  20 + 30 * 10);
   a = 20;
   b = a + b * c;
   BOOST_CHECK_EQUAL(b ,  20 + 30 * 10);
   b = 30;
   c = a + b * c;
   BOOST_CHECK_EQUAL(c ,  20 + 30 * 10);
   c = 10;
   c = a + b / c;
   BOOST_CHECK_EQUAL(c ,  20 + 30 / 10);

   //
   // Test conditionals:
   //
   a = 20;
   test_conditional(a, +a);
   test_conditional(a, (a + 0));

   test_signed_ops<Real>(boost::mpl::bool_<std::numeric_limits<Real>::is_signed>());
}

