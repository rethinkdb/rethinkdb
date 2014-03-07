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
    std::size_t const TOKEN_ID_ABC = 1000;
    std::size_t const TOKEN_ID_STR = 1001;
    std::size_t const TOKEN_ID_WS = 1002;

    token_def c_comment ("\\/\\*[^*]*\\*+([^/*][^*]*\\*+)*\\/", CCOMMENT);
    token_def cpp_comment ("\\/\\/[^\\n\\r]*(\\n|\\r|\\r\\n)", CPPCOMMENT);
    token_def ws_tok ("[\\v\\f\\n\\r]*", TOKEN_ID_WS);

    typedef std::string::iterator base_iterator_type;    
    typedef lex::lexertl::token<base_iterator_type> token_type;
    typedef lex::lexertl::lexer<token_type> lexer_type;

    typedef lex::lexer<lexer_type> lexer_def;

    std::string str("def");

    {
        // initialize lexer
        lexer_def lex;
        token_def ws_tok ("[\\v\\f\\n\\r]*", TOKEN_ID_WS);
        lex.self.add
            (c_comment)(cpp_comment) 
            ('1')('2')('3')
            ("abc", TOKEN_ID_ABC)
            (str, TOKEN_ID_STR)
        ;
        lex.self += token_def(' ') | '\t' | ws_tok;

        // test lexer for different input strings
        BOOST_TEST(test (lex, "/* this is a comment */", CCOMMENT));
        BOOST_TEST(test (lex, "// this is a comment as well\n", CPPCOMMENT));
        BOOST_TEST(test (lex, "\n\n\v\f\r", ws_tok.id()));
        BOOST_TEST(test (lex, " ", ' '));
        BOOST_TEST(test (lex, "2", '2'));
        BOOST_TEST(test (lex, "abc", TOKEN_ID_ABC));
        BOOST_TEST(test (lex, "def", TOKEN_ID_STR));
    }

    {
        // initialize lexer
        lexer_def lex;
        token_def ws_tok ("[\\v\\f\\n\\r]*", TOKEN_ID_WS);

        lex.self.add
            (c_comment)(cpp_comment) 
            ('1')('2')('3')
            ("abc", TOKEN_ID_ABC)
            (str, TOKEN_ID_STR)
        ;

        lex.self("WHITESPACE").add
            (' ')('\t')
            (ws_tok)
        ;

        // test lexer for different input strings
        BOOST_TEST(test (lex, "/* this is a comment */", CCOMMENT));
        BOOST_TEST(test (lex, "// this is a comment as well\n", CPPCOMMENT));
        BOOST_TEST(test (lex, "2", '2'));
        BOOST_TEST(test (lex, "abc", TOKEN_ID_ABC));
        BOOST_TEST(test (lex, "def", TOKEN_ID_STR));

        BOOST_TEST(!test (lex, "\n\n\v\f\r", TOKEN_ID_WS));
        BOOST_TEST(test (lex, " ", ' ', "WHITESPACE"));
        BOOST_TEST(test (lex, "\n\n\v\f\r", TOKEN_ID_WS, "WHITESPACE"));
    }

    return boost::report_errors();
}
