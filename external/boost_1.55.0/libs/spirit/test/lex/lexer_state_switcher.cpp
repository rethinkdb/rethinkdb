//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/mpl/print.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <boost/spirit/include/phoenix_object.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_statement.hpp>
#include <boost/spirit/include/lex_lexertl.hpp>

///////////////////////////////////////////////////////////////////////////////
//  Token definition
///////////////////////////////////////////////////////////////////////////////
template <typename Lexer>
struct switch_state_tokens : boost::spirit::lex::lexer<Lexer>
{
    // define tokens and associate them with the lexer
    switch_state_tokens()
    {
        namespace phoenix = boost::phoenix;
        using boost::spirit::lex::_state;

        identifier = "[a-zA-Z_][a-zA-Z0-9_]*";
        this->self = identifier [ phoenix::ref(state_) = _state ];

        integer = "[0-9]+";
        this->self("INT") = integer [ _state = "INITIAL" ];
    }

    std::string state_;
    boost::spirit::lex::token_def<> identifier, integer;
};

///////////////////////////////////////////////////////////////////////////////
int main()
{
    using namespace boost::spirit;
    using namespace boost::spirit::lex;

    typedef std::string::iterator base_iterator_type;
    typedef boost::spirit::lex::lexertl::token<base_iterator_type> token_type;
    typedef boost::spirit::lex::lexertl::actor_lexer<token_type> lexer_type;

    {
        switch_state_tokens<lexer_type> lex;

        {
            // verify whether using _state as an rvalue works
            std::string input("abc123");
            base_iterator_type first = input.begin();
            BOOST_TEST(boost::spirit::lex::tokenize(first, input.end(), lex) && 
                lex.state_ == "INITIAL");
        }
        {
            // verify whether using _state as an lvalue works
            std::string input("123abc123");
            base_iterator_type first = input.begin();
            BOOST_TEST(boost::spirit::lex::tokenize(first, input.end(), lex, "INT") && 
                lex.state_ == "INITIAL");
        }
    }

    return boost::report_errors();
}

