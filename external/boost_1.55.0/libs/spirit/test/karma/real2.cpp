//  Copyright (c) 2001-2011 Hartmut Kaiser
//  Copyright (c) 2011      Bryce Lelbach
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//#define KARMA_FAIL_COMPILATION

#include "real.hpp"

///////////////////////////////////////////////////////////////////////////////
int main()
{
    using namespace boost::spirit;

    {
        using namespace boost::spirit::ascii;

        ///////////////////////////////////////////////////////////////////////
        BOOST_TEST(test_delimited("0.0 ", double_, 0.0, char_(' ')));
        BOOST_TEST(test_delimited("1.0 ", double_, 1.0, char_(' ')));
        BOOST_TEST(test_delimited("1.0 ", double_, 1.0001, char_(' ')));
        BOOST_TEST(test_delimited("1.001 ", double_, 1.001, char_(' ')));
        BOOST_TEST(test_delimited("1.01 ", double_, 1.010, char_(' ')));
        BOOST_TEST(test_delimited("1.1 ", double_, 1.100, char_(' ')));

        BOOST_TEST(test_delimited("1.234e-04 ", double_, 0.00012345, char_(' ')));
        BOOST_TEST(test_delimited("0.001 ", double_, 0.0012345, char_(' ')));
        BOOST_TEST(test_delimited("0.012 ", double_, 0.012345, char_(' ')));
        BOOST_TEST(test_delimited("0.123 ", double_, 0.12345, char_(' ')));
        BOOST_TEST(test_delimited("1.234 ", double_, 1.2345, char_(' ')));
        BOOST_TEST(test_delimited("12.346 ", double_, 12.346, char_(' ')));
        BOOST_TEST(test_delimited("123.46 ", double_, 123.46, char_(' ')));
        BOOST_TEST(test_delimited("1234.5 ", double_, 1234.5, char_(' ')));
        BOOST_TEST(test_delimited("12342.0 ", double_, 12342., char_(' ')));
        BOOST_TEST(test_delimited("1.234e05 ", double_, 123420., char_(' ')));

        BOOST_TEST(test_delimited("-1.0 ", double_, -1.0, char_(' ')));
        BOOST_TEST(test_delimited("-1.234 ", double_, -1.2345, char_(' ')));
        BOOST_TEST(test_delimited("-1.235 ", double_, -1.2346, char_(' ')));
        BOOST_TEST(test_delimited("-1234.2 ", double_, -1234.2, char_(' ')));

        BOOST_TEST(test_delimited("1.0 ", double_(1.0), char_(' ')));
        BOOST_TEST(test_delimited("1.0 ", double_(1.0001), char_(' ')));
        BOOST_TEST(test_delimited("1.001 ", double_(1.001), char_(' ')));
        BOOST_TEST(test_delimited("1.01 ", double_(1.010), char_(' ')));
        BOOST_TEST(test_delimited("1.1 ", double_(1.100), char_(' ')));

        BOOST_TEST(test_delimited("1.234e-04 ", double_(0.00012345), char_(' ')));
        BOOST_TEST(test_delimited("0.001 ", double_(0.0012345), char_(' ')));
        BOOST_TEST(test_delimited("0.012 ", double_(0.012345), char_(' ')));
        BOOST_TEST(test_delimited("0.123 ", double_(0.12345), char_(' ')));
        BOOST_TEST(test_delimited("1.234 ", double_(1.2345), char_(' ')));
        BOOST_TEST(test_delimited("12.346 ", double_(12.346), char_(' ')));
        BOOST_TEST(test_delimited("123.46 ", double_(123.46), char_(' ')));
        BOOST_TEST(test_delimited("1234.5 ", double_(1234.5), char_(' ')));
        BOOST_TEST(test_delimited("12342.0 ", double_(12342.), char_(' ')));
        BOOST_TEST(test_delimited("1.234e05 ", double_(123420.), char_(' ')));
    }

    {
        using namespace boost::spirit::ascii;

        ///////////////////////////////////////////////////////////////////////
        // test NaN and Inf
        BOOST_TEST(test_delimited("nan ", double_, 
            std::numeric_limits<double>::quiet_NaN(), char_(' ')));
        BOOST_TEST(test_delimited("-nan ", double_, 
            -std::numeric_limits<double>::quiet_NaN(), char_(' ')));
        BOOST_TEST(test_delimited("inf ", double_, 
            std::numeric_limits<double>::infinity(), char_(' ')));
        BOOST_TEST(test_delimited("-inf ", double_, 
            -std::numeric_limits<double>::infinity(), char_(' ')));

        typedef karma::real_generator<double, signed_policy<double> > signed_type;
        signed_type const signed_ = signed_type();

        BOOST_TEST(test_delimited("+nan ", signed_, 
            std::numeric_limits<double>::quiet_NaN(), char_(' ')));
        BOOST_TEST(test_delimited("-nan ", signed_, 
            -std::numeric_limits<double>::quiet_NaN(), char_(' ')));
        BOOST_TEST(test_delimited("+inf ", signed_, 
            std::numeric_limits<double>::infinity(), char_(' ')));
        BOOST_TEST(test_delimited("-inf ", signed_, 
            -std::numeric_limits<double>::infinity(), char_(' ')));
        BOOST_TEST(test_delimited(" 0.0 ", signed_, 0.0, char_(' ')));
    }

    {
        using namespace boost::spirit::ascii;

        ///////////////////////////////////////////////////////////////////////
        typedef karma::real_generator<double, scientific_policy<double> > 
            science_type;
        science_type const science = science_type();

        BOOST_TEST(test("0.0e00", science, 0.0));
        BOOST_TEST(test("1.0e00", science, 1.0));

        BOOST_TEST(test("1.234e-05", science, 0.000012345));
        BOOST_TEST(test("1.234e-04", science, 0.00012345));
        BOOST_TEST(test("1.234e-03", science, 0.0012345));
        BOOST_TEST(test("1.234e-02", science, 0.012345));
        BOOST_TEST(test("1.235e-01", science, 0.12345));     // note the rounding error!
        BOOST_TEST(test("1.234e00", science, 1.2345));
        BOOST_TEST(test("1.235e01", science, 12.346));
        BOOST_TEST(test("1.235e02", science, 123.46));
        BOOST_TEST(test("1.234e03", science, 1234.5));
        BOOST_TEST(test("1.234e04", science, 12342.));
        BOOST_TEST(test("1.234e05", science, 123420.));

        BOOST_TEST(test("-1.234e-05", science, -0.000012345));
        BOOST_TEST(test("-1.234e-04", science, -0.00012345));
        BOOST_TEST(test("-1.234e-03", science, -0.0012345));
        BOOST_TEST(test("-1.234e-02", science, -0.012345));
        BOOST_TEST(test("-1.235e-01", science, -0.12345));   // note the rounding error!
        BOOST_TEST(test("-1.234e00", science, -1.2345));
        BOOST_TEST(test("-1.235e01", science, -12.346));
        BOOST_TEST(test("-1.235e02", science, -123.46));
        BOOST_TEST(test("-1.234e03", science, -1234.5));
        BOOST_TEST(test("-1.234e04", science, -12342.));
        BOOST_TEST(test("-1.234e05", science, -123420.));

        BOOST_TEST(test("1.234E-05", upper[science], 0.000012345));
        BOOST_TEST(test("1.234E-04", upper[science], 0.00012345));
        BOOST_TEST(test("1.234E-03", upper[science], 0.0012345));
        BOOST_TEST(test("1.234E-02", upper[science], 0.012345));
        BOOST_TEST(test("1.235E-01", upper[science], 0.12345));     // note the rounding error!
        BOOST_TEST(test("1.234E00", upper[science], 1.2345));
        BOOST_TEST(test("1.235E01", upper[science], 12.346));
        BOOST_TEST(test("1.235E02", upper[science], 123.46));
        BOOST_TEST(test("1.234E03", upper[science], 1234.5));
        BOOST_TEST(test("1.234E04", upper[science], 12342.));
        BOOST_TEST(test("1.234E05", upper[science], 123420.));

        BOOST_TEST(test("-1.234E-05", upper[science], -0.000012345));
        BOOST_TEST(test("-1.234E-04", upper[science], -0.00012345));
        BOOST_TEST(test("-1.234E-03", upper[science], -0.0012345));
        BOOST_TEST(test("-1.234E-02", upper[science], -0.012345));
        BOOST_TEST(test("-1.235E-01", upper[science], -0.12345));   // note the rounding error!
        BOOST_TEST(test("-1.234E00", upper[science], -1.2345));
        BOOST_TEST(test("-1.235E01", upper[science], -12.346));
        BOOST_TEST(test("-1.235E02", upper[science], -123.46));
        BOOST_TEST(test("-1.234E03", upper[science], -1234.5));
        BOOST_TEST(test("-1.234E04", upper[science], -12342.));
        BOOST_TEST(test("-1.234E05", upper[science], -123420.));
    }
    
    {
        BOOST_TEST(test("1.0", lit(1.0)));
        BOOST_TEST(test("1.0", lit(1.0f)));
        BOOST_TEST(test("1.0", lit(1.0l)));

        BOOST_TEST(test("1.0", double_(1.0), 1.0));
        BOOST_TEST(test("1.0", float_(1.0), 1.0f));
        BOOST_TEST(test("1.0", long_double(1.0), 1.0l));
        BOOST_TEST(!test("", double_(1.0), 2.0));
        BOOST_TEST(!test("", float_(1.0), 2.0f));
        BOOST_TEST(!test("", long_double(1.0), 2.0l));
    }
    
    return boost::report_errors();
}

