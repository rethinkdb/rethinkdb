//  Copyright (c) 2001-2011 Hartmut Kaiser
//  Copyright (c) 2011 Ryan Molden
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/detail/lightweight_test.hpp>
#include <boost/config/warning_disable.hpp>

#include <boost/spirit/include/lex_lexertl.hpp>
#include <boost/spirit/include/lex_generate_static_lexertl.hpp>
#include <boost/spirit/include/lex_static_lexertl.hpp>

#include <fstream>

using namespace std;
using namespace boost::spirit;

template <typename BaseLexer>
struct my_lexer : boost::spirit::lex::lexer<BaseLexer>
{
    my_lexer()
    {
        token = L"Yay winning!";
        this->self = token;
    }

    lex::token_def<lex::unused_type, wchar_t> token;
};

int main(int argc, char* argv[])
{
    typedef lex::lexertl::token<wchar_t const*> token_type;
    typedef lex::lexertl::lexer<token_type> lexer_type;

    my_lexer<lexer_type> lexer;

    basic_ofstream<wchar_t> output_dfa("test_dfa.hpp");
    BOOST_TEST(lex::lexertl::generate_static_dfa(lexer, output_dfa, L"test_dfa"));

    basic_ofstream<wchar_t> output_switch("test_switch.hpp");
    BOOST_TEST(lex::lexertl::generate_static_switch(lexer, output_switch, L"test_switch"));
    return boost::report_errors();
}
