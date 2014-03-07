//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/detail/lightweight_test.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/lex_lexertl.hpp>
#include "test_parser.hpp"

///////////////////////////////////////////////////////////////////////////////
//  Token definition
///////////////////////////////////////////////////////////////////////////////
template <typename Lexer>
struct switch_state_tokens : boost::spirit::lex::lexer<Lexer>
{
    switch_state_tokens()
    {
        // define tokens and associate them with the lexer
        identifier = "[a-zA-Z_][a-zA-Z0-9_]*";
        this->self = identifier;

        // any token definition to be used as the skip parser during parsing 
        // has to be associated with a separate lexer state (here 'WS') 
        white_space = "[ \\t\\n]+";
        this->self("WS") = white_space;

        separators = "[,;]";
        this->self("SEP") = separators;
    }

    boost::spirit::lex::token_def<> identifier, white_space, separators;
};

///////////////////////////////////////////////////////////////////////////////
int main()
{
    using namespace boost::spirit;
    using namespace boost::spirit::qi;
    using namespace spirit_test;

    typedef std::string::iterator base_iterator_type;
    typedef boost::spirit::lex::lexertl::token<base_iterator_type> token_type;
    typedef boost::spirit::lex::lexertl::lexer<token_type> lexer_type;

    {
        // the tokens class will be initialized inside the test_parser function
        switch_state_tokens<lexer_type> lex;

        BOOST_TEST(test_parser("ident", lex.identifier, lex));
        BOOST_TEST(!test_parser("ident", set_state("WS") >> lex.identifier, lex));
        BOOST_TEST(!test_parser("ident", in_state("WS")[lex.identifier], lex));

        BOOST_TEST(test_parser("\t \n", set_state("WS") >> lex.white_space, lex));
        BOOST_TEST(test_parser("\t \n", in_state("WS")[lex.white_space], lex));
        BOOST_TEST(!test_parser("\t \n", lex.white_space, lex));
    }

    {
        // the tokens class will be initialized inside the test_parser function
        switch_state_tokens<lexer_type> lex;

        BOOST_TEST(test_parser(",ident", lex.identifier, lex, 
            in_state("SEP")[lex.separators]));
        BOOST_TEST(!test_parser(";ident", set_state("WS") >> lex.identifier, 
            lex, in_state("SEP")[lex.separators]));
        BOOST_TEST(!test_parser(",ident", in_state("WS")[lex.identifier], 
            lex, in_state("SEP")[lex.separators]));

        BOOST_TEST(test_parser(",\t \n", set_state("WS") >> lex.white_space, 
            lex, in_state("SEP")[lex.separators]));
        BOOST_TEST(test_parser(";\t \n", in_state("WS")[lex.white_space], 
            lex, in_state("SEP")[lex.separators]));
        BOOST_TEST(!test_parser(",\t \n", lex.white_space, lex, 
            in_state("SEP")[lex.separators]));
    }

    {
        // the tokens class will be initialized inside the test_parser function
        switch_state_tokens<lexer_type> lex;
        
        BOOST_TEST(test_parser("ident\t \n", 
            lex.identifier >> set_state("WS") >> lex.white_space, lex));
        BOOST_TEST(test_parser("\t \nident", 
            in_state("WS")[lex.white_space] >> lex.identifier, lex));
    }

    return boost::report_errors();
}

