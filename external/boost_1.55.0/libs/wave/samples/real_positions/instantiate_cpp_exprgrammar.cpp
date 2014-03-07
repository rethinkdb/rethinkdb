/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    Example: real_positions

    Explicit instantiation of the cpp_expression_grammar parsing function

    http://www.boost.org/

    Copyright (c) 2001-2010 Hartmut Kaiser. Distributed under the Boost 
    Software License, Version 1.0. (See accompanying file 
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#include <boost/wave/wave_config.hpp>

#if BOOST_WAVE_SEPARATE_GRAMMAR_INSTANTIATION != 0

#include <string>

#include <boost/wave/token_ids.hpp>

#include "real_position_token.hpp"                    // token class
#include <boost/wave/cpplexer/cpp_lex_iterator.hpp>   // lexer type

#include <boost/wave/grammars/cpp_expression_grammar.hpp>

///////////////////////////////////////////////////////////////////////////////
//  
//  Explicit instantiation of the expression_grammar_gen template with the 
//  correct token type. This instantiates the corresponding parse function, 
//  which in turn instantiates the expression_grammar object (see 
//  wave/grammars/cpp_expression_grammar.hpp)
//
///////////////////////////////////////////////////////////////////////////////

typedef lex_token<> token_type;

template struct boost::wave::grammars::expression_grammar_gen<token_type>;

#endif // #if BOOST_WAVE_SEPARATE_GRAMMAR_INSTANTIATION != 0

