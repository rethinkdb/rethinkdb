//  Copyright (c) 2012 David Bailey
//  Copyright (c) 2001-2012 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/config/warning_disable.hpp>
#include <boost/detail/lightweight_test.hpp>

#ifndef BOOST_SPIRIT_UNICODE
#define BOOST_SPIRIT_UNICODE
#endif

#include <boost/spirit/home/karma/nonterminal/grammar.hpp>
#include <boost/spirit/home/karma/nonterminal/rule.hpp>
#include <boost/spirit/home/karma/char.hpp>

#include "test.hpp"

using namespace spirit_test;

///////////////////////////////////////////////////////////////////////////////
template <typename OutputIterator>
struct unicode_char_grammar_
  : public boost::spirit::karma::grammar<
        OutputIterator, boost::spirit::char_encoding::unicode::char_type()>
{
    unicode_char_grammar_() : unicode_char_grammar_::base_type(thechar)
    {
        using boost::spirit::karma::unicode::char_;
        thechar = char_;
    }

    boost::spirit::karma::rule<
        OutputIterator, boost::spirit::char_encoding::unicode::char_type()
    > thechar;
};

int
main()
{
    using namespace boost::spirit;

    {
        typedef std::basic_string<char_encoding::unicode::char_type> unicode_string;
        typedef std::back_insert_iterator<unicode_string> unicode_back_insert_iterator_type;

        using namespace boost::spirit::unicode;

        BOOST_TEST(test("x", char_, 'x'));
        BOOST_TEST(test(L"x", char_, L'x'));

        char_encoding::unicode::char_type unicodeCharacter = 0x00000078u;
        std::basic_string<char_encoding::unicode::char_type> expected;
        expected.push_back(unicodeCharacter);

        unicode_char_grammar_<unicode_back_insert_iterator_type> unichar;

        BOOST_TEST(test(expected, unichar, unicodeCharacter));
    }

    return boost::report_errors();
}
