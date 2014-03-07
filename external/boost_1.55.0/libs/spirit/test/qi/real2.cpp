/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2001-2011 Hartmut Kaiser
    Copyright (c) 2011      Bryce Lelbach

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#include "real.hpp"

int
main()
{
    using spirit_test::test;
    using spirit_test::test_attr;
    ///////////////////////////////////////////////////////////////////////////
    //  signed real number tests
    ///////////////////////////////////////////////////////////////////////////
    {
        using boost::spirit::qi::double_;
        using boost::spirit::qi::parse;
        double  d;

        BOOST_TEST(test("-1234", double_));
        BOOST_TEST(test_attr("-1234", double_, d) && compare(d, -1234));

        BOOST_TEST(test("-1.2e3", double_));
        BOOST_TEST(test_attr("-1.2e3", double_, d) && compare(d, -1.2e3));

        BOOST_TEST(test("+1.2e3", double_));
        BOOST_TEST(test_attr("+1.2e3", double_, d) && compare(d, 1.2e3));

        BOOST_TEST(test("-0.1", double_));
        BOOST_TEST(test_attr("-0.1", double_, d) && compare(d, -0.1));

        BOOST_TEST(test("-1.2e-3", double_));
        BOOST_TEST(test_attr("-1.2e-3", double_, d) && compare(d, -1.2e-3));

        BOOST_TEST(test("-1.e2", double_));
        BOOST_TEST(test_attr("-1.e2", double_, d) && compare(d, -1.e2));

        BOOST_TEST(test("-.2e3", double_));
        BOOST_TEST(test_attr("-.2e3", double_, d) && compare(d, -.2e3));

        BOOST_TEST(test("-2e3", double_));
        BOOST_TEST(test_attr("-2e3", double_, d) && compare(d, -2e3));

        BOOST_TEST(!test("-e3", double_));
        BOOST_TEST(!test_attr("-e3", double_, d));

        BOOST_TEST(!test("-1.2e", double_));
        BOOST_TEST(!test_attr("-1.2e", double_, d));

#if defined(BOOST_SPIRIT_TEST_REAL_PRECISION)
        BOOST_TEST(test_attr("-5.7222349715140557e+307", double_, d));
        BOOST_TEST(d == -5.7222349715140557e+307); // exact!

        BOOST_TEST(test_attr("2.0332938517515416e-308", double_, d));
        BOOST_TEST(d == 2.0332938517515416e-308); // exact!

        BOOST_TEST(test_attr("20332938517515416e291", double_, d));
        BOOST_TEST(d == 20332938517515416e291); // exact!

        BOOST_TEST(test_attr("2.0332938517515416e307", double_, d));
        BOOST_TEST(d == 2.0332938517515416e307); // exact!
#endif

        using boost::math::fpclassify;
        using boost::spirit::detail::signbit;   // Boost version is broken

        BOOST_TEST(test("-inf", double_));
        BOOST_TEST(test("-infinity", double_));
        BOOST_TEST(test_attr("-inf", double_, d) &&
            FP_INFINITE == fpclassify(d) && signbit(d));
        BOOST_TEST(test_attr("-infinity", double_, d) &&
            FP_INFINITE == fpclassify(d) && signbit(d));
        BOOST_TEST(test("-INF", double_));
        BOOST_TEST(test("-INFINITY", double_));
        BOOST_TEST(test_attr("-INF", double_, d) &&
            FP_INFINITE == fpclassify(d) && signbit(d));
        BOOST_TEST(test_attr("-INFINITY", double_, d) &&
            FP_INFINITE == fpclassify(d) && signbit(d));

        BOOST_TEST(test("-nan", double_));
        BOOST_TEST(test_attr("-nan", double_, d) &&
            FP_NAN == fpclassify(d) && signbit(d));
        BOOST_TEST(test("-NAN", double_));
        BOOST_TEST(test_attr("-NAN", double_, d) &&
            FP_NAN == fpclassify(d) && signbit(d));

        BOOST_TEST(test("-nan(...)", double_));
        BOOST_TEST(test_attr("-nan(...)", double_, d) &&
            FP_NAN == fpclassify(d) && signbit(d));
        BOOST_TEST(test("-NAN(...)", double_));
        BOOST_TEST(test_attr("-NAN(...)", double_, d) &&
            FP_NAN == fpclassify(d) && signbit(d));
    }

    return boost::report_errors();
}
