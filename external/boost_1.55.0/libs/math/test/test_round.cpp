//  (C) Copyright John Maddock 2007.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <pch.hpp>

#include <boost/math/concepts/real_concept.hpp>
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/math/special_functions/round.hpp>
#include <boost/math/special_functions/trunc.hpp>
#include <boost/math/special_functions/modf.hpp>
#include <boost/math/special_functions/sign.hpp>
#include <boost/random/mersenne_twister.hpp>

boost::mt19937 rng;

template <class T>
T get_random()
{
   //
   // Fill all the bits in T with random values,
   // likewise set the exponent to a random value
   // that will still fit inside a T, and always
   // have a remainder as well as an integer part.
   //
   int bits = boost::math::tools::digits<T>();
   int shift = 0;
   int exponent = rng() % (bits - 4);
   T result = 0;
   while(bits > 0)
   {
      result += ldexp(static_cast<T>(rng()), shift);
      shift += std::numeric_limits<int>::digits;
      bits -= std::numeric_limits<int>::digits;
   }
   return rng() & 1u ? -ldexp(frexp(result, &bits), exponent) : ldexp(frexp(result, &bits), exponent);
}

template <class T, class U>
void check_within_half(T a, U u)
{
   BOOST_MATH_STD_USING
   if(fabs(a-u) > 0.5f)
   {
      BOOST_ERROR("Rounded result differed by more than 0.5 from the original");
      std::cerr << "Values were: " << std::setprecision(35) << std::setw(40)
         << std::left << a << u << std::endl;
   }
   if((fabs(a - u) == 0.5f) && (fabs(static_cast<T>(u)) < fabs(a)))
   {
      BOOST_ERROR("Rounded result was towards zero with boost::round");
      std::cerr << "Values were: " << std::setprecision(35) << std::setw(40)
         << std::left << a << u << std::endl;
   }
}

//
// We may not have an abs overload for long long so provide a fall back:
//
template <class T>
inline T safe_abs(T const& v ...)
{
   return v < 0 ? -v : v;
}

template <class T, class U>
void check_trunc_result(T a, U u)
{
   BOOST_MATH_STD_USING
   if(fabs(a-u) >= 1)
   {
      BOOST_ERROR("Rounded result differed by more than 1 from the original");
      std::cerr << "Values were: " << std::setprecision(35) << std::setw(40)
         << std::left << a << u << std::endl;
   }
   if(abs(a) < safe_abs(u))
   {
      BOOST_ERROR("Truncated result had larger absolute value than the original");
      std::cerr << "Values were: " << std::setprecision(35) << std::setw(40)
         << std::left << a << u << std::endl;
   }
   if(fabs(static_cast<T>(u)) > fabs(a))
   {
      BOOST_ERROR("Rounded result was away from zero with boost::trunc");
      std::cerr << "Values were: " << std::setprecision(35) << std::setw(40)
         << std::left << a << u << std::endl;
   }
}

template <class T, class U>
void check_modf_result(T a, T fract, U ipart)
{
   BOOST_MATH_STD_USING
   if(fract + ipart != a)
   {
      BOOST_ERROR("Fractional and integer results do not add up to the original value");
      std::cerr << "Values were: " << std::setprecision(35) << " "
         << std::left << a << ipart << " " << fract << std::endl;
   }
   if((boost::math::sign(a) != boost::math::sign(fract)) && boost::math::sign(fract))
   {
      BOOST_ERROR("Original and fractional parts have differing signs");
      std::cerr << "Values were: " << std::setprecision(35) << " "
         << std::left << a << ipart << " " << fract << std::endl;
   }
   if((boost::math::sign(a) != boost::math::sign(ipart)) && boost::math::sign(ipart))
   {
      BOOST_ERROR("Original and integer parts have differing signs");
      std::cerr << "Values were: " << std::setprecision(35) << " "
         << std::left << a << ipart << " " << ipart << std::endl;
   }
   if(fabs(a-ipart) >= 1)
   {
      BOOST_ERROR("Rounded result differed by more than 1 from the original");
      std::cerr << "Values were: " << std::setprecision(35) << std::setw(40)
         << std::left << a << ipart << std::endl;
   }
}

