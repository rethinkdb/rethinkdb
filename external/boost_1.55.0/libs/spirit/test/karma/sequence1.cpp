//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// #define KARMA_TEST_COMPILE_FAIL

#include <boost/config/warning_disable.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <boost/spirit/include/karma_char.hpp>
#include <boost/spirit/include/karma_string.hpp>
#include <boost/spirit/include/karma_numeric.hpp>
#include <boost/spirit/include/karma_generate.hpp>
#include <boost/spirit/include/karma_operator.hpp>
#include <boost/spirit/include/karma_directive.hpp>
#include <boost/spirit/include/karma_action.hpp>
#include <boost/spirit/include/karma_nonterminal.hpp>
#include <boost/spirit/include/karma_auxiliary.hpp>
#include <boost/spirit/include/karma_directive.hpp>
#include <boost/spirit/include/support_unused.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_statement.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/fusion/include/vector.hpp>

#include "test.hpp"

using namespace spirit_test;

///////////////////////////////////////////////////////////////////////////////
int main()
{
    using namespace boost::spirit;
    using namespace boost::spirit::ascii;
    namespace fusion = boost::fusion;

    {
        BOOST_TEST(test("xi", char_('x') << char_('i')));
        BOOST_TEST(!test("xi", char_('x') << char_('o')));
    }

    {
        BOOST_TEST(test_delimited("x i ", char_('x') << 'i', char(' ')));
        BOOST_TEST(!test_delimited("x i ", 
            char_('x') << char_('o'), char(' ')));
    }

    {
        BOOST_TEST(test_delimited("Hello , World ", 
            lit("Hello") << ',' << "World", char(' ')));
    }

    {
        // a single element
        char attr = 'a';
        BOOST_TEST((test("ab", char_ << 'b', attr)));
    }

    {
        // a single element fusion sequence
        fusion::vector<char> attr('a');
        BOOST_TEST((test("ab", char_ << 'b', attr)));
    }

    {
        fusion::vector<char, char, std::string> p ('a', 'b', "cdefg");
        BOOST_TEST(test("abcdefg", char_ << char_ << string, p));
        BOOST_TEST(test_delimited("a b cdefg ", 
            char_ << char_ << string, p, char(' ')));
    }

    {
        fusion::vector<char, int, char> p ('a', 12, 'c');
        BOOST_TEST(test("a12c", char_ << int_ << char_, p));
        BOOST_TEST(test_delimited("a 12 c ", 
            char_ << int_ << char_, p, char(' ')));
    }

    {
        // element sequence can be shorter and longer than the attribute 
        // sequence
        using boost::spirit::karma::strict;
        using boost::spirit::karma::relaxed;

        fusion::vector<char, int, char> p ('a', 12, 'c');
        BOOST_TEST(test("a12", char_ << int_, p));
        BOOST_TEST(test_delimited("a 12 ", char_ << int_, p, char(' ')));

        BOOST_TEST(test("a12", relaxed[char_ << int_], p));
        BOOST_TEST(test_delimited("a 12 ", relaxed[char_ << int_], p, char(' ')));

        BOOST_TEST(!test("", strict[char_ << int_], p));
        BOOST_TEST(!test_delimited("", strict[char_ << int_], p, char(' ')));

        fusion::vector<char, int> p1 ('a', 12);
        BOOST_TEST(test("a12c", char_ << int_ << char_('c'), p1));
        BOOST_TEST(test_delimited("a 12 c ", char_ << int_ << char_('c'), 
            p1, char(' ')));

        BOOST_TEST(test("a12c", relaxed[char_ << int_ << char_('c')], p1));
        BOOST_TEST(test_delimited("a 12 c ", 
            relaxed[char_ << int_ << char_('c')], p1, char(' ')));

        BOOST_TEST(!test("", strict[char_ << int_ << char_('c')], p1));
        BOOST_TEST(!test_delimited("", strict[char_ << int_ << char_('c')], 
            p1, char(' ')));

        BOOST_TEST(test("a12", strict[char_ << int_], p1));
        BOOST_TEST(test_delimited("a 12 ", strict[char_ << int_], p1, char(' ')));

        std::string value("foo ' bar");
        BOOST_TEST(test("\"foo ' bar\"", '"' << strict[*(~char_('*'))] << '"', value));
        BOOST_TEST(test("\"foo ' bar\"", strict['"' << *(~char_('*')) << '"'], value));
    }

    {
        // if all elements of a sequence have unused parameters, the whole 
        // sequence has an unused parameter as well
        fusion::vector<char, char> p ('a', 'e');
        BOOST_TEST(test("abcde", 
            char_ << (lit('b') << 'c' << 'd') << char_, p));
        BOOST_TEST(test_delimited("a b c d e ", 
            char_ << (lit('b') << 'c' << 'd') << char_, p, char(' ')));
    }

    {
        // literal generators do not need an attribute
        fusion::vector<char, char> p('a', 'c');
        BOOST_TEST(test("abc", char_ << 'b' << char_, p));
        BOOST_TEST(test_delimited("a b c ", 
            char_ << 'b' << char_, p, char(' ')));
    }

    {
        // literal generators do not need an attribute, not even at the end
        fusion::vector<char, char> p('a', 'c');
        BOOST_TEST(test("acb", char_ << char_ << 'b', p));
        BOOST_TEST(test_delimited("a c b ", 
            char_ << char_ << 'b', p, char(' ')));
    }

    return boost::report_errors();
}

