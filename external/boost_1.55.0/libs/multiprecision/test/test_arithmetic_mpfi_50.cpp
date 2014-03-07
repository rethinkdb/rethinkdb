///////////////////////////////////////////////////////////////
//  Copyright 2012 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_

#ifdef _MSC_VER
#  define _SCL_SECURE_NO_WARNINGS
#endif

#include <boost/multiprecision/mpfi.hpp>
#define TEST_MPFR
#include "test_arithmetic.hpp"

template <unsigned D>
struct related_type<boost::multiprecision::number< boost::multiprecision::mpfi_float_backend<D> > >
{
   typedef boost::multiprecision::number< boost::multiprecision::mpfi_float_backend<D/2> > type;
};

int main()
{
   test<boost::multiprecision::mpfi_float_50>();
   return boost::report_errors();
}

