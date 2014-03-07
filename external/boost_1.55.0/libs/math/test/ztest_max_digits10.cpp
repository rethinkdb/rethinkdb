
 // Copyright 2010 Paul A. Bristow
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/* Temporary test program to discover which platforms support

numeric_limits

digits10 and

new max_digits10.

This is needed to produce or select a macro to avoid compilation failure in Boost.Test
for platforms that do not include either or both of these.

BOOST_NO_NUMERIC_LIMITS_LOWEST is suitable but deprecated.


[Boost C++ Libraries]

#5758: Boost.Test Floating-point comparison diagnostic output does not support radix 10.

*/

#include <boost/config.hpp>
#include <boost/version.hpp>

#include <iostream>
#include <limits>

int main()
{
   std::cout  << "Platform: " << BOOST_PLATFORM            << '\n'
        << "Compiler: " << BOOST_COMPILER            << '\n'
        << "STL     : " << BOOST_STDLIB              << '\n'
        << "Boost   : " << BOOST_VERSION/100000      << "."
                        << BOOST_VERSION/100 % 1000  << "."
                        << BOOST_VERSION % 100       << std::endl;

   int digits10 = std::numeric_limits<double>::digits10;
   int max_digits10 = std::numeric_limits<double>::max_digits10;

   std::cout << "std::numeric_limits<double>::digits10 = " << digits10 << std::endl;

   std::cout << "std::numeric_limits<double>::max_digits10 = " << max_digits10 << std::endl;


} // int main()

/*

Output:

  Description: Autorun "J:\Cpp\MathToolkit\test\Math_test\Debug\ztest_max_digits10.exe"
  Platform: Win32
  Compiler: Microsoft Visual C++ version 10.0
  STL     : Dinkumware standard library version 520
  Boost   : 1.50.0
  std::numeric_limits<double>::digits10 = 15
  std::numeric_limits<double>::max_digits10 = 17



*/
