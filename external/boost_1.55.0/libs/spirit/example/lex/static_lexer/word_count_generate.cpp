//  Copyright (c) 2001-2010 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  The purpose of this example is to show, how it is possible to use a lexer 
//  token definition for two purposes:
//
//    . To generate C++ code implementing a static lexical analyzer allowing
//      to recognize all defined tokens (this file)
//    . To integrate the generated C++ lexer into the /Spirit/ framework.
//      (see the file: word_count_static.cpp)

// #define BOOST_SPIRIT_LEXERTL_DEBUG

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/lex_lexertl.hpp>
#include <boost/spirit/include/lex_generate_static_lexertl.hpp>

#include <fstream>

#include "word_count_tokens.hpp"

using namespace boost::spirit;

///////////////////////////////////////////////////////////////////////////////
//[wc_static_generate_main
int main(int argc, char* argv[])
{
    // create the lexer object instance needed to invoke the generator
    word_count_tokens<lex::lexertl::lexer<> > word_count; // the token definition

    // open the output file, where the generated tokenizer function will be 
    // written to
    std::ofstream out(argc < 2 ? "word_count_static.hpp" : argv[1]);

    // invoke the generator, passing the token definition, the output stream 
    // and the name suffix of the tables and functions to be generated
    //
    // The suffix "wc" used below results in a type lexertl::static_::lexer_wc
    // to be generated, which needs to be passed as a template parameter to the 
    // lexertl::static_lexer template (see word_count_static.cpp).
    return lex::lexertl::generate_static_dfa(word_count, out, "wc") ? 0 : -1;
}
//]
