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

// #define BOOST_SPIRIT_DEBUG
// #define BOOST_SPIRIT_LEXERTL_DEBUG

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/lex_static_lexertl.hpp>

#include <iostream>
#include <string>

#include "../example.hpp"
#include "word_count_lexer_tokens.hpp"          // token definition
#include "word_count_lexer_static.hpp"          // generated tokenizer

using namespace boost::spirit;

///////////////////////////////////////////////////////////////////////////////
//[wcl_static_main
int main(int argc, char* argv[])
{
    // read input from the given file
    std::string str (read_from_file(1 == argc ? "word_count.input" : argv[1]));

    // Specifying 'omit' as the token attribute type generates a token class 
    // notholding any token attribute at all (not even the iterator_range of the 
    // matched input sequence), therefor optimizing the token, the lexer, and 
    // possibly the parser implementation as much as possible. 
    //
    // Specifying mpl::false_ as the 3rd template parameter generates a token
    // type and an iterator, both holding no lexer state, allowing for even more 
    // aggressive optimizations.
    //
    // As a result the token instances contain the token ids as the only data 
    // member.
    typedef lex::lexertl::token<char const*, lex::omit, boost::mpl::false_> token_type;

    // Define the lexer type to be used as the base class for our token 
    // definition.
    //
    // This is the only place where the code is different from an equivalent
    // dynamic lexical analyzer. We use the `lexertl::static_lexer<>` instead of
    // the `lexertl::lexer<>` as the base class for our token defintion type.
    //
    // As we specified the suffix "wcl" while generating the static tables we 
    // need to pass the type lexertl::static_::lexer_wcl as the second template
    // parameter below (see word_count_lexer_generate.cpp).
    typedef lex::lexertl::static_actor_lexer<
        token_type, lex::lexertl::static_::lexer_wcl
    > lexer_type;

    // create the lexer object instance needed to invoke the lexical analysis 
    word_count_lexer_tokens<lexer_type> word_count_lexer;

    // tokenize the given string, all generated tokens are discarded
    char const* first = str.c_str();
    char const* last = &first[str.size()];
    bool r = lex::tokenize(first, last, word_count_lexer);

    if (r) {
        std::cout << "lines: " << word_count_lexer.l 
                  << ", words: " << word_count_lexer.w 
                  << ", characters: " << word_count_lexer.c 
                  << "\n";
    }
    else {
        std::string rest(first, last);
        std::cout << "Lexical analysis failed\n" << "stopped at: \"" 
                  << rest << "\"\n";
    }
    return 0;
}
//]
