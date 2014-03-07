//  Copyright (c) 2001-2011 Hartmut Kaiser
//  Copyright (c) 2009 Jean-Francois Ostiguy
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/detail/lightweight_test.hpp>
#include <boost/config/warning_disable.hpp>

#include <boost/spirit/include/lex_lexertl.hpp>
#include <boost/spirit/include/qi.hpp>

#include <string>
#include <iostream>
#include <sstream>

namespace lex = boost::spirit::lex;
namespace qi = boost::spirit::qi;
namespace mpl = boost::mpl;

template <typename Lexer>
struct my_lexer : lex::lexer<Lexer>
{
    my_lexer() 
    {
        delimiter = "BEGIN|END";
        identifier = "[a-zA-Z][_\\.a-zA-Z0-9]*";
        ws = "[ \\t\\n]+";
        real = "([0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?)|([-+]?[1-9]+\\.?([eE][-+]?[0-9]+))";
        integer = "[0-9]+"; 

        this->self += ws[lex::_pass = lex::pass_flags::pass_ignore];
        this->self += delimiter;
        this->self += identifier;
        this->self += real;
        this->self += integer;
        this->self += '=';
        this->self += ';';
    }

    lex::token_def<> ws;
    lex::token_def<std::string> identifier;
    lex::token_def<int> integer;
    lex::token_def<double> real;
    lex::token_def<> delimiter;
};

template <typename Iterator>
struct my_grammar : qi::grammar<Iterator> 
{
    template <typename TokenDef>
    my_grammar( TokenDef const& tok )
      :  my_grammar::base_type(statement) 
    {
        statement 
            =   qi::eoi
            |  *(delimiter | declaration)
            ;

        delimiter = tok.delimiter >> tok.identifier;
        declaration = tok.identifier >> option >> ';';
        option = *(tok.identifier >> '=' >> (tok.real | tok.integer));
    }

    qi::rule<Iterator> statement, delimiter, declaration, option;
};

typedef lex::lexertl::token<char const*
  , mpl::vector<std::string, double, int> > token_type;
typedef lex::lexertl::actor_lexer<token_type> lexer_type;
typedef my_lexer<lexer_type>::iterator_type iterator_type;

int main()
{
    std::string test_string ("BEGIN section\n");
    // we introduce a syntax error: ";;" instead of ";" as a terminator.
    test_string += "Identity;;\n";      // this will make the parser fail 
    test_string += "END section\n" ;

    char const* first = &test_string[0];
    char const* last  = &first[test_string.size()];

    my_lexer<lexer_type> lexer;
    my_grammar<iterator_type> grammar(lexer);

    BOOST_TEST(lex::tokenize_and_parse(first, last, lexer, grammar));
    BOOST_TEST(first != last);

    return boost::report_errors();
}


