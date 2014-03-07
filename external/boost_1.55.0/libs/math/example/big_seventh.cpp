
// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Copyright Paul A. Bristow 2012.
// Copyright Christopher Kormanyos 2012.

// This file is written to be included from a Quickbook .qbk document.
// It can be compiled by the C++ compiler, and run. Any output can
// also be added here as comment or included or pasted in elsewhere.
// Caution: this file contains Quickbook markup as well as code
// and comments: don't change any of the special comment markups!

#ifdef _MSC_VER
#  pragma warning (disable : 4512) // assignment operator could not be generated.
#  pragma warning (disable : 4996)
#endif

//[big_seventh_example_1

/*`[h5 Using Boost.Multiprecision `cpp_float` for numerical calculations with high precision.]

The Boost.Multiprecision library can be used for computations requiring precision
exceeding that of standard built-in types such as float, double
and long double. For extended-precision calculations, Boost.Multiprecision
supplies a template data type called cpp_dec_float. The number of decimal
digits of precision is fixed at compile-time via template parameter.

To use these floating-point types and constants, we need some includes:

*/

#include <boost/math/constants/constants.hpp>

#include <boost/multiprecision/cpp_dec_float.hpp>
// using boost::multiprecision::cpp_dec_float

#include <iostream>
#include <limits>

//` So now we can demonstrate with some trivial calculations:

int main()
{
/*`Using `typedef cpp_dec_float_50` hides the complexity of multiprecision to allow us
  to define variables with 50 decimal digit precision just like built-in `double`.
*/
  using boost::multiprecision::cpp_dec_float_50;

  cpp_dec_float_50 seventh = cpp_dec_float_50(1) / 7;

  /*`By default, output would only show the standard 6 decimal digits,
     so set precision to show all 50 significant digits.
  */
  std::cout.precision(std::numeric_limits<cpp_dec_float_50>::digits10);
  std::cout << seventh << std::endl;
/*`which outputs:

  0.14285714285714285714285714285714285714285714285714

We can also use constants, guaranteed to be initialized with the very last bit of precision.
*/

  cpp_dec_float_50 circumference = boost::math::constants::pi<cpp_dec_float_50>() * 2 * seventh;

  std::cout << circumference << std::endl;

/*`which outputs

    0.89759790102565521098932668093700082405633411410717
*/
//]  [/big_seventh_example_1]

    return 0;
} // int main()


/*
//[big_seventh_example_output

  0.14285714285714285714285714285714285714285714285714
  0.89759790102565521098932668093700082405633411410717

//]

*/

