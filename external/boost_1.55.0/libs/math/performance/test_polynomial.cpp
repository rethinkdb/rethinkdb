//  Copyright John Maddock 2007.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "required_defines.hpp"

#include "performance_measure.hpp"

#include <boost/math/tools/rational.hpp>
#include <boost/cstdint.hpp>

static const double num[13] = {
   static_cast<double>(56906521.91347156388090791033559122686859L),
   static_cast<double>(103794043.1163445451906271053616070238554L),
   static_cast<double>(86363131.28813859145546927288977868422342L),
   static_cast<double>(43338889.32467613834773723740590533316085L),
   static_cast<double>(14605578.08768506808414169982791359218571L),
   static_cast<double>(3481712.15498064590882071018964774556468L),
   static_cast<double>(601859.6171681098786670226533699352302507L),
   static_cast<double>(75999.29304014542649875303443598909137092L),
   static_cast<double>(6955.999602515376140356310115515198987526L),
   static_cast<double>(449.9445569063168119446858607650988409623L),
   static_cast<double>(19.51992788247617482847860966235652136208L),
   static_cast<double>(0.5098416655656676188125178644804694509993L),
   static_cast<double>(0.006061842346248906525783753964555936883222L)
};
static const double denom[13] = {
   static_cast<double>(0u),
   static_cast<double>(39916800u),
   static_cast<double>(120543840u),
   static_cast<double>(150917976u),
   static_cast<double>(105258076u),
   static_cast<double>(45995730u),
   static_cast<double>(13339535u),
   static_cast<double>(2637558u),
   static_cast<double>(357423u),
   static_cast<double>(32670u),
   static_cast<double>(1925u),
   static_cast<double>(66u),
   static_cast<double>(1u)
};
static const boost::uint32_t denom_int[13] = {
   static_cast<boost::uint32_t>(0u),
   static_cast<boost::uint32_t>(39916800u),
   static_cast<boost::uint32_t>(120543840u),
   static_cast<boost::uint32_t>(150917976u),
   static_cast<boost::uint32_t>(105258076u),
   static_cast<boost::uint32_t>(45995730u),
   static_cast<boost::uint32_t>(13339535u),
   static_cast<boost::uint32_t>(2637558u),
   static_cast<boost::uint32_t>(357423u),
   static_cast<boost::uint32_t>(32670u),
   static_cast<boost::uint32_t>(1925u),
   static_cast<boost::uint32_t>(66u),
   static_cast<boost::uint32_t>(1u)
};

template <class T, class U>
U evaluate_polynomial_0(const T* poly, U const& z, std::size_t count)
{
   U sum = static_cast<U>(poly[count - 1]);
   for(int i = static_cast<int>(count) - 2; i >= 0; --i)
   {
      sum *= z;
      sum += static_cast<U>(poly[i]);
   }
   return sum;
}

template <class T, class V>
inline V evaluate_polynomial_1(const T* a, const V& x)
{
   return static_cast<V>((((((((((((a[12] * x + a[11]) * x + a[10]) * x + a[9]) * x + a[8]) * x + a[7]) * x + a[6]) * x + a[5]) * x + a[4]) * x + a[3]) * x + a[2]) * x + a[1]) * x + a[0]);
}

template <class T, class V>
inline V evaluate_polynomial_2(const T* a, const V& x)
{
   V x2 = x * x;
   return static_cast<V>((((((a[12] * x2 + a[10]) * x2 + a[8]) * x2 + a[6]) * x2 + a[4]) * x2 + a[2]) * x2 + a[0] + (((((a[11] * x2 + a[9]) * x2 + a[7]) * x2 + a[5]) * x2 + a[3]) * x2 + a[1]) * x);
}

template <class T, class V>
inline V evaluate_polynomial_3(const T* a, const V& x)
{
   V x2 = x * x;
   V t[2];
   t[0] = static_cast<V>(a[12] * x2 + a[10]);
   t[1] = static_cast<V>(a[11] * x2 + a[9]);
   t[0] *= x2;
   t[1] *= x2;
   t[0] += a[8];
   t[1] += a[7];
   t[0] *= x2;
   t[1] *= x2;
   t[0] += a[6];
   t[1] += a[5];
   t[0] *= x2;
   t[1] *= x2;
   t[0] += a[4];
   t[1] += a[3];
   t[0] *= x2;
   t[1] *= x2;
   t[0] += a[2];
   t[1] += a[1];
   t[0] *= x2;
   t[0] += a[0];
   t[1] *= x;
   return t[0] + t[1];
}

