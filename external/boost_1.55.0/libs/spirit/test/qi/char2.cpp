/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2001-2011 Hartmut Kaiser

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <boost/detail/lightweight_test.hpp>
#include <boost/spirit/include/qi_char.hpp>
#include <boost/spirit/include/qi_action.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>

#include <iostream>
#include "test.hpp"

int
main()
{
    using spirit_test::test;
    using spirit_test::test_attr;
    using spirit_test::print_info;

    {
        using boost::spirit::qi::lit;

        BOOST_TEST(test("x", lit('x')));
        BOOST_TEST(!test("x", lit('y')));

        BOOST_TEST(!test("x", ~lit('x')));
        BOOST_TEST(test(" ", ~lit('x')));
        BOOST_TEST(test("X", ~lit('x')));

        BOOST_TEST(test("x", ~~lit('x')));
        BOOST_TEST(!test(" ", ~~lit('x')));
        BOOST_TEST(!test("X", ~~lit('x')));
    }

    {
        using boost::spirit::qi::lit;
        using boost::spirit::qi::space;

        BOOST_TEST(test("   x", lit('x'), space));
        BOOST_TEST(!test("   x", lit('y'), space));
    }

    {
        using boost::spirit::qi::lit;

        BOOST_TEST(test(L"x", lit(L'x')));
        BOOST_TEST(!test(L"x", lit(L'y')));

        BOOST_TEST(!test(L"x", ~lit(L'x')));
        BOOST_TEST(test(L" ", ~lit(L'x')));
        BOOST_TEST(test(L"X", ~lit(L'x')));

        BOOST_TEST(test(L"x", ~~lit(L'x')));
        BOOST_TEST(!test(L" ", ~~lit(L'x')));
        BOOST_TEST(!test(L"X", ~~lit(L'x')));
    }

    {   // lazy chars
        using boost::spirit::qi::lit;

        using boost::phoenix::val;

        BOOST_TEST((test("x", lit(val('x')))));
    }

    return boost::report_errors();
}
