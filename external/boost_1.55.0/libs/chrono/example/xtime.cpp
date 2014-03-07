//  xtime.cpp  ----------------------------------------------------------//

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

// Example round_up utility:  converts d to To, rounding up for inexact conversions
//   Being able to *easily* write this function is a major feature!
template <class To, class Rep, class Period>
To
round_up(duration<Rep, Period> d)
{
    To result = duration_cast<To>(d);
    if (result < d)
        ++result;
    return result;
}

// demonstrate interaction with xtime-like facility:

struct xtime
{
    long sec;
    unsigned long usec;
};

template <class Rep, class Period>
xtime
to_xtime_truncate(duration<Rep, Period> d)
{
    xtime xt;
    xt.sec = static_cast<long>(duration_cast<seconds>(d).count());
    xt.usec = static_cast<long>(duration_cast<microseconds>(d - seconds(xt.sec)).count());
    return xt;
}

template <class Rep, class Period>
xtime
to_xtime_round_up(duration<Rep, Period> d)
{
    xtime xt;
    xt.sec = static_cast<long>(duration_cast<seconds>(d).count());
    xt.usec = static_cast<unsigned long>(round_up<microseconds>(d - seconds(xt.sec)).count());
    return xt;
}

microseconds
from_xtime(xtime xt)
{
    return seconds(xt.sec) + microseconds(xt.usec);
}

void print(xtime xt)
{
    std::cout << '{' << xt.sec << ',' << xt.usec << "}\n";
}

void test_with_xtime()
{
    std::cout << "test_with_xtime\n";
    xtime xt = to_xtime_truncate(seconds(3) + milliseconds(251));
    print(xt);
    milliseconds ms = duration_cast<milliseconds>(from_xtime(xt));
    std::cout << ms.count() << " milliseconds\n";
    xt = to_xtime_round_up(ms);
    print(xt);
    xt = to_xtime_truncate(seconds(3) + nanoseconds(999));
    print(xt);
    xt = to_xtime_round_up(seconds(3) + nanoseconds(999));
    print(xt);
}


int main()
{
    test_with_xtime();
    return 0;
}

