// Copyright John Maddock 2011.

// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifdef _MSC_VER
#  define _SCL_SECURE_NO_WARNINGS
#endif

#if !defined(TEST_MPZ) && !defined(TEST_TOMMATH) && !defined(TEST_CPP_INT)
#  define TEST_TOMMATH
#  define TEST_MPZ
#  define TEST_CPP_INT

#ifdef _MSC_VER
#pragma message("CAUTION!!: No backend type specified so testing everything.... this will take some time!!")
#endif
#ifdef __GNUC__
#pragma warning "CAUTION!!: No backend type specified so testing everything.... this will take some time!!"
#endif

#endif

#if defined(TEST_MPZ)
#include <boost/multiprecision/gmp.hpp>
#endif
#if defined(TEST_TOMMATH)
#include <boost/multiprecision/tommath.hpp>
#endif
#ifdef TEST_CPP_INT
#include <boost/multiprecision/cpp_int.hpp>
#endif

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int.hpp>
#include "test.hpp"
#include <iostream>
#include <iomanip>

#ifdef BOOST_MSVC
#pragma warning(disable:4127)
#endif

template <class T>
struct unchecked_type{ typedef T type; };

#ifdef TEST_CPP_INT
template <unsigned MinBits, unsigned MaxBits, boost::multiprecision::cpp_integer_type SignType, boost::multiprecision::cpp_int_check_type Checked, class Allocator, boost::multiprecision::expression_template_option ExpressionTemplates>
struct unchecked_type<boost::multiprecision::number<boost::multiprecision::cpp_int_backend<MinBits, MaxBits, SignType, Checked, Allocator>, ExpressionTemplates> >
{
   typedef boost::multiprecision::number<boost::multiprecision::cpp_int_backend<MinBits, MaxBits, SignType, boost::multiprecision::unchecked, Allocator>, ExpressionTemplates> type;
};
#endif

template <class T>
T generate_random()
{
   typedef typename unchecked_type<T>::type unchecked_T;

   static const unsigned limbs = std::numeric_limits<T>::is_specialized && std::numeric_limits<T>::is_bounded ? std::numeric_limits<T>::digits / std::numeric_limits<unsigned>::digits + 3 : 20;

   static boost::random::uniform_int_distribution<unsigned> ui(0, limbs);
   static boost::random::mt19937 gen;
   unchecked_T val = gen();
   unsigned lim = ui(gen);
   for(unsigned i = 0; i < lim; ++i)
   {
      val *= (gen.max)();
      val += gen();
   }
   return val;
}

template <class T>
void do_round_trip(const T& val, std::ios_base::fmtflags f)
{
   std::stringstream ss;
#ifndef BOOST_NO_CXX11_NUMERIC_LIMITS
   ss << std::setprecision(std::numeric_limits<T>::max_digits10);
#else
   ss << std::setprecision(std::numeric_limits<T>::digits10 + 5);
#endif
   ss.flags(f);
   ss << val;
   T new_val = static_cast<T>(ss.str());
   BOOST_CHECK_EQUAL(new_val, val);
   new_val = static_cast<T>(val.str(0, f));
   BOOST_CHECK_EQUAL(new_val, val);
}

template <class T>
void do_round_trip(const T& val)
{
   do_round_trip(val, std::ios_base::fmtflags(0));
   if(val >= 0)
   {
      do_round_trip(val, std::ios_base::fmtflags(std::ios_base::showbase|std::ios_base::hex));
      do_round_trip(val, std::ios_base::fmtflags(std::ios_base::showbase|std::ios_base::oct));
   }
}

template <class T>
void negative_round_trip(T val, const boost::mpl::true_&)
{
   do_round_trip(T(-val));
}
template <class T>
void negative_round_trip(T, const boost::mpl::false_&)
{
}

template <class T>
void negative_spots(const boost::mpl::true_&)
{
   BOOST_CHECK_EQUAL(T(-1002).str(), "-1002");
   if(!std::numeric_limits<T>::is_modulo)
   {
      BOOST_CHECK_THROW(T(-2).str(0, std::ios_base::oct), std::runtime_error);
      BOOST_CHECK_THROW(T(-2).str(0, std::ios_base::hex), std::runtime_error);
   }
}
template <class T>
void negative_spots(const boost::mpl::false_&)
{
}

template <class T>
void test_round_trip()
{
   for(unsigned i = 0; i < 1000; ++i)
   {
      T val = generate_random<T>();
      do_round_trip(val);
      negative_round_trip(val, boost::mpl::bool_<std::numeric_limits<T>::is_signed>());
   }

   BOOST_CHECK_EQUAL(T(1002).str(), "1002");
   BOOST_CHECK_EQUAL(T(1002).str(0, std::ios_base::showpos), "+1002");
   BOOST_CHECK_EQUAL(T(1002).str(0, std::ios_base::oct), "1752");
   BOOST_CHECK_EQUAL(T(1002).str(0, std::ios_base::oct|std::ios_base::showbase), "01752");
   BOOST_CHECK_EQUAL(boost::to_lower_copy(T(1002).str(0, std::ios_base::hex)), "3ea");
   BOOST_CHECK_EQUAL(boost::to_lower_copy(T(1002).str(0, std::ios_base::hex|std::ios_base::showbase)), "0x3ea");
   BOOST_CHECK_EQUAL(T(1002).str(0, std::ios_base::dec), "1002");
   BOOST_CHECK_EQUAL(T(1002).str(0, std::ios_base::dec|std::ios_base::showbase), "1002");

   negative_spots<T>(boost::mpl::bool_<std::numeric_limits<T>::is_signed>());
}

int main()
{
#ifdef TEST_MPZ
   test_round_trip<boost::multiprecision::mpz_int>();
#endif
#ifdef TEST_TOMMATH
   test_round_trip<boost::multiprecision::tom_int>();
#endif
#ifdef TEST_CPP_INT
   test_round_trip<boost::multiprecision::cpp_int>();
   test_round_trip<boost::multiprecision::checked_int1024_t>();
   test_round_trip<boost::multiprecision::checked_uint512_t >();
   test_round_trip<boost::multiprecision::number<boost::multiprecision::cpp_int_backend<32, 32, boost::multiprecision::signed_magnitude, boost::multiprecision::checked, void> > >();
   test_round_trip<boost::multiprecision::number<boost::multiprecision::cpp_int_backend<32, 32, boost::multiprecision::unsigned_magnitude, boost::multiprecision::checked, void> > >();
#endif
   return boost::report_errors();
}

