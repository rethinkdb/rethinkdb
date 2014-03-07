/*=============================================================================
    Copyright (c) 2001-2011 Hartmut Kaiser
    Copyright (c) 2011      Bryce Lelbach

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#include "bool.hpp"

int main()
{
    using spirit_test::test_attr;
    using spirit_test::test;
    using boost::spirit::qi::bool_;
    using boost::spirit::qi::lit;
    using boost::spirit::qi::no_case;

    {
        BOOST_TEST(test("true", bool_(true)));
        BOOST_TEST(test("false", bool_(false)));
        BOOST_TEST(!test("fasle", bool_(false)));
        BOOST_TEST(!test("false", bool_(true)));
        BOOST_TEST(!test("true", bool_(false)));
    }

    {
        BOOST_TEST(test("True", no_case[bool_(true)]));
        BOOST_TEST(test("False", no_case[bool_(false)]));
        BOOST_TEST(test("TRUE", no_case[bool_(true)]));
        BOOST_TEST(test("FALSE", no_case[bool_(false)]));
        BOOST_TEST(!test("True", no_case[bool_(false)]));
        BOOST_TEST(!test("False", no_case[bool_(true)]));
    }

    {
        bool b = false;
        BOOST_TEST(test_attr("true", bool_(true), b) && b);
        BOOST_TEST(test_attr("false", bool_(false), b) && !b);
        BOOST_TEST(!test_attr("fasle", bool_(false), b));
    }

    {
        typedef boost::spirit::qi::bool_parser<bool, backwards_bool_policies> 
            backwards_bool_type;
        backwards_bool_type const backwards_bool = backwards_bool_type();

        BOOST_TEST(test("true", backwards_bool(true)));
        BOOST_TEST(test("eurt", backwards_bool(false)));
        BOOST_TEST(!test("true", backwards_bool(false)));
        BOOST_TEST(!test("eurt", backwards_bool(true)));
    }

    {
        using boost::phoenix::ref;
        bool n = true, m = false;

        BOOST_TEST(test("true", bool_(ref(n))));
        BOOST_TEST(!test("true", bool_(ref(m))));
    }
    
    {
        BOOST_TEST(test("true", lit(true)));
        BOOST_TEST(test("false", lit(false)));
        BOOST_TEST(!test("fasle", lit(false)));
        BOOST_TEST(!test("false", lit(true)));
        BOOST_TEST(!test("true", lit(false)));
    }

    {
        BOOST_TEST(test("True", no_case[lit(true)]));
        BOOST_TEST(test("False", no_case[lit(false)]));
        BOOST_TEST(test("TRUE", no_case[lit(true)]));
        BOOST_TEST(test("FALSE", no_case[lit(false)]));
        BOOST_TEST(!test("True", no_case[lit(false)]));
        BOOST_TEST(!test("False", no_case[lit(true)]));
    }

    {
        using boost::phoenix::ref;
        bool n = true, m = false;

        BOOST_TEST(test("true", lit(ref(n))));
        BOOST_TEST(!test("true", lit(ref(m))));
    }

    return boost::report_errors();
}
