//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/detail/lightweight_test.hpp>
#include <boost/spirit/include/lex_lexertl.hpp>
#include <string>

#include "test.hpp"

///////////////////////////////////////////////////////////////////////////////
// a simple lexer class
template <typename Lexer>
struct lexertl_test 
  : boost::spirit::lex::lexer<Lexer>
{
    typedef boost::spirit::lex::token_def<std::string> token_def;

    static std::size_t const CCOMMENT = 1;
    static std::size_t const CPPCOMMENT = 2;

    lexertl_test()
      : c_comment("\\/\\*[^*]*\\*+([^/*][^*]*\\*+)*\\/", CCOMMENT)
      , cpp_comment("\\/\\/[^\\n\\r]*(\\n|\\r|\\r\\n)", CPPCOMMENT)
    {
        this->self = c_comment;
        this->self += cpp_comment;
    }

    token_def c_comment, cpp_comment;
};

template <typename Lexer>
struct wlexertl_test 
  : boost::spirit::lex::lexer<Lexer>
{
    typedef boost::spirit::lex::token_def<std::basic_string<wchar_t>, wchar_t> 
        token_def;

    static std::size_t const CCOMMENT = 1;
    static std::size_t const CPPCOMMENT = 2;

    wlexertl_test()
      : c_comment(L"\\/\\*[^*]*\\*+([^/*][^*]*\\*+)*\\/", CCOMMENT)
      , cpp_comment(L"\\/\\/[^\\n\\r]*(\\n|\\r|\\r\\n)", CPPCOMMENT)
    {
        this->self = c_comment;
        this->self += cpp_comment;
    }

    token_def c_comment, cpp_comment;
};

///////////////////////////////////////////////////////////////////////////////
int main()
{
    using namespace boost::spirit;
    using namespace spirit_test;

    // the following test aims at the low level lexer_ and token_ objects, 
    // normally not visible/used by the user
    {
        // initialize lexer
        typedef std::string::iterator base_iterator_type;
        typedef lex::lexertl::token<base_iterator_type> token_type;
        typedef lex::lexertl::lexer<token_type> lexer_type;
        typedef lexertl_test<lexer_type> lexer_def;

        // test lexer for two different input strings
        lexer_def lex;
        BOOST_TEST(test (lex, "/* this is a comment */", lexer_def::CCOMMENT));
        BOOST_TEST(test (lex, "// this is a comment as well\n", lexer_def::CPPCOMMENT));
    }

    {
        // initialize lexer
        typedef std::basic_string<wchar_t>::iterator base_iterator_type;
        typedef lex::lexertl::token<base_iterator_type> token_type;
        typedef lex::lexertl::lexer<token_type> lexer_type;
        typedef wlexertl_test<lexer_type> lexer_def;

        // test lexer for two different input strings
        lexer_def lex;
        BOOST_TEST(test (lex, L"/* this is a comment */", lexer_def::CCOMMENT));
        BOOST_TEST(test (lex, L"// this is a comment as well\n", lexer_def::CPPCOMMENT));
    }

    return boost::report_errors();
}

