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


#include <boost/chrono/duration.hpp>
#include <boost/type_traits.hpp>
#include <limits>

#if !defined(BOOST_NO_CXX11_STATIC_ASSERT)
#define NOTHING ""
#endif

template <typename D, int ExpectedDigits, typename ExpectedPeriod>
void check_duration()
{
    typedef typename D::rep Rep;
    typedef typename D::period Period;
    BOOST_CHRONO_STATIC_ASSERT(boost::is_signed<Rep>::value, NOTHING, ());
    BOOST_CHRONO_STATIC_ASSERT(boost::is_integral<Rep>::value, NOTHING, ());
    BOOST_CHRONO_STATIC_ASSERT(std::numeric_limits<Rep>::digits >= ExpectedDigits, NOTHING, ());
    BOOST_CHRONO_STATIC_ASSERT((boost::is_same<Period, ExpectedPeriod >::value), NOTHING, ());
}

void test()
{
    check_duration<boost::chrono::hours, 22, boost::ratio<3600> >();
    check_duration<boost::chrono::minutes, 28, boost::ratio<60> >();
    check_duration<boost::chrono::seconds, 34, boost::ratio<1> >();
    check_duration<boost::chrono::milliseconds, 44, boost::milli >();
    check_duration<boost::chrono::microseconds, 54, boost::micro >();
    check_duration<boost::chrono::nanoseconds, 63, boost::nano >();
}
