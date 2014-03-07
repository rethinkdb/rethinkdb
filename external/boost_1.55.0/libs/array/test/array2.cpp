/* example for using class array<>
 * (C) Copyright Nicolai M. Josuttis 2001.
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef _SCL_SECURE_NO_WARNINGS
// Suppress warnings from the std lib:
#  define _SCL_SECURE_NO_WARNINGS
#endif

#include <algorithm>
#include <functional>
#include <boost/array.hpp>
#include "print.hpp"
using namespace std;

int main()
{
    // create and initialize array
    boost::array<int,10> a = { { 1, 2, 3, 4, 5 } };

    print_elements(a);

    // modify elements directly
    for (unsigned i=0; i<a.size(); ++i) {
        ++a[i];
    }
    print_elements(a);

    // change order using an STL algorithm
    reverse(a.begin(),a.end());
    print_elements(a);

    // negate elements using STL framework
    transform(a.begin(),a.end(),    // source
              a.begin(),            // destination
              negate<int>());       // operation
    print_elements(a);

    return 0;  // makes Visual-C++ compiler happy
}