template <class T, class U, class V>
V evaluate_rational_0(const T* num, const U* denom, const V& z_, std::size_t count)
{
   V z(z_);
   V s1, s2;
   if(z <= 1)
   {
      s1 = static_cast<V>(num[count-1]);
      s2 = static_cast<V>(denom[count-1]);
      for(int i = (int)count - 2; i >= 0; --i)
      {
         s1 *= z;
         s2 *= z;
         s1 += num[i];
         s2 += denom[i];
      }
   }
   else
   {
      z = 1 / z;
      s1 = static_cast<V>(num[0]);
      s2 = static_cast<V>(denom[0]);
      for(unsigned i = 1; i < count; ++i)
      {
         s1 *= z;
         s2 *= z;
         s1 += num[i];
         s2 += denom[i];
      }
   }
   return s1 / s2;
}

template <class T, class U, class V>
inline V evaluate_rational_1(const T* a, const U* b, const V& x)
{
   if(x <= 1)
     return static_cast<V>(((((((((((((a[12] * x + a[11]) * x + a[10]) * x + a[9]) * x + a[8]) * x + a[7]) * x + a[6]) * x + a[5]) * x + a[4]) * x + a[3]) * x + a[2]) * x + a[1]) * x + a[0]) / ((((((((((((b[12] * x + b[11]) * x + b[10]) * x + b[9]) * x + b[8]) * x + b[7]) * x + b[6]) * x + b[5]) * x + b[4]) * x + b[3]) * x + b[2]) * x + b[1]) * x + b[0]));
   else
   {
      V z = 1 / x;
      return static_cast<V>(((((((((((((a[0] * z + a[1]) * z + a[2]) * z + a[3]) * z + a[4]) * z + a[5]) * z + a[6]) * z + a[7]) * z + a[8]) * z + a[9]) * z + a[10]) * z + a[11]) * z + a[12]) / ((((((((((((b[0] * z + b[1]) * z + b[2]) * z + b[3]) * z + b[4]) * z + b[5]) * z + b[6]) * z + b[7]) * z + b[8]) * z + b[9]) * z + b[10]) * z + b[11]) * z + b[12]));
   }
}

template <class T, class U, class V>
inline V evaluate_rational_2(const T* a, const U* b, const V& x)
{
   if(x <= 1)
   {
      V x2 = x * x;
      return static_cast<V>(((((((a[12] * x2 + a[10]) * x2 + a[8]) * x2 + a[6]) * x2 + a[4]) * x2 + a[2]) * x2 + a[0] + (((((a[11] * x2 + a[9]) * x2 + a[7]) * x2 + a[5]) * x2 + a[3]) * x2 + a[1]) * x) / ((((((b[12] * x2 + b[10]) * x2 + b[8]) * x2 + b[6]) * x2 + b[4]) * x2 + b[2]) * x2 + b[0] + (((((b[11] * x2 + b[9]) * x2 + b[7]) * x2 + b[5]) * x2 + b[3]) * x2 + b[1]) * x));
   }
   else
   {
      V z = 1 / x;
      V z2 = 1 / (x * x);
      return static_cast<V>(((((((a[0] * z2 + a[2]) * z2 + a[4]) * z2 + a[6]) * z2 + a[8]) * z2 + a[10]) * z2 + a[12] + (((((a[1] * z2 + a[3]) * z2 + a[5]) * z2 + a[7]) * z2 + a[9]) * z2 + a[11]) * z) / ((((((b[0] * z2 + b[2]) * z2 + b[4]) * z2 + b[6]) * z2 + b[8]) * z2 + b[10]) * z2 + b[12] + (((((b[1] * z2 + b[3]) * z2 + b[5]) * z2 + b[7]) * z2 + b[9]) * z2 + b[11]) * z));
   }
}

