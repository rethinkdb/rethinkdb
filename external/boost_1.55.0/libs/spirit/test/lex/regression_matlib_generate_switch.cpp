//  Copyright (c) 2001-2011 Hartmut Kaiser
//  Copyright (c) 2009 Carl Barron
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/detail/lightweight_test.hpp>
#include <boost/spirit/include/lex_lexertl.hpp>
#include <boost/spirit/include/lex_generate_static_lexertl.hpp>

#include <fstream>
#include <vector>

#include "matlib.h"

int main(int argc, char* argv[])
{
    std::vector<std::vector<double> > results;

    typedef std::string::iterator iter;
    typedef boost::spirit::lex::lexertl::actor_lexer<
        boost::spirit::lex::lexertl::token<iter>
    > lexer_type;

    typedef matlib_tokens<lexer_type> matlib_type;
    matlib_type matrix(results);

    std::ofstream out(argc < 2 ? "matlib_static_switch.h" : argv[1]);
    BOOST_TEST(boost::spirit::lex::lexertl::generate_static_switch(
        matrix, out, "matlib_switch"));
    return boost::report_errors();
}

