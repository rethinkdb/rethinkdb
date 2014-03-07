//  cycle_count.cpp  ----------------------------------------------------------//

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


template <long long speed>
struct cycle_count
{
    typedef typename boost::ratio_multiply<boost::ratio<speed>, boost::mega>::type frequency;  // Mhz
    typedef typename boost::ratio_divide<boost::ratio<1>, frequency>::type period;
    typedef long long rep;
    typedef boost::chrono::duration<rep, period> duration;
    typedef boost::chrono::time_point<cycle_count> time_point;

    static time_point now()
    {
        static long long tick = 0;
        // return exact cycle count
        return time_point(duration(++tick));  // fake access to clock cycle count
    }
};

template <long long speed>
struct approx_cycle_count
{
    static const long long frequency = speed * 1000000;  // MHz
    typedef nanoseconds duration;
    typedef duration::rep rep;
    typedef duration::period period;
    static const long long nanosec_per_sec = period::den;
    typedef boost::chrono::time_point<approx_cycle_count> time_point;

    static time_point now()
    {
        static long long tick = 0;
        // return cycle count as an approximate number of nanoseconds
        // compute as if nanoseconds is only duration in the std::lib
        return time_point(duration(++tick * nanosec_per_sec / frequency));
    }
};

void cycle_count_delay()
{
    {
    typedef cycle_count<400> clock;
    std::cout << "\nSimulated " << clock::frequency::num / boost::mega::num << "MHz clock which has a tick period of "
         << duration<double, boost::nano>(clock::duration(1)).count() << " nanoseconds\n";
    nanoseconds delayns(500);
    clock::duration delay = duration_cast<clock::duration>(delayns);
    std::cout << "delay = " << delayns.count() << " nanoseconds which is " << delay.count() << " cycles\n";
    clock::time_point start = clock::now();
    clock::time_point stop = start + delay;
    while (clock::now() < stop)  // no multiplies or divides in this loop
        ;
    clock::time_point end = clock::now();
    clock::duration elapsed = end - start;
    std::cout << "paused " << elapsed.count() << " cycles ";
    std::cout << "which is " << duration_cast<nanoseconds>(elapsed).count() << " nanoseconds\n";
    }
    {
    typedef approx_cycle_count<400> clock;
    std::cout << "\nSimulated " << clock::frequency / 1000000 << "MHz clock modeled with nanoseconds\n";
    clock::duration delay = nanoseconds(500);
    std::cout << "delay = " << delay.count() << " nanoseconds\n";
    clock::time_point start = clock::now();
    clock::time_point stop = start + delay;
    while (clock::now() < stop) // 1 multiplication and 1 division in this loop
        ;
    clock::time_point end = clock::now();
    clock::duration elapsed = end - start;
    std::cout << "paused " << elapsed.count() << " nanoseconds\n";
    }
    {
    typedef cycle_count<1500> clock;
    std::cout << "\nSimulated " << clock::frequency::num / boost::mega::num << "MHz clock which has a tick period of "
         << duration<double, boost::nano>(clock::duration(1)).count() << " nanoseconds\n";
    nanoseconds delayns(500);
    clock::duration delay = duration_cast<clock::duration>(delayns);
    std::cout << "delay = " << delayns.count() << " nanoseconds which is " << delay.count() << " cycles\n";
    clock::time_point start = clock::now();
    clock::time_point stop = start + delay;
    while (clock::now() < stop)  // no multiplies or divides in this loop
        ;
    clock::time_point end = clock::now();
    clock::duration elapsed = end - start;
    std::cout << "paused " << elapsed.count() << " cycles ";
    std::cout << "which is " << duration_cast<nanoseconds>(elapsed).count() << " nanoseconds\n";
    }
    {
    typedef approx_cycle_count<1500> clock;
    std::cout << "\nSimulated " << clock::frequency / 1000000 << "MHz clock modeled with nanoseconds\n";
    clock::duration delay = nanoseconds(500);
    std::cout << "delay = " << delay.count() << " nanoseconds\n";
    clock::time_point start = clock::now();
    clock::time_point stop = start + delay;
    while (clock::now() < stop) // 1 multiplication and 1 division in this loop
        ;
    clock::time_point end = clock::now();
    clock::duration elapsed = end - start;
    std::cout << "paused " << elapsed.count() << " nanoseconds\n";
    }
}

int main()
{
    cycle_count_delay();
    return 0;
}

