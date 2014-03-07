//  (C) Copyright John Maddock 2005.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifdef TEST_STD_HEADERS
#include <complex>
#else
#include <boost/tr1/complex.hpp>
#endif

#include <boost/test/test_tools.hpp>
#include <boost/test/included/test_exec_monitor.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/is_floating_point.hpp>
#include <boost/mpl/if.hpp>
#include <boost/static_assert.hpp>

#include <iostream>
#include <iomanip>

#ifndef VERBOSE
#undef BOOST_MESSAGE
#define BOOST_MESSAGE(x)
#endif

//
// This test verifies that the complex-algorithms that are
// overloaded for scalar types produce the same result as casting
// the argument to a complex type, and calling the complex version
// of the algorithm.  Relative errors must be within 2e in order for 
// the tests to pass.
//

template <class T, class U>
void do_check(const T& t, const U& u)
{
   static const T two = 2;
   static const T factor = std::pow(two, 1-std::numeric_limits<T>::digits) * 200;
   BOOST_STATIC_ASSERT((::boost::is_same<T,U>::value));
   BOOST_CHECK_CLOSE(t, u, factor);
}

template <class T, class U>
void do_check(const std::complex<T>& t, const std::complex<U>& u)
{
   BOOST_STATIC_ASSERT((::boost::is_same<T,U>::value));
   do_check(t.real(), u.real());
   do_check(t.imag(), u.imag());
}

template <class T>
void check_val(const T& val)
{
   typedef typename boost::mpl::if_< boost::is_floating_point<T>, T, double>::type real_type;
   typedef std::complex<real_type> complex_type;

   real_type rval = static_cast<real_type>(val);
   complex_type cval = rval;

   if(val)
   {
      std::cout << "    Testing std::arg.\n";
      do_check(std::arg(cval), std::arg(rval));
      do_check(std::arg(cval), std::arg(val));
   }
   std::cout << "    Testing std::norm.\n";
   do_check(std::norm(cval), std::norm(rval));
   do_check(std::norm(cval), std::norm(val));
   std::cout << "    Testing std::conj.\n";
   do_check(std::conj(cval), std::conj(rval));
   do_check(std::conj(cval), std::conj(val));
   std::cout << "    Testing std::polar.\n";
   do_check(std::polar(val), std::polar(rval));
   do_check(std::polar(val, 0), std::polar(rval, 0));
   do_check(std::polar(val, val), std::polar(rval, rval));
   do_check(std::polar(val, rval), std::polar(rval, val));
   std::cout << "    Testing std::real.\n";
   do_check(std::real(cval), std::real(rval));
   do_check(std::real(cval), std::real(val));
   std::cout << "    Testing std::imaj.\n";
   do_check(std::imag(cval), std::imag(rval));
   do_check(std::imag(cval), std::imag(val));
   if(val && !boost::is_floating_point<T>::value)
   {
      //
      // Note that these tests are not run for floating point
      // types as that would only test the std lib vendor's
      // implementation of pow, not our additional overloads.
      // Note that some std lib's do fail these tests, gcc on
      // Darwin is a particularly bad example !
      //
      std::cout << "    Testing std::pow.\n";
      do_check(std::pow(cval, cval), std::pow(cval, val));
      do_check(std::pow(cval, cval), std::pow(cval, rval));
      do_check(std::pow(cval, cval), std::pow(val, cval));
      do_check(std::pow(cval, cval), std::pow(rval, cval));
   }
}

void do_check(double i)
{
   std::cout << "Checking type double with value " << i << std::endl;
   check_val(i);
   std::cout << "Checking type float with value " << i << std::endl;
   check_val(static_cast<float>(i));
   std::cout << "Checking type long double with value " << i << std::endl;
   check_val(static_cast<long double>(i));
}

void do_check(int i)
{
   std::cout << "Checking type char with value " << i << std::endl;
   check_val(static_cast<char>(i));
   std::cout << "Checking type unsigned char with value " << i << std::endl;
   check_val(static_cast<unsigned char>(i));
   std::cout << "Checking type signed char with value " << i << std::endl;
   check_val(static_cast<signed char>(i));
   std::cout << "Checking type short with value " << i << std::endl;
   check_val(static_cast<short>(i));
   std::cout << "Checking type unsigned short with value " << i << std::endl;
   check_val(static_cast<unsigned short>(i));
   std::cout << "Checking type int with value " << i << std::endl;
   check_val(static_cast<int>(i));
   std::cout << "Checking type unsigned int with value " << i << std::endl;
   check_val(static_cast<unsigned int>(i));
   std::cout << "Checking type long with value " << i << std::endl;
   check_val(static_cast<long>(i));
   std::cout << "Checking type unsigned long with value " << i << std::endl;
   check_val(static_cast<unsigned long>(i));
#ifdef BOOST_HAS_LONG_LONG
   std::cout << "Checking type long long with value " << i << std::endl;
   check_val(static_cast<long long>(i));
   std::cout << "Checking type unsigned long long with value " << i << std::endl;
   check_val(static_cast<unsigned long long>(i));
#elif defined(BOOST_HAS_MS_INT64)
   std::cout << "Checking type __int64 with value " << i << std::endl;
   check_val(static_cast<__int64>(i));
   std::cout << "Checking type unsigned __int64 with value " << i << std::endl;
   check_val(static_cast<unsigned __int64>(i));
#endif
   do_check(static_cast<double>(i));
}

int test_main(int, char*[])
{
   do_check(0);
   do_check(0.0);
   do_check(1);
   do_check(1.5);
   do_check(0.5);
   return 0;
}

