//  manipulate_clock_object.cpp  ----------------------------------------------------------//

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

#if defined _MSC_VER
#pragma warning(push)
#pragma warning(disable:4100)
#endif
void manipulate_clock_object(system_clock clock)
#if defined _MSC_VER
#pragma warning(pop)
#endif
{
    system_clock::duration delay = milliseconds(5);
    system_clock::time_point start = clock.now();

    while ((clock.now() - start) <= delay) {}

    system_clock::time_point stop = clock.now();
    system_clock::duration elapsed = stop - start;
    std::cout << "paused " << nanoseconds(elapsed).count() << " nanoseconds\n";
}


int main()
{
    manipulate_clock_object(system_clock());
    return 0;
}