template <class T, class U, class V>
inline V evaluate_rational_3(const T* a, const U* b, const V& x)
{
   if(x <= 1)
   {
      V x2 = x * x;
      V t[4];
      t[0] = a[12] * x2 + a[10];
      t[1] = a[11] * x2 + a[9];
      t[2] = b[12] * x2 + b[10];
      t[3] = b[11] * x2 + b[9];
      t[0] *= x2;
      t[1] *= x2;
      t[2] *= x2;
      t[3] *= x2;
      t[0] += a[8];
      t[1] += a[7];
      t[2] += b[8];
      t[3] += b[7];
      t[0] *= x2;
      t[1] *= x2;
      t[2] *= x2;
      t[3] *= x2;
      t[0] += a[6];
      t[1] += a[5];
      t[2] += b[6];
      t[3] += b[5];
      t[0] *= x2;
      t[1] *= x2;
      t[2] *= x2;
      t[3] *= x2;
      t[0] += a[4];
      t[1] += a[3];
      t[2] += b[4];
      t[3] += b[3];
      t[0] *= x2;
      t[1] *= x2;
      t[2] *= x2;
      t[3] *= x2;
      t[0] += a[2];
      t[1] += a[1];
      t[2] += b[2];
      t[3] += b[1];
      t[0] *= x2;
      t[2] *= x2;
      t[0] += a[0];
      t[2] += b[0];
      t[1] *= x;
      t[3] *= x;
      return (t[0] + t[1]) / (t[2] + t[3]);
   }
   else
   {
      V z = 1 / x;
      V z2 = 1 / (x * x);
      V t[4];
      t[0] = a[0] * z2 + a[2];
      t[1] = a[1] * z2 + a[3];
      t[2] = b[0] * z2 + b[2];
      t[3] = b[1] * z2 + b[3];
      t[0] *= z2;
      t[1] *= z2;
      t[2] *= z2;
      t[3] *= z2;
      t[0] += a[4];
      t[1] += a[5];
      t[2] += b[4];
      t[3] += b[5];
      t[0] *= z2;
      t[1] *= z2;
      t[2] *= z2;
      t[3] *= z2;
      t[0] += a[6];
      t[1] += a[7];
      t[2] += b[6];
      t[3] += b[7];
      t[0] *= z2;
      t[1] *= z2;
      t[2] *= z2;
      t[3] *= z2;
      t[0] += a[8];
      t[1] += a[9];
      t[2] += b[8];
      t[3] += b[9];
      t[0] *= z2;
      t[1] *= z2;
      t[2] *= z2;
      t[3] *= z2;
      t[0] += a[10];
      t[1] += a[11];
      t[2] += b[10];
      t[3] += b[11];
      t[0] *= z2;
      t[2] *= z2;
      t[0] += a[12];
      t[2] += b[12];
      t[1] *= z;
      t[3] *= z;
      return (t[0] + t[1]) / (t[2] + t[3]);
   }
}



BOOST_MATH_PERFORMANCE_TEST(polynomial_evaluate_0, "Polynomial-method-0")
{
   double result = 0;
   for(double i = 1; i < 1000; i += 0.5)
      result += evaluate_polynomial_0(num, i, 13) / evaluate_polynomial_0(denom, i, 13);
   consume_result(result);
   set_call_count(1000);
}

BOOST_MATH_PERFORMANCE_TEST(polynomial_evaluate_1, "Polynomial-method-1")
{
   double result = 0;
   for(double i = 1; i < 1000; i += 0.5)
      result += evaluate_polynomial_1(num, i) / evaluate_polynomial_1(denom, i);
   consume_result(result);
   set_call_count(1000);
}

BOOST_MATH_PERFORMANCE_TEST(polynomial_evaluate_2, "Polynomial-method-2")
{
   double result = 0;
   for(double i = 1; i < 1000; i += 0.5)
      result += evaluate_polynomial_2(num, i) / evaluate_polynomial_2(denom, i);
   consume_result(result);
   set_call_count(1000);
}

