//  min_time_point.cpp  ----------------------------------------------------------//

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

#include <boost/chrono/typeof/boost/chrono/chrono.hpp>
#include <boost/type_traits.hpp>

#include <iostream>

using namespace boost::chrono;

template <class Rep, class Period>
void print_duration(std::ostream& os, duration<Rep, Period> d)
{
    os << d.count() << " * " << Period::num << '/' << Period::den << " seconds\n";
}

namespace my_ns {
// Example min utility:  returns the earliest time_point
//   Being able to *easily* write this function is a major feature!
template <class Clock, class Duration1, class Duration2>
inline
typename boost::common_type<time_point<Clock, Duration1>,
                     time_point<Clock, Duration2> >::type
min BOOST_PREVENT_MACRO_SUBSTITUTION  (time_point<Clock, Duration1> t1, time_point<Clock, Duration2> t2)
{
    return t2 < t1 ? t2 : t1;
}
}
void test_min()
{
#if 1
    typedef time_point<system_clock,
      boost::common_type<system_clock::duration, seconds>::type> T1;
    typedef time_point<system_clock,
      boost::common_type<system_clock::duration, nanoseconds>::type> T2;
    typedef boost::common_type<T1, T2>::type T3;
    /*auto*/ T1 t1 = system_clock::now() + seconds(3);
    /*auto*/ T2 t2 = system_clock::now() + nanoseconds(3);
    /*auto*/ T3 t3 = (my_ns::min)(t1, t2);
#else
    BOOST_AUTO(t1, system_clock::now() + seconds(3));
    BOOST_AUTO(t2, system_clock::now() + nanoseconds(3));
    BOOST_AUTO(t3, (min)(t1, t2));
#endif
    print_duration(std::cout, t1 - t3);
    print_duration(std::cout, t2 - t3);
}

int main()
{
    test_min();
    return 0;
}

