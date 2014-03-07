//  test_system_clock.cpp  ----------------------------------------------------------//

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

#include "clock_name.hpp"

#if defined(BOOST_NO_CXX11_CONSTEXPR)

using namespace boost::chrono;

template <typename Clock>
void test_clock()
{
    std::cout << "\n"<< name<Clock>::apply() << " test" << std::endl;
{
    typename Clock::duration delay = milliseconds(5);
    typename Clock::time_point start = Clock::now();
    while (Clock::now() - start <= delay)
        ;
    typename Clock::time_point stop = Clock::now();
    //typename Clock::duration elapsed = stop - start;
    std::cout << "5 milliseconds paused " << nanoseconds(stop - start).count() << " nanoseconds\n";
}
{
    typename Clock::time_point start = Clock::now();
    typename Clock::time_point stop;
    std::size_t counter=1;
    while ((stop=Clock::now()) == start) {
        ++counter;
    }
    //typename Clock::duration elapsed = stop - start;
    std::cout << "After " << counter << " trials, elapsed time " << nanoseconds(stop - start).count() << " nanoseconds\n";

    start = Clock::now();
    for (std::size_t c=counter; c>0; --c) {
        stop=Clock::now();;
    }
    std::cout << "After " << counter << " trials, elapsed time " << nanoseconds(stop - start).count() << " nanoseconds\n";


}
{
    typename Clock::time_point start = Clock::now();
    typename Clock::time_point stop = Clock::now();
    std::cout << "Resolution estimate: " << nanoseconds(stop-start).count() << " nanoseconds\n";
}
}

void test_system_clock()
{
    std::cout << "system_clock test" << std::endl;
    system_clock::duration delay = milliseconds(5);
    system_clock::time_point start = system_clock::now();
    while (system_clock::now() - start <= delay)
        ;
    system_clock::time_point stop = system_clock::now();
    system_clock::duration elapsed = stop - start;
    std::cout << "paused " << nanoseconds(elapsed).count() << " nanoseconds\n";
    start = system_clock::now();
    stop = system_clock::now();
    std::cout << "system_clock resolution estimate: " << nanoseconds(stop-start).count() << " nanoseconds\n";
}

void test_steady_clock()
{
#ifdef BOOST_CHRONO_HAS_CLOCK_STEADY
    std::cout << "steady_clock test" << std::endl;
    steady_clock::duration delay = milliseconds(5);
    steady_clock::time_point start = steady_clock::now();
    while (steady_clock::now() - start <= delay)
        ;
    steady_clock::time_point stop = steady_clock::now();
    steady_clock::duration elapsed = stop - start;
    std::cout << "paused " << nanoseconds(elapsed).count() << " nanoseconds\n";
    start = steady_clock::now();
    stop = steady_clock::now();
    std::cout << "steady_clock resolution estimate: " << nanoseconds(stop-start).count() << " nanoseconds\n";
#endif
}
void test_hi_resolution_clock()
{
    std::cout << "high_resolution_clock test" << std::endl;
    high_resolution_clock::duration delay = milliseconds(5);
    high_resolution_clock::time_point start = high_resolution_clock::now();
    while (high_resolution_clock::now() - start <= delay)
      ;
    high_resolution_clock::time_point stop = high_resolution_clock::now();
    high_resolution_clock::duration elapsed = stop - start;
    std::cout << "paused " << nanoseconds(elapsed).count() << " nanoseconds\n";
    start = high_resolution_clock::now();
    stop = high_resolution_clock::now();
    std::cout << "high_resolution_clock resolution estimate: " << nanoseconds(stop-start).count() << " nanoseconds\n";
}

//void test_mixed_clock()
//{
//    std::cout << "mixed clock test" << std::endl;
//    high_resolution_clock::time_point hstart = high_resolution_clock::now();
//    std::cout << "Add 5 milliseconds to a high_resolution_clock::time_point\n";
//    steady_clock::time_point mend = hstart + milliseconds(5);
//    bool b = hstart == mend;
//    system_clock::time_point sstart = system_clock::now();
//    std::cout << "Subtracting system_clock::time_point from steady_clock::time_point doesn't compile\n";
////  mend - sstart; // doesn't compile
//    std::cout << "subtract high_resolution_clock::time_point from steady_clock::time_point"
//            " and add that to a system_clock::time_point\n";
//    system_clock::time_point send = sstart + duration_cast<system_clock::duration>(mend - hstart);
//    std::cout << "subtract two system_clock::time_point's and output that in microseconds:\n";
//    microseconds ms = send - sstart;
//    std::cout << ms.count() << " microseconds\n";
//}
//
//void test_c_mapping()
//{
//    std::cout << "C map test\n";
//    using namespace boost::chrono;
//    system_clock::time_point t1 = system_clock::now();
//    std::time_t c_time = system_clock::to_time_t(t1);
//    std::tm* tmptr = std::localtime(&c_time);
//    std::cout << "It is now " << tmptr->tm_hour << ':' << tmptr->tm_min << ':' << tmptr->tm_sec << ' '
//              << tmptr->tm_year + 1900 << '-' << tmptr->tm_mon + 1 << '-' << tmptr->tm_mday << '\n';
//    c_time = std::mktime(tmptr);
//    system_clock::time_point t2 = system_clock::from_time_t(c_time);
//    microseconds ms = t1 - t2;
//    std::cout << "Round-tripping through the C interface truncated the precision by " << ms.count() << " microseconds\n";
//}


int main()
{
    test_system_clock();
    test_steady_clock();
    test_hi_resolution_clock();
    //test_mixed_clock();
    test_clock<system_clock>();
#ifdef BOOST_CHRONO_HAS_CLOCK_STEADY
    test_clock<steady_clock>();
#endif
    test_clock<high_resolution_clock>();



    return 0;
}

#else
int main()
{


    return 0;
}
#endif
