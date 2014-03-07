/*=============================================================================
    Copyright (c) 2001-2011 Hartmut Kaiser

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/karma_operator.hpp>
#include <boost/spirit/include/karma_char.hpp>
#include <boost/spirit/include/karma_numeric.hpp>
#include <boost/spirit/include/karma_nonterminal.hpp>
#include <boost/spirit/include/karma_generate.hpp>
#include <boost/function_output_iterator.hpp>

#include "test.hpp"

using namespace boost::spirit;
using namespace boost::spirit::ascii;

// this test must fail compiling as the rule is used with an incompatible 
// delimiter type
int main()
{
    typedef spirit_test::output_iterator<char>::type outiter_type;

    std::string generated;

    karma::rule<outiter_type, karma::rule<outiter_type> > def;
    def = int_(1) << ',' << int_(0);

    std::back_insert_iterator<std::string> outit(generated);
    generate_delimited(outit, def, char_('%') << '\n');

    return 0;
}
