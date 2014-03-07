/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library

    Sample: Explicit instantiation of the xlex_functor generation function
    
    http://www.boost.org/

    Copyright (c) 2001-2010 Hartmut Kaiser. Distributed under the Boost 
    Software License, Version 1.0. (See accompanying file 
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#include <boost/wave.hpp>

#if BOOST_WAVE_SEPARATE_LEXER_INSTANTIATION != 0

#include <string>

#include <boost/wave/token_ids.hpp>

#include <boost/wave/cpplexer/cpp_lex_token.hpp>
#include "xlex_iterator.hpp"

///////////////////////////////////////////////////////////////////////////////
//  The following file needs to be included only once throughout the whole
//  program.
#include "xlex/xlex_lexer.hpp"

///////////////////////////////////////////////////////////////////////////////
//
//  This instantiates the correct 'new_lexer' function, which generates the 
//  C++ lexer used in this sample.
//
//  This is moved into a separate compilation unit to decouple the compilation
//  of the C++ lexer from the compilation of the other modules, which helps to 
//  reduce compilation time.
//
//  The template parameter(s) supplied should be identical to the parameters
//  supplied while instantiating the context<> template.
//
///////////////////////////////////////////////////////////////////////////////

template struct boost::wave::cpplexer::xlex::new_lexer_gen<std::string::iterator>;

#endif // BOOST_WAVE_SEPARATE_LEXER_INSTANTIATION != 0
