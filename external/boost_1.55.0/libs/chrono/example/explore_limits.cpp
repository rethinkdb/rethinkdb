//  explore_limits.cpp  ----------------------------------------------------------//

//  Copyright 2008 Howard Hinnant
//  Copyright 2008 Beman Dawes
//  Copyright 2009 Vicente J. Botet Escriba

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

/*
This code was extracted by Vicente J. Botet Escriba from Beman Dawes time2_demo.cpp which
was derived by Beman Dawes from Howard Hinnant's time2_demo prototype.
Many thanks to Howard for making his code available under the Boost license.
The original code was modified to conform to Boost conventions and to section
20.9 Time utilities [time] of the C++ committee's working paper N2798.
See http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2008/n2798.pdf.

time2_demo contained this comment:

    Much thanks to Andrei Alexandrescu,
                   Walter Brown,
                   Peter Dimov,
                   Jeff Garland,
                   Terry Golubiewski,
                   Daniel Krugler,
                   Anthony Williams.
*/

#include <boost/chrono/chrono.hpp>
#include <boost/type_traits.hpp>

#include <iostream>

using namespace boost::chrono;


void explore_limits()
{
    typedef duration<long long, boost::ratio_multiply<boost::ratio<24*3652425,10000>,
      hours::period>::type> Years;
#ifdef BOOST_CHRONO_HAS_CLOCK_STEADY
    steady_clock::time_point t1( Years(250));
    steady_clock::time_point t2(-Years(250));
#else
    system_clock::time_point t1( Years(250));
    system_clock::time_point t2(-Years(250));
#endif
    // nanosecond resolution is likely to overflow.  "up cast" to microseconds.
    //   The "up cast" trades precision for range.
    microseconds d = time_point_cast<microseconds>(t1) - time_point_cast<microseconds>(t2);
    std::cout << d.count() << " microseconds\n";
}


int main()
{
    explore_limits();
    return 0;
}