BOOST_MATH_PERFORMANCE_TEST(polynomial_evaluate_3, "Polynomial-method-3")
{
   double result = 0;
   for(double i = 1; i < 1000; i += 0.5)
      result += evaluate_polynomial_3(num, i) / evaluate_polynomial_3(denom, i);
   consume_result(result);
   set_call_count(1000);
}

BOOST_MATH_PERFORMANCE_TEST(polynomial_m_evaluate_0, "Polynomial-mixed-method-0")
{
   double result = 0;
   for(double i = 1; i < 1000; i += 0.5)
      result += evaluate_polynomial_0(num, i, 13) / evaluate_polynomial_0(denom_int, i, 13);
   consume_result(result);
   set_call_count(1000);
}

BOOST_MATH_PERFORMANCE_TEST(polynomial_m_evaluate_1, "Polynomial-mixed-method-1")
{
   double result = 0;
   for(double i = 1; i < 1000; i += 0.5)
      result += evaluate_polynomial_1(num, i) / evaluate_polynomial_1(denom_int, i);
   consume_result(result);
   set_call_count(1000);
}

BOOST_MATH_PERFORMANCE_TEST(polynomial_m_evaluate_2, "Polynomial-mixed-method-2")
{
   double result = 0;
   for(double i = 1; i < 1000; i += 0.5)
      result += evaluate_polynomial_2(num, i) / evaluate_polynomial_2(denom_int, i);
   consume_result(result);
   set_call_count(1000);
}

BOOST_MATH_PERFORMANCE_TEST(polynomial_m_evaluate_3, "Polynomial-mixed-method-3")
{
   double result = 0;
   for(double i = 1; i < 1000; i += 0.5)
      result += evaluate_polynomial_3(num, i) / evaluate_polynomial_3(denom_int, i);
   consume_result(result);
   set_call_count(1000);
}

BOOST_MATH_PERFORMANCE_TEST(rational_evaluate_0, "Rational-method-0")
{
   double result = 0;
   for(double i = 1; i < 1000; i += 0.5)
      result += evaluate_rational_0(num, denom, i, 13);
   consume_result(result);
   set_call_count(1000);
}

BOOST_MATH_PERFORMANCE_TEST(rational_evaluate_1, "Rational-method-1")
{
   double result = 0;
   for(double i = 1; i < 1000; i += 0.5)
      result += evaluate_rational_1(num, denom, i);
   consume_result(result);
   set_call_count(1000);
}

BOOST_MATH_PERFORMANCE_TEST(rational_evaluate_2, "Rational-method-2")
{
   double result = 0;
   for(double i = 1; i < 1000; i += 0.5)
      result += evaluate_rational_2(num, denom, i);
   consume_result(result);
   set_call_count(1000);
}

BOOST_MATH_PERFORMANCE_TEST(rational_evaluate_3, "Rational-method-3")
{
   double result = 0;
   for(double i = 1; i < 1000; i += 0.5)
      result += evaluate_rational_3(num, denom, i);
   consume_result(result);
   set_call_count(1000);
}

BOOST_MATH_PERFORMANCE_TEST(rational_m_evaluate_0, "Rational-mixed-method-0")
{
   double result = 0;
   for(double i = 1; i < 1000; i += 0.5)
      result += evaluate_rational_0(num, denom_int, i, 13);
   consume_result(result);
   set_call_count(1000);
}

BOOST_MATH_PERFORMANCE_TEST(rational_m_evaluate_1, "Rational-mixed-method-1")
{
   double result = 0;
   for(double i = 1; i < 1000; i += 0.5)
      result += evaluate_rational_1(num, denom_int, i);
   consume_result(result);
   set_call_count(1000);
}

BOOST_MATH_PERFORMANCE_TEST(rational_m_evaluate_2, "Rational-mixed-method-2")
{
   double result = 0;
   for(double i = 1; i < 1000; i += 0.5)
      result += evaluate_rational_2(num, denom_int, i);
   consume_result(result);
   set_call_count(1000);
}

BOOST_MATH_PERFORMANCE_TEST(rational_m_evaluate_3, "Rational-mixed-method-3")
{
   double result = 0;
   for(double i = 1; i < 1000; i += 0.5)
      result += evaluate_rational_3(num, denom_int, i);
   consume_result(result);
   set_call_count(1000);
}


