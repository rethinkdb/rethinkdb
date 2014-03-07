/*=============================================================================
    Copyright (c) 2001-2010 Hartmut Kaiser

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#include <boost/spirit/include/qi_operator.hpp>
#include <boost/spirit/include/qi_char.hpp>
#include <boost/spirit/include/qi_string.hpp>
#include <boost/spirit/include/qi_numeric.hpp>
#include <boost/spirit/include/qi_nonterminal.hpp>
#include <boost/spirit/include/qi_parse.hpp>

namespace qi = boost::spirit::qi;

struct num_list : qi::grammar<char const*, qi::rule<char const*> >
{
    num_list() : base_type(start)
    {
        num = qi::int_;
        start = num >> *(',' >> num);
    }

    qi::rule<char const*, qi::rule<char const*> > start, num;
};

// this test must fail compiling
int main()
{
    char const* input = "some input, it doesn't matter";
    char const* end = &input[strlen(input)+1];

    num_list g;
    bool r = qi::phrase_parse(input, end, g,
        qi::space | ('%' >> *~qi::char_('\n') >> '\n'));

    return 0;
}
