///////////////////////////////////////////////////////////////
//  Copyright 2012 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_

#ifdef _MSC_VER
#  define _SCL_SECURE_NO_WARNINGS
#endif

#include <boost/multiprecision/gmp.hpp>

#define NO_MIXED_OPS

#include "test_arithmetic.hpp"
#include <boost/rational.hpp>

template <class T>
struct is_boost_rational<boost::rational<T> > : public boost::mpl::true_{};

namespace boost{ namespace multiprecision{

template<>
struct number_category<rational<mpz_int> > : public mpl::int_<number_kind_rational> {};

}}

int main()
{
   test<boost::rational<boost::multiprecision::mpz_int> >();
   return boost::report_errors();
}

