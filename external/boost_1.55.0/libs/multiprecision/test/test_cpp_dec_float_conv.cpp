///////////////////////////////////////////////////////////////
//  Copyright 2012 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_
//

#ifdef _MSC_VER
#  define _SCL_SECURE_NO_WARNINGS
#endif

#include <boost/detail/lightweight_test.hpp>
#include <boost/array.hpp>
#include "test.hpp"

#include <boost/multiprecision/cpp_dec_float.hpp>

int main()
{
   using namespace boost::multiprecision;
   //
   // Test interconversions between different precisions:
   //
   cpp_dec_float_50    f1(2);
   cpp_dec_float_100   f2(3);

   cpp_dec_float_100   f3 = f1;  // implicit conversion OK
   BOOST_TEST(f3 == 2);
   cpp_dec_float_50    f4(f2);   // explicit conversion OK
   BOOST_TEST(f4 == 3);

   f2 = f1;
   BOOST_TEST(f2 == 2);
   f2 = 4;
   f1 = static_cast<cpp_dec_float_50>(f2);
   BOOST_TEST(f1 == 4);


   return boost::report_errors();
}



