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
        ///////////////////////////////////////////////////////////////////////
        // use the default real_policies
        BOOST_TEST(test("0.0", double_, 0.0));
        BOOST_TEST(test("1.0", double_, 1.0));
        BOOST_TEST(test("1.0", double_, 1.0001));
        BOOST_TEST(test("1.001", double_, 1.001));
        BOOST_TEST(test("1.01", double_, 1.010));
        BOOST_TEST(test("1.1", double_, 1.100));

        BOOST_TEST(test("1.234e-04", double_, 0.00012345));
        BOOST_TEST(test("0.001", double_, 0.0012345));
        BOOST_TEST(test("0.012", double_, 0.012345));
        BOOST_TEST(test("0.123", double_, 0.12345));
        BOOST_TEST(test("1.234", double_, 1.2345));
        BOOST_TEST(test("12.346", double_, 12.346));
        BOOST_TEST(test("123.46", double_, 123.46));
        BOOST_TEST(test("1234.5", double_, 1234.5));
        BOOST_TEST(test("12342.0", double_, 12342.));
        BOOST_TEST(test("1.234e05", double_, 123420.));

        BOOST_TEST(test("-1.0", double_, -1.0));
        BOOST_TEST(test("-1.234", double_, -1.2345));
        BOOST_TEST(test("-1.235", double_, -1.2346));
        BOOST_TEST(test("-1234.2", double_, -1234.2));

        BOOST_TEST(test("1.0", double_(1.0)));
        BOOST_TEST(test("1.0", double_(1.0001)));
        BOOST_TEST(test("1.001", double_(1.001)));
        BOOST_TEST(test("1.01", double_(1.010)));
        BOOST_TEST(test("1.1", double_(1.100)));

        BOOST_TEST(test("1.234e-04", double_(0.00012345)));
        BOOST_TEST(test("0.001", double_(0.0012345)));
        BOOST_TEST(test("0.012", double_(0.012345)));
        BOOST_TEST(test("0.123", double_(0.12345)));
        BOOST_TEST(test("1.234", double_(1.2345)));
        BOOST_TEST(test("12.346", double_(12.346)));
        BOOST_TEST(test("123.46", double_(123.46)));
        BOOST_TEST(test("1234.5", double_(1234.5)));
        BOOST_TEST(test("12342.0", double_(12342.)));
        BOOST_TEST(test("1.234e05", double_(123420.)));
    }

    {
        ///////////////////////////////////////////////////////////////////////
        // test NaN and Inf
        BOOST_TEST(test("nan", double_, std::numeric_limits<double>::quiet_NaN()));
        BOOST_TEST(test("-nan", double_, -std::numeric_limits<double>::quiet_NaN()));
        BOOST_TEST(test("inf", double_, std::numeric_limits<double>::infinity()));
        BOOST_TEST(test("-inf", double_, -std::numeric_limits<double>::infinity()));

        typedef karma::real_generator<double, signed_policy<double> > signed_type;
        signed_type const signed_ = signed_type();

        BOOST_TEST(test("+nan", signed_, std::numeric_limits<double>::quiet_NaN()));
        BOOST_TEST(test("-nan", signed_, -std::numeric_limits<double>::quiet_NaN()));
        BOOST_TEST(test("+inf", signed_, std::numeric_limits<double>::infinity()));
        BOOST_TEST(test("-inf", signed_, -std::numeric_limits<double>::infinity()));
        BOOST_TEST(test(" 0.0", signed_, 0.0));

        BOOST_TEST(test("+nan", signed_(std::numeric_limits<double>::quiet_NaN())));
        BOOST_TEST(test("-nan", signed_(-std::numeric_limits<double>::quiet_NaN())));
        BOOST_TEST(test("+inf", signed_(std::numeric_limits<double>::infinity())));
        BOOST_TEST(test("-inf", signed_(-std::numeric_limits<double>::infinity())));
        BOOST_TEST(test(" 0.0", signed_(0.0)));
    }

    {
        ///////////////////////////////////////////////////////////////////////
        typedef karma::real_generator<double, statefull_policy<double> > 
            statefull_type;

        statefull_policy<double> policy(5, true);
        statefull_type const statefull = statefull_type(policy);

        BOOST_TEST(test("0.00000", statefull, 0.0));
        BOOST_TEST(test("0.00000", statefull(0.0)));

        using namespace boost::phoenix;
        BOOST_TEST(test("0.00000", statefull(val(0.0))));
    }

    {
        ///////////////////////////////////////////////////////////////////////
        typedef karma::real_generator<double, trailing_zeros_policy<double> > 
            trailing_zeros_type;
        trailing_zeros_type const trail_zeros = trailing_zeros_type();

        BOOST_TEST(test("0.0000", trail_zeros, 0.0));
        BOOST_TEST(test("1.0000", trail_zeros, 1.0));
        BOOST_TEST(test("1.0001", trail_zeros, 1.0001));
        BOOST_TEST(test("1.0010", trail_zeros, 1.001));
        BOOST_TEST(test("1.0100", trail_zeros, 1.010));
        BOOST_TEST(test("1.1000", trail_zeros, 1.100));

        BOOST_TEST(test("1.2345e-04", trail_zeros, 0.00012345));
        BOOST_TEST(test("0.0012", trail_zeros, 0.0012345));
        BOOST_TEST(test("0.0123", trail_zeros, 0.012345));
        BOOST_TEST(test("0.1235", trail_zeros, 0.12345));
        BOOST_TEST(test("1.2345", trail_zeros, 1.2345));
        BOOST_TEST(test("12.3460", trail_zeros, 12.346));
        BOOST_TEST(test("123.4600", trail_zeros, 123.46));
        BOOST_TEST(test("1234.5000", trail_zeros, 1234.5));
        BOOST_TEST(test("12342.0000", trail_zeros, 12342.));
        BOOST_TEST(test("1.2342e05", trail_zeros, 123420.));

        BOOST_TEST(test("-1.0000", trail_zeros, -1.0));
        BOOST_TEST(test("-1.2345", trail_zeros, -1.2345));
        BOOST_TEST(test("-1.2346", trail_zeros, -1.2346));
        BOOST_TEST(test("-1234.2000", trail_zeros, -1234.2));

        BOOST_TEST(test("1.0000", trail_zeros(1.0)));
        BOOST_TEST(test("1.0001", trail_zeros(1.0001)));
        BOOST_TEST(test("1.0010", trail_zeros(1.001)));
        BOOST_TEST(test("1.0100", trail_zeros(1.010)));
        BOOST_TEST(test("1.1000", trail_zeros(1.100)));

        BOOST_TEST(test("1.2345e-04", trail_zeros(0.00012345)));
        BOOST_TEST(test("0.0012", trail_zeros(0.0012345)));
        BOOST_TEST(test("0.0123", trail_zeros(0.012345)));
        BOOST_TEST(test("0.1235", trail_zeros(0.12345)));
        BOOST_TEST(test("1.2345", trail_zeros(1.2345)));
        BOOST_TEST(test("12.3460", trail_zeros(12.346)));
        BOOST_TEST(test("123.4600", trail_zeros(123.46)));
        BOOST_TEST(test("1234.5000", trail_zeros(1234.5)));
        BOOST_TEST(test("12342.0000", trail_zeros(12342.)));
        BOOST_TEST(test("1.2342e05", trail_zeros(123420.)));
    }
    
    {
        using namespace boost::spirit::ascii;

        ///////////////////////////////////////////////////////////////////////
        // test NaN and Inf
        BOOST_TEST(test("NAN", upper[double_], 
            std::numeric_limits<double>::quiet_NaN()));
        BOOST_TEST(test("-NAN", upper[double_], 
            -std::numeric_limits<double>::quiet_NaN()));
        BOOST_TEST(test("INF", upper[double_], 
            std::numeric_limits<double>::infinity()));
        BOOST_TEST(test("-INF", upper[double_], 
            -std::numeric_limits<double>::infinity()));

        typedef karma::real_generator<double, signed_policy<double> > signed_type;
        signed_type const signed_ = signed_type();

        BOOST_TEST(test("+NAN", upper[signed_], 
            std::numeric_limits<double>::quiet_NaN()));
        BOOST_TEST(test("-NAN", upper[signed_], 
            -std::numeric_limits<double>::quiet_NaN()));
        BOOST_TEST(test("+INF", upper[signed_], 
            std::numeric_limits<double>::infinity()));
        BOOST_TEST(test("-INF", upper[signed_], 
            -std::numeric_limits<double>::infinity()));
        BOOST_TEST(test(" 0.0", upper[signed_], 0.0));
    }


    return boost::report_errors();
}

