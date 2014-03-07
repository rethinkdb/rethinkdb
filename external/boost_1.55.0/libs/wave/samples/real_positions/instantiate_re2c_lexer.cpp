/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    Explicit instantiation of the lex_functor generation function

    http://www.boost.org/

    Copyright (c) 2001-2010 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#include <boost/wave/wave_config.hpp>                 // configuration data

#if BOOST_WAVE_SEPARATE_LEXER_INSTANTIATION != 0

#include <string>

#include <boost/wave/token_ids.hpp>

#include "real_position_token.hpp"                    // token class
#include <boost/wave/cpplexer/cpp_lex_iterator.hpp>   // lexer type

///////////////////////////////////////////////////////////////////////////////
//  The following file needs to be included only once throughout the whole
//  program.
#include <boost/wave/cpplexer/re2clex/cpp_re2c_lexer.hpp>

// this must occur after all of the includes and before any code appears
#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_PREFIX
#endif

///////////////////////////////////////////////////////////////////////////////
//
//  This instantiates the correct 'new_lexer' function, which generates the 
//  C++ lexer used in this sample. You will have to instantiate the 
//  new_lexer_gen<> template with the same iterator type, as you have used for
//  instantiating the boost::wave::context<> object.
//
//  This is moved into a separate compilation unit to decouple the compilation
//  of the C++ lexer from the compilation of the other modules, which helps to 
//  reduce compilation time.
//
//  The template parameter(s) supplied should be identical to the first 
//  parameter supplied while instantiating the boost::wave::context<> template 
//  (see the file cpp.cpp).
//
///////////////////////////////////////////////////////////////////////////////

template struct boost::wave::cpplexer::new_lexer_gen<
    BOOST_WAVE_STRINGTYPE::iterator, boost::wave::util::file_position_type,
    lex_token<boost::wave::util::file_position_type> >;
template struct boost::wave::cpplexer::new_lexer_gen<
    BOOST_WAVE_STRINGTYPE::const_iterator, boost::wave::util::file_position_type,
    lex_token<boost::wave::util::file_position_type> >;

// the suffix header occurs after all of the code
#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_WAVE_SEPARATE_LEXER_INSTANTIATION != 0
