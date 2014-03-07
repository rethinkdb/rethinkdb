//  Copyright (c) 2001-2010 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  Simple lexer/parser to test the Spirit installation.
//
//  This example shows, how to create a simple lexer recognizing 5 different 
//  tokens, and how to use a single token definition as the skip parser during 
//  the parsing. Additionally, it demonstrates how to use one of the defined 
//  tokens as a parser component in the grammar.
//
//  The grammar recognizes a simple input structure, for instance:
//
//        {
//            hello world, hello it is me
//        }
//
//  Any number of simple sentences (optionally comma separated) inside a pair 
//  of curly braces will be matched.

// #define BOOST_SPIRIT_LEXERTL_DEBUG

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/lex_lexertl.hpp>

#include <iostream>
#include <fstream>
#include <string>

#include "example.hpp"

using namespace boost::spirit;

///////////////////////////////////////////////////////////////////////////////
//  Token definition
///////////////////////////////////////////////////////////////////////////////
template <typename Lexer>
struct example1_tokens : lex::lexer<Lexer>
{
    example1_tokens()
    {
        // define tokens and associate them with the lexer
        identifier = "[a-zA-Z_][a-zA-Z0-9_]*";
        this->self = lex::char_(',') | '{' | '}' | identifier;

        // any token definition to be used as the skip parser during parsing 
        // has to be associated with a separate lexer state (here 'WS') 
        this->white_space = "[ \\t\\n]+";
        this->self("WS") = white_space;
    }

    lex::token_def<> identifier, white_space;
};

///////////////////////////////////////////////////////////////////////////////
//  Grammar definition
///////////////////////////////////////////////////////////////////////////////
template <typename Iterator>
struct example1_grammar 
  : qi::grammar<Iterator, qi::in_state_skipper<lex::token_def<> > >
{
    template <typename TokenDef>
    example1_grammar(TokenDef const& tok)
      : example1_grammar::base_type(start)
    {
        start = '{' >> *(tok.identifier >> -ascii::char_(',')) >> '}';
    }

    qi::rule<Iterator, qi::in_state_skipper<lex::token_def<> > > start;
};

///////////////////////////////////////////////////////////////////////////////
int main()
{
    // iterator type used to expose the underlying input stream
    typedef std::string::iterator base_iterator_type;

    // This is the token type to return from the lexer iterator
    typedef lex::lexertl::token<base_iterator_type> token_type;

    // This is the lexer type to use to tokenize the input.
    // We use the lexertl based lexer engine.
    typedef lex::lexertl::lexer<token_type> lexer_type;

    // This is the lexer type (derived from the given lexer type).
    typedef example1_tokens<lexer_type> example1_lex;

    // This is the iterator type exposed by the lexer 
    typedef example1_lex::iterator_type iterator_type;

    // This is the type of the grammar to parse
    typedef example1_grammar<iterator_type> example1_grammar;

    // now we use the types defined above to create the lexer and grammar
    // object instances needed to invoke the parsing process
    example1_lex lex;                             // Our lexer
    example1_grammar calc(lex);                   // Our grammar definition

    std::string str (read_from_file("example1.input"));

    // At this point we generate the iterator pair used to expose the
    // tokenized input stream.
    std::string::iterator it = str.begin();
    iterator_type iter = lex.begin(it, str.end());
    iterator_type end = lex.end();

    // Parsing is done based on the the token stream, not the character 
    // stream read from the input.
    // Note, how we use the token_def defined above as the skip parser. It must
    // be explicitly wrapped inside a state directive, switching the lexer 
    // state for the duration of skipping whitespace.
    bool r = qi::phrase_parse(iter, end, calc, qi::in_state("WS")[lex.white_space]);

    if (r && iter == end)
    {
        std::cout << "-------------------------\n";
        std::cout << "Parsing succeeded\n";
        std::cout << "-------------------------\n";
    }
    else
    {
        std::string rest(iter, end);
        std::cout << "-------------------------\n";
        std::cout << "Parsing failed\n";
        std::cout << "stopped at: \"" << rest << "\"\n";
        std::cout << "-------------------------\n";
    }

    std::cout << "Bye... :-) \n\n";
    return 0;
}
