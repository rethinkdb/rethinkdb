//  stopclock_perf.cpp  ---------------------------------------------------//

//  Copyright 2009 Vicente J. Botet Escriba
//  Copyright 2009 Howard Hinnant

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

//  See http://www.boost.org/libs/chrono for documentation.

#ifndef BOOST_CHRONO_CLOCK_NAME_HPP
#define BOOST_CHRONO_CLOCK_NAME_HPP

#include <boost/chrono/chrono.hpp>
#include <boost/type_traits/is_same.hpp>

template <typename Clock,
          bool = boost::is_same<Clock, boost::chrono::system_clock>::value,
#ifdef BOOST_CHRONO_HAS_CLOCK_STEADY
    bool = boost::is_same<Clock, boost::chrono::steady_clock>::value,
#else
    bool = false,
#endif
          bool = boost::is_same<Clock, boost::chrono::high_resolution_clock>::value
         >
struct name;

template <typename Clock>
struct name<Clock, false, false, false>  {
    static const char* apply() { return "unknown clock";}
};

template <typename Clock>
struct name<Clock, true, false, false>  {
    static const char* apply() { return "system_clock";}
};

template <typename Clock>
struct name<Clock, false, true, false>  {
    static const char* apply() { return "steady_clock";}
};

template <typename Clock>
struct name<Clock, false, false, true>  {
    static const char* apply() { return "high_resolution_clock";}
};

template <typename Clock>
struct name<Clock, false, true, true>  {
    static const char* apply() { return "steady_clock and high_resolution_clock";}
};

template <typename Clock>
struct name<Clock, true, false, true>  {
    static const char* apply() { return "system_clock and high_resolution_clock";}
};

template <typename Clock>
struct name<Clock, true, true, false>  {
    static const char* apply() { return "system_clock and steady_clock";}
};

template <typename Clock>
struct name<Clock, true, true, true>  {
    static const char* apply() { return "system_clock, steady_clock and high_resolution_clock";}
};

#endif
