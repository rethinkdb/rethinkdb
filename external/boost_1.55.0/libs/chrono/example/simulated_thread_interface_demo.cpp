//  simulated_thread_interface_demo.cpp  ----------------------------------------------------------//

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

#define _CRT_SECURE_NO_WARNINGS  // disable VC++ foolishness

#include <boost/chrono/chrono.hpp>
#include <boost/type_traits.hpp>

#include <iostream>
#include <ostream>
#include <stdexcept>
#include <climits>

//////////////////////////////////////////////////////////
///////////// simulated thread interface /////////////////
//////////////////////////////////////////////////////////

namespace boost {

namespace detail {
void print_time(boost::chrono::system_clock::time_point t)
{
    using namespace boost::chrono;
    time_t c_time = system_clock::to_time_t(t);
    std::tm* tmptr = std::localtime(&c_time);
    system_clock::duration d = t.time_since_epoch();
    std::cout << tmptr->tm_hour << ':' << tmptr->tm_min << ':' << tmptr->tm_sec
              << '.' << (d - duration_cast<seconds>(d)).count();
}
}
namespace this_thread {

template <class Rep, class Period>
void sleep_for(const boost::chrono::duration<Rep, Period>& d)
{
    boost::chrono::microseconds t = boost::chrono::duration_cast<boost::chrono::microseconds>(d);
    if (t < d)
        ++t;
    if (t > boost::chrono::microseconds(0))
        std::cout << "sleep_for " << t.count() << " microseconds\n";
}

template <class Clock, class Duration>
void sleep_until(const boost::chrono::time_point<Clock, Duration>& t)
{
    using namespace boost::chrono;
    typedef time_point<Clock, Duration> Time;
    typedef system_clock::time_point SysTime;
    if (t > Clock::now())
    {
        typedef typename boost::common_type<typename Time::duration,
                                     typename SysTime::duration>::type D;
        /* auto */ D d = t - Clock::now();
        microseconds us = duration_cast<microseconds>(d);
        if (us < d)
            ++us;
        SysTime st = system_clock::now() + us;
        std::cout << "sleep_until    ";
        boost::detail::print_time(st);
        std::cout << " which is " << (st - system_clock::now()).count() << " microseconds away\n";
    }
}

}  // this_thread

struct mutex {};

struct timed_mutex
{
    bool try_lock() {std::cout << "timed_mutex::try_lock()\n"; return true;}

    template <class Rep, class Period>
        bool try_lock_for(const boost::chrono::duration<Rep, Period>& d)
        {
            boost::chrono::microseconds t = boost::chrono::duration_cast<boost::chrono::microseconds>(d);
            if (t <= boost::chrono::microseconds(0))
                return try_lock();
            std::cout << "try_lock_for " << t.count() << " microseconds\n";
            return true;
        }

    template <class Clock, class Duration>
    bool try_lock_until(const boost::chrono::time_point<Clock, Duration>& t)
    {
        using namespace boost::chrono;
        typedef time_point<Clock, Duration> Time;
        typedef system_clock::time_point SysTime;
        if (t <= Clock::now())
            return try_lock();
        typedef typename boost::common_type<typename Time::duration,
          typename Clock::duration>::type D;
        /* auto */ D d = t - Clock::now();
        microseconds us = duration_cast<microseconds>(d);
        SysTime st = system_clock::now() + us;
        std::cout << "try_lock_until ";
        boost::detail::print_time(st);
        std::cout << " which is " << (st - system_clock::now()).count()
          << " microseconds away\n";
        return true;
    }
};

struct condition_variable
{
    template <class Rep, class Period>
        bool wait_for(mutex&, const boost::chrono::duration<Rep, Period>& d)
        {
            boost::chrono::microseconds t = boost::chrono::duration_cast<boost::chrono::microseconds>(d);
            std::cout << "wait_for " << t.count() << " microseconds\n";
            return true;
        }

    template <class Clock, class Duration>
    bool wait_until(mutex&, const boost::chrono::time_point<Clock, Duration>& t)
    {
        using namespace boost::chrono;
        typedef time_point<Clock, Duration> Time;
        typedef system_clock::time_point SysTime;
        if (t <= Clock::now())
            return false;
        typedef typename boost::common_type<typename Time::duration,
          typename Clock::duration>::type D;
        /* auto */ D d = t - Clock::now();
        microseconds us = duration_cast<microseconds>(d);
        SysTime st = system_clock::now() + us;
         std::cout << "wait_until     ";
        boost::detail::print_time(st);
        std::cout << " which is " << (st - system_clock::now()).count()
          << " microseconds away\n";
        return true;
    }
};

}

//////////////////////////////////////////////////////////
//////////// Simple sleep and wait examples //////////////
//////////////////////////////////////////////////////////

boost::mutex m;
boost::timed_mutex mut;
boost::condition_variable cv;

void basic_examples()
{
    std::cout << "Running basic examples\n";
    using namespace boost;
    using namespace boost::chrono;
    system_clock::time_point time_limit = system_clock::now() + seconds(4) + milliseconds(500);
    this_thread::sleep_for(seconds(3));
    this_thread::sleep_for(nanoseconds(300));
    this_thread::sleep_until(time_limit);
//    this_thread::sleep_for(time_limit);  // desired compile-time error
//    this_thread::sleep_until(seconds(3)); // desired compile-time error
    mut.try_lock_for(milliseconds(30));
    mut.try_lock_until(time_limit);
//    mut.try_lock_for(time_limit);        // desired compile-time error
//    mut.try_lock_until(milliseconds(30)); // desired compile-time error
    cv.wait_for(m, minutes(1));    // real code would put this in a loop
    cv.wait_until(m, time_limit);  // real code would put this in a loop
    // For those who prefer floating point
    this_thread::sleep_for(duration<double>(0.25));
    this_thread::sleep_until(system_clock::now() + duration<double>(1.5));
}



int main()
{
    basic_examples();
    return 0;
}

