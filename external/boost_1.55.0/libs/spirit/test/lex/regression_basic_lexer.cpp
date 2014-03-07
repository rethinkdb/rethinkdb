//  Copyright (c) 2001-2011 Hartmut Kaiser
//  Copyright (c) 2009 Pavel Baranov
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/detail/lightweight_test.hpp>
#include <boost/spirit/include/lex_lexertl.hpp>

#include <iostream>
#include <string>

using namespace boost::spirit;
using namespace boost::spirit::lex;

typedef const char * base_iterator;

///////////////////////////////////////////////////////////////////////////////
//  Token definition
///////////////////////////////////////////////////////////////////////////////
template <typename Lexer>
struct position_helper_tokens : lexer<Lexer> 
{
    position_helper_tokens()
    {
        // define tokens and associate them with the lexer
        eol = "\n";
        any = "[^\n]+";

        // associate tokens with the lexer
        this->self
            =   eol
            |   any
            ;
    }

    token_def<> any, eol;
};

int main()
{
    // read input from the given file
    std::string str ("test");

    // token type
    typedef lexertl::token<base_iterator, lex::omit, boost::mpl::false_> token_type;

    // lexer type
    typedef lexertl::actor_lexer<token_type> lexer_type;

    // create the lexer object instance needed to invoke the lexical analysis
    position_helper_tokens<lexer_type> position_helper_lexer;

    // tokenize the given string, all generated tokens are discarded
    base_iterator first = str.c_str(); 
    base_iterator last = &first[str.size()];

    for(lexer_type::iterator_type i = position_helper_lexer.begin(first, last); 
        i != position_helper_lexer.end() && (*i).is_valid(); i++ )
    {
    }
    return boost::report_errors();
}

