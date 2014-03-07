/*=============================================================================
    Copyright (c) 2001-2010 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <boost/detail/lightweight_test.hpp>
#include <boost/spirit/include/qi_operator.hpp>
#include <boost/spirit/include/qi_char.hpp>
#include <boost/spirit/include/qi_string.hpp>
#include <boost/spirit/include/qi_numeric.hpp>
#include <boost/spirit/include/qi_directive.hpp>
#include <boost/spirit/include/qi_action.hpp>
#include <boost/spirit/include/support_argument.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>

#include <string>
#include <iostream>
#include "test.hpp"

int
main()
{
    using namespace boost::spirit::ascii;
    using boost::spirit::lit;
    using spirit_test::test;

    {
        BOOST_TEST(test("b", char_ - 'a'));
        BOOST_TEST(!test("a", char_ - 'a'));
        BOOST_TEST(test("/* abcdefghijk */", "/*" >> *(char_ - "*/") >> "*/"));
    }

    {
        BOOST_TEST(test("b", char_ - no_case['a']));
        BOOST_TEST(!test("a", char_ - no_case['a']));
        BOOST_TEST(!test("A", char_ - no_case['a']));

        BOOST_TEST(test("b", no_case[lower - 'a']));
        BOOST_TEST(test("B", no_case[lower - 'a']));
        BOOST_TEST(!test("a", no_case[lower - 'a']));
        BOOST_TEST(!test("A", no_case[lower - 'a']));
    }

    {
        // $$$ See difference.hpp why these tests are not done anymore. $$$

        // BOOST_TEST(test("switcher", lit("switcher") - "switch"));
        // BOOST_TEST(test("    switcher    ", lit("switcher") - "switch", space));

        BOOST_TEST(!test("switch", lit("switch") - "switch"));
    }

    {
        using boost::spirit::_1;
        namespace phx = boost::phoenix;

        std::string s;

        BOOST_TEST(test(
            "/*abcdefghijk*/"
          , "/*" >> *(char_ - "*/")[phx::ref(s) += _1] >> "*/"
        ));
        BOOST_TEST(s == "abcdefghijk");
        s.clear();

        BOOST_TEST(test(
            "    /*abcdefghijk*/"
          , "/*" >> *(char_ - "*/")[phx::ref(s) += _1] >> "*/"
          , space
        ));
        BOOST_TEST(s == "abcdefghijk");
    }

    return boost::report_errors();
}

