/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#define BOOST_WAVE_SOURCE 1

// disable stupid compiler warnings
#include <boost/config/warning_disable.hpp>
#include <boost/wave/wave_config.hpp>

#if BOOST_WAVE_SEPARATE_GRAMMAR_INSTANTIATION != 0

#include <string>

#include <boost/wave/cpplexer/cpp_lex_token.hpp>
#include <boost/wave/cpplexer/cpp_lex_iterator.hpp>

#include <boost/wave/grammars/cpp_literal_grammar_gen.hpp>
#include <boost/wave/grammars/cpp_intlit_grammar.hpp>
#include <boost/wave/grammars/cpp_chlit_grammar.hpp>

// this must occur after all of the includes and before any code appears
#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_PREFIX
#endif

///////////////////////////////////////////////////////////////////////////////
//  
//  Explicit instantiation of the intlit_grammar_gen and chlit_grammar_gen 
//  templates with the correct token type. This instantiates the corresponding 
//  parse function, which in turn instantiates the corresponding parser object.
//
///////////////////////////////////////////////////////////////////////////////

typedef boost::wave::cpplexer::lex_token<> token_type;

// no need to change anything below
template struct boost::wave::grammars::intlit_grammar_gen<token_type>;
#if BOOST_WAVE_WCHAR_T_SIGNEDNESS == BOOST_WAVE_WCHAR_T_AUTOSELECT || \
    BOOST_WAVE_WCHAR_T_SIGNEDNESS == BOOST_WAVE_WCHAR_T_FORCE_SIGNED
template struct boost::wave::grammars::chlit_grammar_gen<int, token_type>;
#endif
template struct boost::wave::grammars::chlit_grammar_gen<unsigned int, token_type>;

// the suffix header occurs after all of the code
#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_SUFFIX
#endif

#endif // #if BOOST_WAVE_SEPARATE_GRAMMAR_INSTANTIATION != 0

