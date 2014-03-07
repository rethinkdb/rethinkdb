//  Copyright (c) 2001-2010 Hartmut Kaiser
//  Copyright (c) 2009 Carl Barron
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/detail/lightweight_test.hpp>
#include <boost/spirit/include/lex_static_lexertl.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <exception>

#include "matlib_static.h"
#include "matlib.h"

void test_matrix(std::vector<std::vector<double> > const& x)
{
    BOOST_TEST(x.size() == 3);
    BOOST_TEST(x[0].size() == 2 && x[0][0] == 1 && x[0][1] == 2);
    BOOST_TEST(x[1].size() == 1 && x[1][0] == 3);
    BOOST_TEST(x[2].size() == 3 && x[2][0] == 4 && x[2][1] == 5 && x[2][2] == 6);
}

int main()
{
    std::string input("[[1,2][3][4,5,6]]");
    std::vector<std::vector<double> > results;

    typedef std::string::iterator iter;
    typedef boost::spirit::lex::lexertl::static_actor_lexer<
        boost::spirit::lex::lexertl::token<iter>,
        boost::spirit::lex::lexertl::static_::lexer_matlib
    > lexer_type;

    typedef matlib_tokens<lexer_type> matlib_type;
    matlib_type matrix(results);
    iter first = input.begin();

    try {
        BOOST_TEST(boost::spirit::lex::tokenize(first, input.end(), matrix));
        test_matrix(results);
    }
    catch (std::runtime_error const& e) {
        std::cerr << e.what() << '\n';
        BOOST_TEST(false);
    }
    return boost::report_errors();
}
