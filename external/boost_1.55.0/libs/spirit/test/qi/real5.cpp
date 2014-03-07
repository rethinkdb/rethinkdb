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
    //  parameterized signed real number tests
    ///////////////////////////////////////////////////////////////////////////
    {
        using boost::spirit::qi::double_;
        double d;

        BOOST_TEST(test("+1234", double_(1234)));
        BOOST_TEST(!test("+1234", double_(-1234)));
        BOOST_TEST(test_attr("+1234", double_(1234), d));
        BOOST_TEST(compare(d, 1234));
        BOOST_TEST(!test_attr("+1234", double_(-1234), d));
        
        BOOST_TEST(test("-1234", double_(-1234)));
        BOOST_TEST(!test("-1234", double_(1234)));
        BOOST_TEST(test_attr("-1234", double_(-1234), d));
        BOOST_TEST(compare(d, -1234));
        BOOST_TEST(!test_attr("-1234", double_(1234), d));

        BOOST_TEST(test("+1.2e3", double_(1.2e3)));
        BOOST_TEST(!test("+1.2e3", double_(-1.2e3)));
        BOOST_TEST(test_attr("+1.2e3", double_(1.2e3), d));
        BOOST_TEST(compare(d, 1.2e3));
        BOOST_TEST(!test_attr("+1.2e3", double_(-1.2e3), d));
        
        BOOST_TEST(test("-1.2e3", double_(-1.2e3)));
        BOOST_TEST(!test("-1.2e3", double_(1.2e3)));
        BOOST_TEST(test_attr("-1.2e3", double_(-1.2e3), d));
        BOOST_TEST(compare(d, -1.2e3));
        BOOST_TEST(!test_attr("-1.2e3", double_(1.2e3), d));
    }

    ///////////////////////////////////////////////////////////////////////////
    //  parameterized unsigned real number tests
    ///////////////////////////////////////////////////////////////////////////
    {
        using boost::spirit::qi::real_parser;
        using boost::spirit::qi::ureal_policies;
        double d;

        real_parser<double, ureal_policies<double> > udouble;

        BOOST_TEST(test("1234", udouble(1234)));
        BOOST_TEST(!test("1234", udouble(4321)));
        BOOST_TEST(test_attr("1234", udouble(1234), d));
        BOOST_TEST(compare(d, 1234));
        BOOST_TEST(!test_attr("1234", udouble(4321), d));

        BOOST_TEST(test("1.2e3", udouble(1.2e3)));
        BOOST_TEST(!test("1.2e3", udouble(3.2e1)));
        BOOST_TEST(test_attr("1.2e3", udouble(1.2e3), d));
        BOOST_TEST(compare(d, 1.2e3));
        BOOST_TEST(!test_attr("1.2e3", udouble(3.2e1), d));
    }
    
    ///////////////////////////////////////////////////////////////////////////
    //  parameterized custom data type
    ///////////////////////////////////////////////////////////////////////////
    {
        using boost::math::concepts::real_concept;
        using boost::spirit::qi::real_parser;
        using boost::spirit::qi::real_policies;

        real_parser<real_concept, real_policies<real_concept> > custom_real;
        real_concept d;

        BOOST_TEST(test("-1234", custom_real(-1234)));
        BOOST_TEST(!test("-1234", custom_real(4321)));
        BOOST_TEST(test_attr("-1234", custom_real(-1234), d));
        BOOST_TEST(compare(d, -1234));
        BOOST_TEST(!test_attr("-1234", custom_real(-4321), d));

        BOOST_TEST(test("1.2e3", custom_real(1.2e3)));
        BOOST_TEST(!test("1.2e3", custom_real(-1.2e3)));
        BOOST_TEST(test_attr("1.2e3", custom_real(1.2e3), d));
        BOOST_TEST(compare(d, 1.2e3));
        BOOST_TEST(!test_attr("1.2e3", custom_real(-3.2e1), d));
    }
    
    ///////////////////////////////////////////////////////////////////////////
    //  parameterized lazy tests
    ///////////////////////////////////////////////////////////////////////////
    {
        using boost::phoenix::ref;
        using boost::spirit::qi::double_;
        double n = 1.2e3, m = 3.2e1;

        BOOST_TEST(test("1.2e3", double_(ref(n))));
        BOOST_TEST(!test("1.2e3", double_(ref(m))));
    }
   
    ///////////////////////////////////////////////////////////////////////////
    //  literal real number tests
    ///////////////////////////////////////////////////////////////////////////
    {
        using boost::spirit::qi::lit;

        BOOST_TEST(test("+1.2e3", lit(1.2e3)));
        BOOST_TEST(!test("+1.2e3", lit(-1.2e3)));
        BOOST_TEST(test("-1.2e3", lit(-1.2e3)));
        BOOST_TEST(!test("-1.2e3", lit(1.2e3)));
        BOOST_TEST(test("1.2e3", lit(1.2e3)));
        BOOST_TEST(!test("1.2e3", lit(3.2e1)));
    }
    
    ///////////////////////////////////////////////////////////////////////////
    //  literal lazy tests
    ///////////////////////////////////////////////////////////////////////////
    {
        using boost::spirit::qi::lit;
        using boost::phoenix::ref;
        double n = 1.2e3, m = 3.2e1;

        BOOST_TEST(test("1.2e3", lit(ref(n))));
        BOOST_TEST(!test("1.2e3", lit(ref(m))));
    }

    return boost::report_errors();
}
