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
#include <boost/type_traits.hpp>
#include <limits>
#include <boost/detail/lightweight_test.hpp>

#include "../rep.h"

int main()
{
    BOOST_TEST((boost::chrono::duration_values<int>::min)() ==
           (std::numeric_limits<int>::min)());
    BOOST_TEST((boost::chrono::duration_values<double>::min)() ==
           -(std::numeric_limits<double>::max)());
    BOOST_TEST((boost::chrono::duration_values<Rep>::min)() ==
           (std::numeric_limits<Rep>::min)());

    BOOST_TEST((boost::chrono::duration_values<int>::max)() ==
           (std::numeric_limits<int>::max)());
    BOOST_TEST((boost::chrono::duration_values<double>::max)() ==
           (std::numeric_limits<double>::max)());
    BOOST_TEST((boost::chrono::duration_values<Rep>::max)() ==
           (std::numeric_limits<Rep>::max)());

    BOOST_TEST(boost::chrono::duration_values<int>::zero() == 0);
    BOOST_TEST(boost::chrono::duration_values<Rep>::zero() == 0);

    return boost::report_errors();
}
