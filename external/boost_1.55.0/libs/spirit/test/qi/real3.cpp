/*=============================================================================
    Copyright (c) 2001-2010 Joel de Guzman
    Copyright (c) 2001-2010 Hartmut Kaiser

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
    //  strict real number tests
    ///////////////////////////////////////////////////////////////////////////
    {
        using boost::spirit::qi::real_parser;
        using boost::spirit::qi::parse;
        using boost::spirit::qi::strict_ureal_policies;
        using boost::spirit::qi::strict_real_policies;

        real_parser<double, strict_ureal_policies<double> > strict_udouble;
        real_parser<double, strict_real_policies<double> > strict_double;
        double  d;

        BOOST_TEST(!test("1234", strict_udouble));
        BOOST_TEST(!test_attr("1234", strict_udouble, d));

        BOOST_TEST(test("1.2", strict_udouble));
        BOOST_TEST(test_attr("1.2", strict_udouble, d) && compare(d, 1.2));

        BOOST_TEST(!test("-1234", strict_double));
        BOOST_TEST(!test_attr("-1234", strict_double, d));

        BOOST_TEST(test("123.", strict_double));
        BOOST_TEST(test_attr("123.", strict_double, d) && compare(d, 123));

        BOOST_TEST(test("3.E6", strict_double));
        BOOST_TEST(test_attr("3.E6", strict_double, d) && compare(d, 3e6));

        real_parser<double, no_trailing_dot_policy<double> > notrdot_real;
        real_parser<double, no_leading_dot_policy<double> > nolddot_real;

        BOOST_TEST(!test("1234.", notrdot_real));          //  Bad trailing dot
        BOOST_TEST(!test(".1234", nolddot_real));          //  Bad leading dot
    }

    ///////////////////////////////////////////////////////////////////////////
    //  Special thousands separated numbers
    ///////////////////////////////////////////////////////////////////////////
    {
        using boost::spirit::qi::real_parser;
        using boost::spirit::qi::parse;
        real_parser<double, ts_real_policies<double> > ts_real;
        double  d;

        BOOST_TEST(test("123,456,789.01", ts_real));
        BOOST_TEST(test_attr("123,456,789.01", ts_real, d)
                && compare(d, 123456789.01));

        BOOST_TEST(test("12,345,678.90", ts_real));
        BOOST_TEST(test_attr("12,345,678.90", ts_real, d)
                && compare(d, 12345678.90));

        BOOST_TEST(test("1,234,567.89", ts_real));
        BOOST_TEST(test_attr("1,234,567.89", ts_real, d)
                && compare(d, 1234567.89));

        BOOST_TEST(!test("1234,567,890", ts_real));
        BOOST_TEST(!test("1,234,5678,9", ts_real));
        BOOST_TEST(!test("1,234,567.89e6", ts_real));
        BOOST_TEST(!test("1,66", ts_real));
    }

    return boost::report_errors();
}
