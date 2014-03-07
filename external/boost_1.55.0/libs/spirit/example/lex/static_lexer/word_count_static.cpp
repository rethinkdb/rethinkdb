//  Copyright (c) 2001-2010 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  The purpose of this example is to show, how it is possible to use a lexer 
//  token definition for two purposes:
//
//    . To generate C++ code implementing a static lexical analyzer allowing
//      to recognize all defined tokens 
//    . To integrate the generated C++ lexer into the /Spirit/ framework.
//

// #define BOOST_SPIRIT_LEXERTL_DEBUG
#define BOOST_VARIANT_MINIMIZE_SIZE

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
//[wc_static_include
#include <boost/spirit/include/lex_static_lexertl.hpp>
//]
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_statement.hpp>
#include <boost/spirit/include/phoenix_container.hpp>

#include <iostream>
#include <string>

#include "../example.hpp"
#include "word_count_tokens.hpp"          // token definition
#include "word_count_static.hpp"          // generated tokenizer

using namespace boost::spirit;
using namespace boost::spirit::ascii;

///////////////////////////////////////////////////////////////////////////////
//  Grammar definition
///////////////////////////////////////////////////////////////////////////////
//[wc_static_grammar
//  This is an ordinary grammar definition following the rules defined by 
//  Spirit.Qi. There is nothing specific about it, except it gets the token
//  definition class instance passed to the constructor to allow accessing the
//  embedded token_def<> instances.
template <typename Iterator>
struct word_count_grammar : qi::grammar<Iterator>
{
    template <typename TokenDef>
    word_count_grammar(TokenDef const& tok)
      : word_count_grammar::base_type(start)
      , c(0), w(0), l(0)
    {
        using boost::phoenix::ref;
        using boost::phoenix::size;

        //  associate the defined tokens with the lexer, at the same time 
        //  defining the actions to be executed 
        start =  *(   tok.word          [ ++ref(w), ref(c) += size(_1) ]
                  |   lit('\n')         [ ++ref(l), ++ref(c) ] 
                  |   qi::token(IDANY)  [ ++ref(c) ]
                  )
              ;
    }

    std::size_t c, w, l;      // counter for characters, words, and lines
    qi::rule<Iterator> start;
};
//]

///////////////////////////////////////////////////////////////////////////////
//[wc_static_main
int main(int argc, char* argv[])
{
    // Define the token type to be used: 'std::string' is available as the type 
    // of the token value.
    typedef lex::lexertl::token<
        char const*, boost::mpl::vector<std::string>
    > token_type;

    // Define the lexer type to be used as the base class for our token 
    // definition.
    //
    // This is the only place where the code is different from an equivalent
    // dynamic lexical analyzer. We use the `lexertl::static_lexer<>` instead of
    // the `lexertl::lexer<>` as the base class for our token defintion type.
    //
    // As we specified the suffix "wc" while generating the static tables we 
    // need to pass the type lexertl::static_::lexer_wc as the second template
    // parameter below (see word_count_generate.cpp).
    typedef lex::lexertl::static_lexer<
        token_type, lex::lexertl::static_::lexer_wc
    > lexer_type;

    // Define the iterator type exposed by the lexer.
    typedef word_count_tokens<lexer_type>::iterator_type iterator_type;

    // Now we use the types defined above to create the lexer and grammar
    // object instances needed to invoke the parsing process.
    word_count_tokens<lexer_type> word_count;           // Our lexer
    word_count_grammar<iterator_type> g (word_count);   // Our parser

    // Read in the file into memory.
    std::string str (read_from_file(1 == argc ? "word_count.input" : argv[1]));
    char const* first = str.c_str();
    char const* last = &first[str.size()];

    // Parsing is done based on the the token stream, not the character stream.
    bool r = lex::tokenize_and_parse(first, last, word_count, g);

    if (r) {    // success
        std::cout << "lines: " << g.l << ", words: " << g.w 
                  << ", characters: " << g.c << "\n";
    }
    else {
        std::string rest(first, last);
        std::cerr << "Parsing failed\n" << "stopped at: \"" 
                  << rest << "\"\n";
    }
    return 0;
}
//]
