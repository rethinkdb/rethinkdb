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

#include <boost/chrono/chrono.hpp>
#include <boost/detail/lightweight_test.hpp>

#if defined BOOST_NO_CXX11_NUMERIC_LIMITS || defined BOOST_NO_CXX11_CONSTEXPR
#define BOOST_CONSTEXPR_ASSERT(C) BOOST_TEST(C)
#else
#include <boost/static_assert.hpp>
#define BOOST_CONSTEXPR_ASSERT(C) BOOST_STATIC_ASSERT(C)
#endif

int main()
{
    typedef boost::chrono::system_clock Clock;
    typedef boost::chrono::milliseconds Duration;
    typedef boost::chrono::time_point<Clock, Duration> TP;

    BOOST_CONSTEXPR_ASSERT((TP::min)() == TP((Duration::min)()));
    BOOST_CONSTEXPR_ASSERT((TP::max)() == TP((Duration::max)()));

    return boost::report_errors();
}
