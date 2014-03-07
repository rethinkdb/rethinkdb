//  rational number example program  ----------------------------------------//

//  (C) Copyright Paul Moore 1999. Permission to copy, use, modify, sell
//  and distribute this software is granted provided this copyright notice
//  appears in all copies. This software is provided "as is" without express or
//  implied warranty, and with no claim as to its suitability for any purpose.

//  Revision History
//  14 Dec 99  Initial version

#include <iostream>
#include <cassert>
#include <cstdlib>
#include <boost/config.hpp>
#ifndef BOOST_NO_LIMITS
#include <limits>
#else
#include <limits.h>
#endif
#include <exception>
#include <boost/rational.hpp>

using std::cout;
using std::endl;
using boost::rational;

#ifdef BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP
// This is a nasty hack, required because MSVC does not implement "Koenig
// Lookup". Basically, if I call abs(r), the C++ standard says that the
// compiler should look for a definition of abs in the namespace which
// contains r's class (in this case boost) - among other places.

// Koenig Lookup is a relatively recent feature, and other compilers may not
// implement it yet. If so, try including this line.

using boost::abs;
#endif

int main ()
{
    rational<int> half(1,2);
    rational<int> one(1);
    rational<int> two(2);

    // Some basic checks
    assert(half.numerator() == 1);
    assert(half.denominator() == 2);
    assert(boost::rational_cast<double>(half) == 0.5);

    // Arithmetic
    assert(half + half == one);
    assert(one - half == half);
    assert(two * half == one);
    assert(one / half == two);

    // With conversions to integer
    assert(half+half == 1);
    assert(2 * half == one);
    assert(2 * half == 1);
    assert(one / half == 2);
    assert(1 / half == 2);

    // Sign handling
    rational<int> minus_half(-1,2);
    assert(-half == minus_half);
    assert(abs(minus_half) == half);

    // Do we avoid overflow?
#ifndef BOOST_NO_LIMITS
    int maxint = (std::numeric_limits<int>::max)();
#else
    int maxint = INT_MAX;
#endif
    rational<int> big(maxint, 2);
    assert(2 * big == maxint);

    // Print some of the above results
    cout << half << "+" << half << "=" << one << endl;
    cout << one << "-" << half << "=" << half << endl;
    cout << two << "*" << half << "=" << one << endl;
    cout << one << "/" << half << "=" << two << endl;
    cout << "abs(" << minus_half << ")=" << half << endl;
    cout << "2 * " << big << "=" << maxint
         << " (rational: " << rational<int>(maxint) << ")" << endl;

    // Some extras
    rational<int> pi(22,7);
    cout << "pi = " << boost::rational_cast<double>(pi) << " (nearly)" << endl;

    // Exception handling
    try {
        rational<int> r;        // Forgot to initialise - set to 0
        r = 1/r;                // Boom!
    }
    catch (const boost::bad_rational &e) {
        cout << "Bad rational, as expected: " << e.what() << endl;
    }
    catch (...) {
        cout << "Wrong exception raised!" << endl;
    }

    return 0;
}

