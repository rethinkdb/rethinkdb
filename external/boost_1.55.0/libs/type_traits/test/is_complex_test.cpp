
//  (C) Copyright John Maddock 2007. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "test.hpp"
#include "check_integral_constant.hpp"
#ifdef TEST_STD
#  include <type_traits>
#else
#  include <boost/type_traits/is_complex.hpp>
#endif
#include <iostream>

struct bad_struct
{
   operator std::complex<double> ()const;
};

struct derived_complex : public std::complex<double>
{
};

TT_TEST_BEGIN(is_complex)

   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_complex<int>::value, false);
   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_complex<double>::value, false);
   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_complex<float>::value, false);
   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_complex<bad_struct>::value, false);
   //BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_complex<derived_complex>::value, false);
   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_complex<std::complex<long double> >::value, true);
   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_complex<std::complex<double> >::value, true);
   BOOST_CHECK_INTEGRAL_CONSTANT(::tt::is_complex<std::complex<float> >::value, true);

TT_TEST_END









