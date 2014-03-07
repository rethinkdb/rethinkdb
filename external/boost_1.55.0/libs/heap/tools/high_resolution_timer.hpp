/*=============================================================================
    Copyright (c) 2005-2007 Hartmut Kaiser
                  2007, 2009 Tim Blechmann

    Distributed under the Boost Software License, Version 1.0.
    (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_HIGH_RESOLUTION_TIMER_HPP)
#define BOOST_HIGH_RESOLUTION_TIMER_HPP

#include <boost/config.hpp>
#include <boost/throw_exception.hpp>

#if _POSIX_C_SOURCE >= 199309L

#include "time.h"

#include <stdexcept>
#include <limits>

namespace boost {

class high_resolution_timer
{
public:
    high_resolution_timer()
    {
        restart();
    }

    void restart()
    {
        int status = clock_gettime(CLOCK_REALTIME, &start_time);

        if (status == -1)
            boost::throw_exception(std::runtime_error("Couldn't initialize start_time"));
    }

    double elapsed() const                  // return elapsed time in seconds
    {
        struct timespec now;

        int status = clock_gettime(CLOCK_REALTIME, &now);

        if (status == -1)
            boost::throw_exception(std::runtime_error("Couldn't get current time"));

        struct timespec diff;

        double ret_sec = double(now.tv_sec - start_time.tv_sec);
        double ret_nsec = double(now.tv_nsec - start_time.tv_nsec);

        while (ret_nsec < 0)
        {
            ret_sec -= 1.0;
            ret_nsec += 1e9;
        }

        double ret = ret_sec + ret_nsec / 1e9;

        return ret;
    }

    double elapsed_max() const   // return estimated maximum value for elapsed()
    {
        return double((std::numeric_limits<double>::max)());
    }

    double elapsed_min() const            // return minimum value for elapsed()
    {
        return 0.0;
    }

private:
    struct timespec start_time;
};

} // namespace boost

#elif defined(__APPLE__)

#import <mach/mach_time.h>


namespace boost {

class high_resolution_timer
{
public:
    high_resolution_timer(void)
    {
        mach_timebase_info_data_t info;

        kern_return_t err = mach_timebase_info(&info);
        if (err)
            throw std::runtime_error("cannot create mach timebase info");

        conversion_factor = (double)info.numer/(double)info.denom;
        restart();
    }

    void restart()
    {
        start = mach_absolute_time();
    }

    double elapsed() const                  // return elapsed time in seconds
    {
        uint64_t now = mach_absolute_time();
        double duration = double(now - start) * conversion_factor;

        return duration
    }

    double elapsed_max() const   // return estimated maximum value for elapsed()
    {
        return double((std::numeric_limits<double>::max)());
    }

    double elapsed_min() const            // return minimum value for elapsed()
    {
        return 0.0;
    }

private:
    uint64_t start;
    double conversion_factor;
};

} // namespace boost

#elif defined(BOOST_WINDOWS)

#include <stdexcept>
#include <limits>
#include <windows.h>

namespace boost {

///////////////////////////////////////////////////////////////////////////////
//
//  high_resolution_timer
//      A timer object measures elapsed time.
//      CAUTION: Windows only!
//
///////////////////////////////////////////////////////////////////////////////
class high_resolution_timer
{
public:
    high_resolution_timer()
    {
        start_time.QuadPart = 0;
        frequency.QuadPart = 0;

        if (!QueryPerformanceFrequency(&frequency))
            boost::throw_exception(std::runtime_error("Couldn't acquire frequency"));

        restart();
    }

    void restart()
    {
        if (!QueryPerformanceCounter(&start_time))
            boost::throw_exception(std::runtime_error("Couldn't initialize start_time"));
    }

    double elapsed() const                  // return elapsed time in seconds
    {
        LARGE_INTEGER now;
        if (!QueryPerformanceCounter(&now))
            boost::throw_exception(std::runtime_error("Couldn't get current time"));

        return double(now.QuadPart - start_time.QuadPart) / frequency.QuadPart;
    }

    double elapsed_max() const   // return estimated maximum value for elapsed()
    {
        return (double((std::numeric_limits<LONGLONG>::max)())
            - double(start_time.QuadPart)) / double(frequency.QuadPart);
    }

    double elapsed_min() const            // return minimum value for elapsed()
    {
        return 1.0 / frequency.QuadPart;
    }

private:
    LARGE_INTEGER start_time;
    LARGE_INTEGER frequency;
};

} // namespace boost

#else

//  For other platforms, simply fall back to boost::timer
#include <boost/timer.hpp>
#include <boost/throw_exception.hpp>

namespace boost {
    typedef boost::timer high_resolution_timer;
}

#endif

#endif  // !defined(BOOST_HIGH_RESOLUTION_TIMER_HPP)

