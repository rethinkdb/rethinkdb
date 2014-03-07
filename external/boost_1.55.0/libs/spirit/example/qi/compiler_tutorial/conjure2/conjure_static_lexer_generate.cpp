//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This small utility program generates the 2 static lexers, the static table 
// driven and the static switch based lexer.

#include <fstream>
#include <iostream>

#include "lexer_def.hpp"
#include <boost/spirit/include/lex_generate_static_lexertl.hpp>

int main()
{
    typedef std::string::const_iterator base_iterator_type;
    typedef client::lexer::conjure_tokens<base_iterator_type> lexer_type;

    lexer_type lexer;

    // first generate the static switch based lexer 
    std::ofstream out_static("conjure_static_switch_lexer.hpp");

    bool result = boost::spirit::lex::lexertl::generate_static_switch(
        lexer, out_static, "conjure_static_switch");
    if (!result) {
        std::cerr << "Failed to generate static switch based lexer\n";
        return -1;
    }

    // now generate the static table based lexer 
    std::ofstream out("conjure_static_lexer.hpp");
    result = boost::spirit::lex::lexertl::generate_static(
        lexer, out, "conjure_static");
    if (!result) {
        std::cerr << "Failed to generate static table based lexer\n";
        return -1;
    }

    return 0;
}

