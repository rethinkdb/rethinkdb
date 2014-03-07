//  Copyright (c) 2001-2010 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  This example shows how to create a simple lexer recognizing a couple of 
//  different tokens and how to use this with a grammar. This example has a 
//  heavily backtracking grammar which makes it a candidate for lexer based 
//  parsing (all tokens are scanned and generated only once, even if 
//  backtracking is required) which speeds up the overall parsing process 
//  considerably, out-weighting the overhead needed for setting up the lexer.
//
//  Additionally, this example demonstrates, how to define a token set usable 
//  as the skip parser during parsing, allowing to define several tokens to be 
//  ignored.
//
//  This example recognizes couplets, which are sequences of numbers enclosed 
//  in matching pairs of parenthesis. See the comments below to for details
//  and examples.

// #define BOOST_SPIRIT_LEXERTL_DEBUG
// #define BOOST_SPIRIT_DEBUG

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
struct example3_tokens : lex::lexer<Lexer>
{
    example3_tokens()
    {
        // define the tokens to match
        ellipses = "\\.\\.\\.";
        number = "[0-9]+";

        // associate the tokens and the token set with the lexer
        this->self = ellipses | '(' | ')' | number;

        // define the whitespace to ignore (spaces, tabs, newlines and C-style 
        // comments)
        this->self("WS") 
            =   lex::token_def<>("[ \\t\\n]+")          // whitespace
            |   "\\/\\*[^*]*\\*+([^/*][^*]*\\*+)*\\/"   // C style comments
            ;
    }

    // these tokens expose the iterator_range of the matched input sequence
    lex::token_def<> ellipses, identifier, number;
};

///////////////////////////////////////////////////////////////////////////////
//  Grammar definition
///////////////////////////////////////////////////////////////////////////////
template <typename Iterator, typename Lexer>
struct example3_grammar 
  : qi::grammar<Iterator, qi::in_state_skipper<Lexer> >
{
    template <typename TokenDef>
    example3_grammar(TokenDef const& tok)
      : example3_grammar::base_type(start)
    {
        start 
            =  +(couplet | tok.ellipses)
            ;

        //  A couplet matches nested left and right parenthesis.
        //  For example:
        //    (1) (1 2) (1 2 3) ...
        //    ((1)) ((1 2)(3 4)) (((1) (2 3) (1 2 (3) 4))) ...
        //    (((1))) ...
        couplet
            =   tok.number
            |   '(' >> +couplet >> ')'
            ;

        BOOST_SPIRIT_DEBUG_NODE(start);
        BOOST_SPIRIT_DEBUG_NODE(couplet);
    }

    qi::rule<Iterator, qi::in_state_skipper<Lexer> > start, couplet;
};

///////////////////////////////////////////////////////////////////////////////
int main()
{
    // iterator type used to expose the underlying input stream
    typedef std::string::iterator base_iterator_type;

    // This is the token type to return from the lexer iterator
    typedef lex::lexertl::token<base_iterator_type> token_type;

    // This is the lexer type to use to tokenize the input.
    // Here we use the lexertl based lexer engine.
    typedef lex::lexertl::lexer<token_type> lexer_type;

    // This is the token definition type (derived from the given lexer type).
    typedef example3_tokens<lexer_type> example3_tokens;

    // this is the iterator type exposed by the lexer 
    typedef example3_tokens::iterator_type iterator_type;

    // this is the type of the grammar to parse
    typedef example3_grammar<iterator_type, example3_tokens::lexer_def> example3_grammar;

    // now we use the types defined above to create the lexer and grammar
    // object instances needed to invoke the parsing process
    example3_tokens tokens;                         // Our lexer
    example3_grammar calc(tokens);                  // Our parser

    std::string str (read_from_file("example3.input"));

    // At this point we generate the iterator pair used to expose the
    // tokenized input stream.
    std::string::iterator it = str.begin();
    iterator_type iter = tokens.begin(it, str.end());
    iterator_type end = tokens.end();

    // Parsing is done based on the the token stream, not the character 
    // stream read from the input.
    // Note how we use the lexer defined above as the skip parser.
    bool r = qi::phrase_parse(iter, end, calc, qi::in_state("WS")[tokens.self]);

    if (r && iter == end)
    {
        std::cout << "-------------------------\n";
        std::cout << "Parsing succeeded\n";
        std::cout << "-------------------------\n";
    }
    else
    {
        std::cout << "-------------------------\n";
        std::cout << "Parsing failed\n";
        std::cout << "-------------------------\n";
    }

    std::cout << "Bye... :-) \n\n";
    return 0;
}