template <class T>
void test_round(T, const char* name )
{
   BOOST_MATH_STD_USING
#ifdef BOOST_HAS_LONG_LONG
   using boost::math::llround;  using boost::math::lltrunc;
#endif

   std::cout << "Testing rounding with type " << name << std::endl;

   for(int i = 0; i < 1000; ++i)
   {
      T arg = get_random<T>();
      T r = round(arg);
      check_within_half(arg, r);
      r = trunc(arg);
      check_trunc_result(arg, r);
      T frac = modf(arg, &r);
      check_modf_result(arg, frac, r);

      if(abs(r) < (std::numeric_limits<int>::max)())
      {
         int i = iround(arg);
         check_within_half(arg, i);
         i = itrunc(arg);
         check_trunc_result(arg, i);
         r = modf(arg, &i);
         check_modf_result(arg, r, i);
      }
      if(std::numeric_limits<T>::digits >= std::numeric_limits<int>::digits)
      {
         int si = iround(static_cast<T>((std::numeric_limits<int>::max)()));
         check_within_half(static_cast<T>((std::numeric_limits<int>::max)()), si);
         si = iround(static_cast<T>((std::numeric_limits<int>::min)()));
         check_within_half(static_cast<T>((std::numeric_limits<int>::min)()), si);
         si = itrunc(static_cast<T>((std::numeric_limits<int>::max)()));
         check_trunc_result(static_cast<T>((std::numeric_limits<int>::max)()), si);
         si = itrunc(static_cast<T>((std::numeric_limits<int>::min)()));
         check_trunc_result(static_cast<T>((std::numeric_limits<int>::min)()), si);
      }
      if(abs(r) < (std::numeric_limits<long>::max)())
      {
         long l = lround(arg);
         check_within_half(arg, l);
         l = ltrunc(arg);
         check_trunc_result(arg, l);
         r = modf(arg, &l);
         check_modf_result(arg, r, l);
      }
      if(std::numeric_limits<T>::digits >= std::numeric_limits<long>::digits)
      {
         long k = lround(static_cast<T>((std::numeric_limits<long>::max)()));
         check_within_half(static_cast<T>((std::numeric_limits<long>::max)()), k);
         k = lround(static_cast<T>((std::numeric_limits<long>::min)()));
         check_within_half(static_cast<T>((std::numeric_limits<long>::min)()), k);
         k = ltrunc(static_cast<T>((std::numeric_limits<long>::max)()));
         check_trunc_result(static_cast<T>((std::numeric_limits<long>::max)()), k);
         k = ltrunc(static_cast<T>((std::numeric_limits<long>::min)()));
         check_trunc_result(static_cast<T>((std::numeric_limits<long>::min)()), k);
      }

#ifdef BOOST_HAS_LONG_LONG
      if(abs(r) < (std::numeric_limits<boost::long_long_type>::max)())
      {
         boost::long_long_type ll = llround(arg);
         check_within_half(arg, ll);
         ll = lltrunc(arg);
         check_trunc_result(arg, ll);
         r = modf(arg, &ll);
         check_modf_result(arg, r, ll);
      }
      if(std::numeric_limits<T>::digits >= std::numeric_limits<boost::long_long_type>::digits)
      {
         boost::long_long_type j = llround(static_cast<T>((std::numeric_limits<boost::long_long_type>::max)()));
         check_within_half(static_cast<T>((std::numeric_limits<boost::long_long_type>::max)()), j);
         j = llround(static_cast<T>((std::numeric_limits<boost::long_long_type>::min)()));
         check_within_half(static_cast<T>((std::numeric_limits<boost::long_long_type>::min)()), j);
         j = lltrunc(static_cast<T>((std::numeric_limits<boost::long_long_type>::max)()));
         check_trunc_result(static_cast<T>((std::numeric_limits<boost::long_long_type>::max)()), j);
         j = lltrunc(static_cast<T>((std::numeric_limits<boost::long_long_type>::min)()));
         check_trunc_result(static_cast<T>((std::numeric_limits<boost::long_long_type>::min)()), j);
      }
#endif
   }
   //
   // Finish off by testing the error handlers:
   //
   BOOST_CHECK_THROW(iround(static_cast<T>(1e20)), boost::math::rounding_error);
   BOOST_CHECK_THROW(iround(static_cast<T>(-1e20)), boost::math::rounding_error);
   BOOST_CHECK_THROW(lround(static_cast<T>(1e20)), boost::math::rounding_error);
   BOOST_CHECK_THROW(lround(static_cast<T>(-1e20)), boost::math::rounding_error);
#ifdef BOOST_HAS_LONG_LONG
   BOOST_CHECK_THROW(llround(static_cast<T>(1e20)), boost::math::rounding_error);
   BOOST_CHECK_THROW(llround(static_cast<T>(-1e20)), boost::math::rounding_error);
#endif
   if(std::numeric_limits<T>::has_infinity)
   {
      BOOST_CHECK_THROW(round(std::numeric_limits<T>::infinity()), boost::math::rounding_error);
      BOOST_CHECK_THROW(iround(std::numeric_limits<T>::infinity()), boost::math::rounding_error);
      BOOST_CHECK_THROW(iround(-std::numeric_limits<T>::infinity()), boost::math::rounding_error);
      BOOST_CHECK_THROW(lround(std::numeric_limits<T>::infinity()), boost::math::rounding_error);
      BOOST_CHECK_THROW(lround(-std::numeric_limits<T>::infinity()), boost::math::rounding_error);
   #ifdef BOOST_HAS_LONG_LONG
      BOOST_CHECK_THROW(llround(std::numeric_limits<T>::infinity()), boost::math::rounding_error);
      BOOST_CHECK_THROW(llround(-std::numeric_limits<T>::infinity()), boost::math::rounding_error);
   #endif
   }
   if(std::numeric_limits<T>::has_quiet_NaN)
   {
      BOOST_CHECK_THROW(round(std::numeric_limits<T>::quiet_NaN()), boost::math::rounding_error);
      BOOST_CHECK_THROW(iround(std::numeric_limits<T>::quiet_NaN()), boost::math::rounding_error);
      BOOST_CHECK_THROW(lround(std::numeric_limits<T>::quiet_NaN()), boost::math::rounding_error);
   #ifdef BOOST_HAS_LONG_LONG
      BOOST_CHECK_THROW(llround(std::numeric_limits<T>::quiet_NaN()), boost::math::rounding_error);
   #endif
   }
   BOOST_CHECK_THROW(itrunc(static_cast<T>(1e20)), boost::math::rounding_error);
   BOOST_CHECK_THROW(itrunc(static_cast<T>(-1e20)), boost::math::rounding_error);
   BOOST_CHECK_THROW(ltrunc(static_cast<T>(1e20)), boost::math::rounding_error);
   BOOST_CHECK_THROW(ltrunc(static_cast<T>(-1e20)), boost::math::rounding_error);
#ifdef BOOST_HAS_LONG_LONG
   BOOST_CHECK_THROW(lltrunc(static_cast<T>(1e20)), boost::math::rounding_error);
   BOOST_CHECK_THROW(lltrunc(static_cast<T>(-1e20)), boost::math::rounding_error);
#endif
   if(std::numeric_limits<T>::has_infinity)
   {
      BOOST_CHECK_THROW(trunc(std::numeric_limits<T>::infinity()), boost::math::rounding_error);
      BOOST_CHECK_THROW(itrunc(std::numeric_limits<T>::infinity()), boost::math::rounding_error);
      BOOST_CHECK_THROW(itrunc(-std::numeric_limits<T>::infinity()), boost::math::rounding_error);
      BOOST_CHECK_THROW(ltrunc(std::numeric_limits<T>::infinity()), boost::math::rounding_error);
      BOOST_CHECK_THROW(ltrunc(-std::numeric_limits<T>::infinity()), boost::math::rounding_error);
   #ifdef BOOST_HAS_LONG_LONG
      BOOST_CHECK_THROW(lltrunc(std::numeric_limits<T>::infinity()), boost::math::rounding_error);
      BOOST_CHECK_THROW(lltrunc(-std::numeric_limits<T>::infinity()), boost::math::rounding_error);
   #endif
   }
   if(std::numeric_limits<T>::has_quiet_NaN)
   {
      BOOST_CHECK_THROW(trunc(std::numeric_limits<T>::quiet_NaN()), boost::math::rounding_error);
      BOOST_CHECK_THROW(itrunc(std::numeric_limits<T>::quiet_NaN()), boost::math::rounding_error);
      BOOST_CHECK_THROW(ltrunc(std::numeric_limits<T>::quiet_NaN()), boost::math::rounding_error);
   #ifdef BOOST_HAS_LONG_LONG
      BOOST_CHECK_THROW(lltrunc(std::numeric_limits<T>::quiet_NaN()), boost::math::rounding_error);
   #endif
   }
   if(std::numeric_limits<T>::digits >= std::numeric_limits<int>::digits)
   {
      BOOST_CHECK_THROW(itrunc(static_cast<T>((std::numeric_limits<int>::max)()) + 1), boost::math::rounding_error);
      BOOST_CHECK_THROW(itrunc(static_cast<T>((std::numeric_limits<int>::min)()) - 1), boost::math::rounding_error);
   }
   if(std::numeric_limits<T>::digits >= std::numeric_limits<long>::digits)
   {
      BOOST_CHECK_THROW(ltrunc(static_cast<T>((std::numeric_limits<long>::max)()) + 1), boost::math::rounding_error);
      BOOST_CHECK_THROW(ltrunc(static_cast<T>((std::numeric_limits<long>::min)()) - 1), boost::math::rounding_error);
   }
#ifndef BOOST_NO_LONG_LONG
   if(std::numeric_limits<T>::digits >= std::numeric_limits<boost::long_long_type>::digits)
   {
      BOOST_CHECK_THROW(lltrunc(static_cast<T>((std::numeric_limits<boost::long_long_type>::max)()) + 1), boost::math::rounding_error);
      BOOST_CHECK_THROW(lltrunc(static_cast<T>((std::numeric_limits<boost::long_long_type>::min)()) - 1), boost::math::rounding_error);
   }
#endif
   if(std::numeric_limits<T>::digits >= std::numeric_limits<int>::digits)
   {
      BOOST_CHECK_THROW(iround(static_cast<T>((std::numeric_limits<int>::max)()) + 1), boost::math::rounding_error);
      BOOST_CHECK_THROW(iround(static_cast<T>((std::numeric_limits<int>::min)()) - 1), boost::math::rounding_error);
   }
   if(std::numeric_limits<T>::digits >= std::numeric_limits<long>::digits)
   {
      BOOST_CHECK_THROW(lround(static_cast<T>((std::numeric_limits<long>::max)()) + 1), boost::math::rounding_error);
      BOOST_CHECK_THROW(lround(static_cast<T>((std::numeric_limits<long>::min)()) - 1), boost::math::rounding_error);
   }
#ifndef BOOST_NO_LONG_LONG
   if(std::numeric_limits<T>::digits >= std::numeric_limits<boost::long_long_type>::digits)
   {
      BOOST_CHECK_THROW(llround(static_cast<T>((std::numeric_limits<boost::long_long_type>::max)()) + 1), boost::math::rounding_error);
      BOOST_CHECK_THROW(llround(static_cast<T>((std::numeric_limits<boost::long_long_type>::min)()) - 1), boost::math::rounding_error);
   }
#endif
   //
   // try non-throwing error handlers:
   //
   boost::math::policies::policy<boost::math::policies::rounding_error<boost::math::policies::ignore_error> > pol;

   if(std::numeric_limits<T>::digits >= std::numeric_limits<int>::digits)
   {
      BOOST_CHECK_EQUAL(iround((std::numeric_limits<int>::max)() + T(1.0), pol), (std::numeric_limits<int>::max)());
      BOOST_CHECK_EQUAL(iround((std::numeric_limits<int>::min)() - T(1.0), pol), (std::numeric_limits<int>::min)());
      BOOST_CHECK_EQUAL(itrunc((std::numeric_limits<int>::max)() + T(1.0), pol), (std::numeric_limits<int>::max)());
      BOOST_CHECK_EQUAL(itrunc((std::numeric_limits<int>::min)() - T(1.0), pol), (std::numeric_limits<int>::min)());
   }
   if(std::numeric_limits<T>::digits >= std::numeric_limits<long>::digits)
   {
      BOOST_CHECK_EQUAL(lround((std::numeric_limits<long>::max)() + T(1.0), pol), (std::numeric_limits<long>::max)());
      BOOST_CHECK_EQUAL(lround((std::numeric_limits<long>::min)() - T(1.0), pol), (std::numeric_limits<long>::min)());
      BOOST_CHECK_EQUAL(ltrunc((std::numeric_limits<long>::max)() + T(1.0), pol), (std::numeric_limits<long>::max)());
      BOOST_CHECK_EQUAL(ltrunc((std::numeric_limits<long>::min)() - T(1.0), pol), (std::numeric_limits<long>::min)());
   }
#ifndef BOOST_NO_LONG_LONG
   if(std::numeric_limits<T>::digits >= std::numeric_limits<long long>::digits)
   {
      BOOST_CHECK_EQUAL(llround((std::numeric_limits<long long>::max)() + T(1.0), pol), (std::numeric_limits<long long>::max)());
      BOOST_CHECK_EQUAL(llround((std::numeric_limits<long long>::min)() - T(1.0), pol), (std::numeric_limits<long long>::min)());
      BOOST_CHECK_EQUAL(lltrunc((std::numeric_limits<long long>::max)() + T(1.0), pol), (std::numeric_limits<long long>::max)());
      BOOST_CHECK_EQUAL(lltrunc((std::numeric_limits<long long>::min)() - T(1.0), pol), (std::numeric_limits<long long>::min)());
   }
#endif
   // Again with bigger value:
   T big = 1e20f;
   BOOST_CHECK_EQUAL(iround(big, pol), (std::numeric_limits<int>::max)());
   BOOST_CHECK_EQUAL(lround(big, pol), (std::numeric_limits<long>::max)());
   BOOST_CHECK_EQUAL(iround(-big, pol), (std::numeric_limits<int>::min)());
   BOOST_CHECK_EQUAL(lround(-big, pol), (std::numeric_limits<long>::min)());
   BOOST_CHECK_EQUAL(itrunc(big, pol), (std::numeric_limits<int>::max)());
   BOOST_CHECK_EQUAL(ltrunc(big, pol), (std::numeric_limits<long>::max)());
   BOOST_CHECK_EQUAL(itrunc(-big, pol), (std::numeric_limits<int>::min)());
   BOOST_CHECK_EQUAL(ltrunc(-big, pol), (std::numeric_limits<long>::min)());
#ifndef BOOST_NO_LONG_LONG
   BOOST_CHECK_EQUAL(llround(big, pol), (std::numeric_limits<long long>::max)());
   BOOST_CHECK_EQUAL(llround(-big, pol), (std::numeric_limits<long long>::min)());
   BOOST_CHECK_EQUAL(lltrunc(big, pol), (std::numeric_limits<long long>::max)());
   BOOST_CHECK_EQUAL(lltrunc(-big, pol), (std::numeric_limits<long long>::min)());
#endif
}

BOOST_AUTO_TEST_CASE( test_main )
{
   test_round(0.1F, "float");
   test_round(0.1, "double");
#ifndef BOOST_MATH_NO_LONG_DOUBLE_MATH_FUNCTIONS
   test_round(0.1L, "long double");
   test_round(boost::math::concepts::real_concept(0.1), "real_concept");
#else
   std::cout << "<note>The long double tests have been disabled on this platform "
      "either because the long double overloads of the usual math functions are "
      "not available at all, or because they are too inaccurate for these tests "
      "to pass.</note>" << std::cout;
#endif
   
}





