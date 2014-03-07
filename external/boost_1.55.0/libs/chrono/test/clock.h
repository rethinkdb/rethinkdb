//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//  Adaptation to Boost of the libcxx
//  Copyright 2010 Vicente J. Botet Escriba
//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

#ifndef CLOCK_H
#define CLOCK_H

#include <boost/chrono/chrono.hpp>

class Clock
{
    typedef boost::chrono::nanoseconds               duration;
    typedef duration::rep                            rep;
    typedef duration::period                         period;
    typedef boost::chrono::time_point<Clock, duration> time_point;
    static const bool is_steady =                false;

    static time_point now();
};

#endif  // CLOCK_H
