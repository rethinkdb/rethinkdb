/*=============================================================================
    Copyright (c) 2001-2010 Hartmut Kaiser

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#include <boost/spirit/include/qi_operator.hpp>
#include <boost/spirit/include/qi_char.hpp>
#include <boost/spirit/include/qi_numeric.hpp>
#include <boost/spirit/include/qi_nonterminal.hpp>
#include <boost/spirit/include/qi_parse.hpp>

using namespace boost::spirit;
using namespace boost::spirit::qi;
using namespace boost::spirit::ascii;

// this test must fail compiling
int main()
{
    char const* input = "some input, it doesn't matter";
    char const* end = &input[strlen(input)+1];

    rule<char const*, rule<char const*> > def;
    def = int_ >> *(',' >> int_);

    bool r = phrase_parse(input, end, def,
        qi::space | ('%' >> *~char_('\n') >> '\n'));

    return 0;
}
