//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/detail/lightweight_test.hpp>
#include <boost/spirit/include/lex_lexertl.hpp>
#include <boost/spirit/include/lex_lexertl_position_token.hpp>
#include "test.hpp"

///////////////////////////////////////////////////////////////////////////////
int main()
{
    using namespace boost::spirit;
    using namespace spirit_test;

    // the following test aims at the low level lexer and token_def objects, 
    // normally not visible to/directly used by the user

    // initialize tokens
    typedef lex::token_def<std::string> token_def;

    std::size_t const CCOMMENT = 1;
    std::size_t const CPPCOMMENT = 2;
    token_def c_comment ("\\/\\*[^*]*\\*+([^/*][^*]*\\*+)*\\/", CCOMMENT);
    token_def cpp_comment ("\\/\\/[^\\n\\r]*(\\n|\\r|\\r\\n)", CPPCOMMENT);

    typedef std::string::iterator base_iterator_type;

    // test with default token type
    typedef lex::lexertl::token<base_iterator_type> token_type;
    typedef lex::lexertl::lexer<token_type> lexer_type;
    typedef lex::lexer<lexer_type> lexer_def;

    {
        // initialize lexer
        lexer_def lex;
        lex.self = c_comment;
        lex.self += cpp_comment;

        // test lexer for two different input strings
        BOOST_TEST(test (lex, "/* this is a comment */", CCOMMENT));
        BOOST_TEST(test (lex, "// this is a comment as well\n", CPPCOMMENT));
    }

    {
        // initialize lexer
        lexer_def lex;
        lex.self = c_comment | cpp_comment;

        // test lexer for two different input strings
        BOOST_TEST(test (lex, "/* this is a comment */", CCOMMENT));
        BOOST_TEST(test (lex, "// this is a comment as well\n", CPPCOMMENT));
    }

    {
        // initialize lexer
        lexer_def lex;
        lex.self = token_def('+') | '-' | c_comment;
        lex.self += lex::char_('*') | '/' | cpp_comment;

        // test lexer for two different input strings
        BOOST_TEST(test (lex, "/* this is a comment */", CCOMMENT));
        BOOST_TEST(test (lex, "// this is a comment as well\n", CPPCOMMENT));
        BOOST_TEST(test (lex, "+", '+'));
        BOOST_TEST(test (lex, "-", '-'));
        BOOST_TEST(test (lex, "*", '*'));
        BOOST_TEST(test (lex, "/", '/'));
    }

    // test with position_token 
    typedef lex::lexertl::position_token<base_iterator_type> position_token_type;
    typedef lex::lexertl::lexer<position_token_type> position_lexer_type;
    typedef lex::lexer<position_lexer_type> position_lexer_def;

    {
        // initialize lexer
        position_lexer_def lex;
        lex.self = c_comment;
        lex.self += cpp_comment;

        // test lexer for two different input strings
        BOOST_TEST(test (lex, "/* this is a comment */", CCOMMENT));
        BOOST_TEST(test (lex, "// this is a comment as well\n", CPPCOMMENT));
    }

    {
        // initialize lexer
        position_lexer_def lex;
        lex.self = c_comment | cpp_comment;

        // test lexer for two different input strings
        BOOST_TEST(test (lex, "/* this is a comment */", CCOMMENT));
        BOOST_TEST(test (lex, "// this is a comment as well\n", CPPCOMMENT));
    }

    {
        // initialize lexer
        position_lexer_def lex;
        lex.self = token_def('+') | '-' | c_comment;
        lex.self += lex::char_('*') | '/' | cpp_comment;

        // test lexer for two different input strings
        BOOST_TEST(test (lex, "/* this is a comment */", CCOMMENT));
        BOOST_TEST(test (lex, "// this is a comment as well\n", CPPCOMMENT));
        BOOST_TEST(test (lex, "+", '+'));
        BOOST_TEST(test (lex, "-", '-'));
        BOOST_TEST(test (lex, "*", '*'));
        BOOST_TEST(test (lex, "/", '/'));
    }

    return boost::report_errors();
}
