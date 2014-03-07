/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2001-2011 Hartmut Kaiser
    http://spirit.sourceforge.net/

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <boost/detail/lightweight_test.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/spirit/include/qi_string.hpp>
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
    using boost::spirit::qi::lit;
    using boost::spirit::qi::_1;

    {
        BOOST_TEST((test("kimpo", lit("kimpo"))));

        std::basic_string<char> s("kimpo");
        std::basic_string<wchar_t> ws(L"kimpo");
        BOOST_TEST((test("kimpo", lit(s))));
        BOOST_TEST((test(L"kimpo", lit(ws))));
    }

    {
        std::basic_string<char> s("kimpo");
        BOOST_TEST((test("kimpo", lit(s))));

        std::basic_string<wchar_t> ws(L"kimpo");
        BOOST_TEST((test(L"kimpo", lit(ws))));
    }

    {
        using namespace boost::spirit::ascii;
        BOOST_TEST((test("    kimpo", lit("kimpo"), space)));
        BOOST_TEST((test(L"    kimpo", lit(L"kimpo"), space)));
    }

    {
        using namespace boost::spirit::ascii;
        BOOST_TEST((test("    kimpo", lit("kimpo"), space)));
        BOOST_TEST((test(L"    kimpo", lit(L"kimpo"), space)));
    }

    return boost::report_errors();
}
