/*=============================================================================
    Copyright (c) 2001-2011 Hartmut Kaiser

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#include <boost/config/warning_disable.hpp>

#include <boost/spirit/include/karma_operator.hpp>
#include <boost/spirit/include/karma_char.hpp>
#include <boost/spirit/include/karma_string.hpp>
#include <boost/spirit/include/karma_numeric.hpp>
#include <boost/spirit/include/karma_nonterminal.hpp>
#include <boost/spirit/include/karma_generate.hpp>

#include "test.hpp"

using namespace boost::spirit;
using namespace boost::spirit::ascii;

typedef spirit_test::output_iterator<char>::type outiter_type;

struct num_list : karma::grammar<outiter_type, karma::rule<outiter_type> >
{
    num_list() : num_list::base_type(start)
    {
        start = int_(1) << ',' << int_(0);
    }

    karma::rule<outiter_type, karma::rule<outiter_type> > start;
};

// this test must fail compiling as the rule is used with an incompatible 
// delimiter type
int main()
{
    std::string generated;

    std::back_insert_iterator<std::string> outit(generated);
    num_list def;
    generate_delimited(outit, def, char_('%') << '\n');

    return 0;
}
