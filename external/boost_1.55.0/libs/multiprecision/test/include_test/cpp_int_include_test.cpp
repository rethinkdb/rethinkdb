///////////////////////////////////////////////////////////////
//  Copyright 2012 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_

#include <boost/multiprecision/cpp_int.hpp>

using namespace boost::multiprecision;

number<cpp_int_backend<> > a;
number<cpp_int_backend<>, et_off> b;
number<cpp_int_backend<64, 64, signed_magnitude, checked, void>, et_off> c;
number<cpp_int_backend<128, 128, signed_magnitude, checked, void>, et_off> d;
number<cpp_int_backend<500, 500, signed_magnitude, checked, void>, et_off> e;


