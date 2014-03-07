//   Copyright (c) 2013 Alex Korobka
// 
//   Distributed under the Boost Software License, Version 1.0. (See accompanying
//   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This test checks whether #8970 was fixed

#include <boost/config/warning_disable.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <iostream>
#include <boost/spirit/include/karma.hpp>

#include "test.hpp"

using namespace spirit_test;
using namespace boost::spirit;

template <typename Num>
struct signed_policy : karma::real_policies<Num>
{
    static bool force_sign(Num n) { return true; }
};

int main()
{
    karma::real_generator<double, signed_policy<double> > const force_sign_double =
        karma::real_generator<double, signed_policy<double> >();

    BOOST_TEST(test("-0.123", force_sign_double, -0.123));
    BOOST_TEST(test("-1.123", force_sign_double, -1.123));
    BOOST_TEST(test("0.0", force_sign_double, 0));
    BOOST_TEST(test("+0.123", force_sign_double, 0.123));
    BOOST_TEST(test("+1.123", force_sign_double, 1.123));

    using karma::double_;
    BOOST_TEST(test("-0.123", double_, -0.123));
    BOOST_TEST(test("-1.123", double_, -1.123));
    BOOST_TEST(test("0.0", double_, 0));
    BOOST_TEST(test("0.123", double_, 0.123));
    BOOST_TEST(test("1.123", double_, 1.123));

    return boost::report_errors();
} 
