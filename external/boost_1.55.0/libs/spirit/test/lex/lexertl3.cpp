//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/detail/lightweight_test.hpp>
#include <boost/spirit/include/lex_lexertl.hpp>
#include "test.hpp"

///////////////////////////////////////////////////////////////////////////////
int main()
{
    using namespace boost::spirit;
    using namespace boost::spirit::lex;
    using namespace spirit_test;

    // initialize tokens
    typedef lex::token_def<std::string> token_def;

    std::size_t const CCOMMENT = 1;
    std::size_t const CPPCOMMENT = 2;
    token_def c_comment ("\\/\\*[^*]*\\*+([^/*][^*]*\\*+)*\\/", CCOMMENT);
    token_def cpp_comment ("\\/\\/[^\\n\\r]*(\\n|\\r|\\r\\n)", CPPCOMMENT);

    typedef std::string::iterator base_iterator_type;    
    typedef lex::lexertl::token<base_iterator_type> token_type;
    typedef lex::lexertl::lexer<token_type> lexer_type;

    typedef lex::lexer<lexer_type> lexer_def;

    {
        // initialize lexer
        std::string str("def");
        token_def ws_tok ("[\\v\\f\\n\\r]*");
        lexer_def lex;
        lex.self = c_comment;
        lex.self += cpp_comment | '1' | '2' | '3' | "abc" | str;
        lex.self += token_def(' ') | '\t' | ws_tok;

        // test lexer for two different input strings
        BOOST_TEST(test (lex, "/* this is a comment */", CCOMMENT));
        BOOST_TEST(test (lex, "// this is a comment as well\n", CPPCOMMENT));
        BOOST_TEST(test (lex, "\n\n\v\f\r", ws_tok.id()));
        BOOST_TEST(test (lex, " ", ' '));
        BOOST_TEST(test (lex, "2", '2'));
        BOOST_TEST(test (lex, "abc"));
        BOOST_TEST(test (lex, "def"));
    }

    {
        // initialize lexer
        lexer_def lex;
        token_def ws_tok ("[\\v\\f\\n\\r]*");
        lex.self = c_comment;
        lex.self += cpp_comment | '1' | '2' | '3';
        lex.self("WHITESPACE") = token_def(' ') | '\t' | ws_tok;

        // test lexer for two different input strings
        BOOST_TEST(test (lex, "/* this is a comment */", CCOMMENT));
        BOOST_TEST(test (lex, "// this is a comment as well\n", CPPCOMMENT));
        BOOST_TEST(test (lex, "2", '2'));
        BOOST_TEST(!test (lex, "\n\n\v\f\r", ws_tok.id()));
        BOOST_TEST(test (lex, " ", ' ', "WHITESPACE"));
        BOOST_TEST(test (lex, "\n\n\v\f\r", ws_tok.id(), "WHITESPACE"));
    }

    return boost::report_errors();
}
