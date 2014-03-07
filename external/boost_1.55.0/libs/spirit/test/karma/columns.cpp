//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/config/warning_disable.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <boost/spirit/include/karma_char.hpp>
#include <boost/spirit/include/karma_generate.hpp>
#include <boost/spirit/include/karma_numeric.hpp>
#include <boost/spirit/include/karma_directive.hpp>

#include <boost/spirit/include/karma_nonterminal.hpp>
#include <boost/spirit/include/karma_action.hpp>

#include <boost/spirit/include/phoenix_core.hpp>

#include "test.hpp"

using namespace spirit_test;
namespace karma = boost::spirit::karma;

///////////////////////////////////////////////////////////////////////////////
int
main()
{
    {
        using karma::columns;
        using karma::int_;

        std::vector<int> v;
        for (int i = 0; i < 11; ++i)
            v.push_back(i);

        BOOST_TEST(test("01234\n56789\n10\n", columns[*int_], v));
        BOOST_TEST(test_delimited("0 1 2 3 4 \n5 6 7 8 9 \n10 \n", columns[*int_]
          , v, karma::space));
    }

    {
        using karma::columns;
        using karma::int_;

        std::vector<int> v;
        for (int i = 0; i < 11; ++i)
            v.push_back(i);

        BOOST_TEST(test("012\n345\n678\n910\n", columns(3)[*int_], v));
        BOOST_TEST(test_delimited("0 1 2 \n3 4 5 \n6 7 8 \n9 10 \n"
          , columns(3)[*int_], v, karma::space));
    }

    {
        using karma::columns;
        using karma::int_;
        using boost::phoenix::ref;

        std::vector<int> v;
        for (int i = 0; i < 11; ++i)
            v.push_back(i);

        int count = 3;
        BOOST_TEST(test("012\n345\n678\n910\n", columns(ref(count))[*int_], v));
        BOOST_TEST(test_delimited("0 1 2 \n3 4 5 \n6 7 8 \n9 10 \n"
          , columns(val(ref(count)))[*int_], v, karma::space));
    }

    {
        using karma::columns;
        using karma::int_;
        using karma::lit;

        std::vector<int> v;
        for (int i = 0; i < 11; ++i)
            v.push_back(i);

        BOOST_TEST(test("01234\t56789\t10\t", columns(lit('\t'))[*int_], v));
        BOOST_TEST(test_delimited("0 1 2 3 4 \t5 6 7 8 9 \t10 \t"
          , columns(lit('\t'))[*int_], v, karma::space));
    }

    {
        using karma::columns;
        using karma::int_;
        using karma::lit;

        std::vector<int> v;
        for (int i = 0; i < 11; ++i)
            v.push_back(i);

        BOOST_TEST(test("012\t345\t678\t910\t", columns(3, lit('\t'))[*int_], v));
        BOOST_TEST(test_delimited("0 1 2 \t3 4 5 \t6 7 8 \t9 10 \t"
          , columns(3, lit('\t'))[*int_], v, karma::space));
    }
    return boost::report_errors();
}
