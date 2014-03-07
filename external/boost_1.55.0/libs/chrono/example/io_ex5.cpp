//  io_ex1.cpp  ----------------------------------------------------------//

//  Copyright 2010 Howard Hinnant
//  Copyright 2010 Vicente J. Botet Escriba

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

/*
This code was adapted by Vicente J. Botet Escriba from Hinnant's html documentation.
Many thanks to Howard for making his code available under the Boost license.

*/

#include <boost/chrono/chrono_io.hpp>
#include <ostream>
#include <iostream>

// format duration as [-]d/hh::mm::ss.cc
template <class CharT, class Traits, class Rep, class Period>
std::basic_ostream<CharT, Traits>&
display(std::basic_ostream<CharT, Traits>& os,
        boost::chrono::duration<Rep, Period> d)
{
    using std::cout;
    using namespace boost;
    using namespace boost::chrono;

    typedef duration<long long, ratio<86400> > days;
    typedef duration<long long, centi> centiseconds;

    // if negative, print negative sign and negate
    if (d < duration<Rep, Period>(0))
    {
        d = -d;
        os << '-';
    }
    // round d to nearest centiseconds, to even on tie
    centiseconds cs = duration_cast<centiseconds>(d);
    if (d - cs > milliseconds(5)
        || (d - cs == milliseconds(5) && cs.count() & 1))
        ++cs;
    // separate seconds from centiseconds
    seconds s = duration_cast<seconds>(cs);
    cs -= s;
    // separate minutes from seconds
    minutes m = duration_cast<minutes>(s);
    s -= m;
    // separate hours from minutes
    hours h = duration_cast<hours>(m);
    m -= h;
    // separate days from hours
    days dy = duration_cast<days>(h);
    h -= dy;
    // print d/hh:mm:ss.cc
    os << dy.count() << '/';
    if (h < hours(10))
        os << '0';
    os << h.count() << ':';
    if (m < minutes(10))
        os << '0';
    os << m.count() << ':';
    if (s < seconds(10))
        os << '0';
    os << s.count() << '.';
    if (cs < centiseconds(10))
        os << '0';
    os << cs.count();
    return os;
}

int main()
{
    using std::cout;
    using namespace boost;
    using namespace boost::chrono;

#ifdef BOOST_CHRONO_HAS_CLOCK_STEADY
    display(cout, steady_clock::now().time_since_epoch()
                  + duration<long, mega>(1)) << '\n';
#endif
    display(cout, -milliseconds(6)) << '\n';
    display(cout, duration<long, mega>(1)) << '\n';
    display(cout, -duration<long, mega>(1)) << '\n';
}

//~ 12/06:03:22.95
//~ -0/00:00:00.01
//~ 11/13:46:40.00
//~ -11/13:46:40.00
