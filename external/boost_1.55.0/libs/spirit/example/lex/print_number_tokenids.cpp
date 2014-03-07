//  Copyright (c) 2001-2010 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  This example is the equivalent to the following lex program:
//
//     %{
//     #include <stdio.h>
//     %}
//     %%
//     [0-9]+   { printf("%s\n", yytext); }
//     .|\n     ;
//     %%
//     main()
//     {
//             yylex();
//     }
//
//  Its purpose is to print all the (integer) numbers found in a file

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/lex_lexertl.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>

#include <iostream>
#include <string>

#include "example.hpp"

using namespace boost::spirit;

///////////////////////////////////////////////////////////////////////////////
//  Token definition: We use the lexertl based lexer engine as the underlying 
//                    lexer type.
///////////////////////////////////////////////////////////////////////////////
template <typename Lexer>
struct print_numbers_tokenids : lex::lexer<Lexer>
{
    // define tokens and associate it with the lexer, we set the lexer flags
    // not to match newlines while matching a dot, so we need to add the
    // '\n' explicitly below
    print_numbers_tokenids()
      : print_numbers_tokenids::base_type(lex::match_flags::match_not_dot_newline)
    {
        this->self = lex::token_def<int>("[0-9]*") | ".|\n";
    }
};

///////////////////////////////////////////////////////////////////////////////
//  Grammar definition
///////////////////////////////////////////////////////////////////////////////
template <typename Iterator>
struct print_numbers_grammar : qi::grammar<Iterator>
{
    print_numbers_grammar()
      : print_numbers_grammar::base_type(start)
    {
        // we just know, that the token ids get assigned starting min_token_id
        // so, "[0-9]*" gets the id 'min_token_id' and ".|\n" gets the id
        // 'min_token_id+1'.

        // this prints the token ids of the matched tokens
        start =  *(   qi::tokenid(lex::min_token_id) 
                  |   qi::tokenid(lex::min_token_id+1)
                  ) 
                  [ std::cout << _1  << "\n" ]
              ;
    }

    qi::rule<Iterator> start;
};

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    // iterator type used to expose the underlying input stream
    typedef std::string::iterator base_iterator_type;

    // the token type to be used, 'int' is available as the type of the token 
    // attribute and no lexer state is supported
    typedef lex::lexertl::token<base_iterator_type, boost::mpl::vector<int>
      , boost::mpl::false_> token_type;

    // lexer type
    typedef lex::lexertl::lexer<token_type> lexer_type;

    // iterator type exposed by the lexer 
    typedef print_numbers_tokenids<lexer_type>::iterator_type iterator_type;

    // now we use the types defined above to create the lexer and grammar
    // object instances needed to invoke the parsing process
    print_numbers_tokenids<lexer_type> print_tokens;  // Our lexer
    print_numbers_grammar<iterator_type> print;       // Our parser 

    // Parsing is done based on the the token stream, not the character 
    // stream read from the input.
    std::string str (read_from_file(1 == argc ? "print_numbers.input" : argv[1]));
    base_iterator_type first = str.begin();
    bool r = lex::tokenize_and_parse(first, str.end(), print_tokens, print);

    if (r) {
        std::cout << "-------------------------\n";
        std::cout << "Parsing succeeded\n";
        std::cout << "-------------------------\n";
    }
    else {
        std::string rest(first, str.end());
        std::cout << "-------------------------\n";
        std::cout << "Parsing failed\n";
        std::cout << "stopped at: \"" << rest << "\"\n";
        std::cout << "-------------------------\n";
    }

    std::cout << "Bye... :-) \n\n";
    return 0;
}



