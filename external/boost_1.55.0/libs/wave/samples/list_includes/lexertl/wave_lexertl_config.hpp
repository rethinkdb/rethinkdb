/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2010 Hartmut Kaiser. Distributed under the Boost 
    Software License, Version 1.0. (See accompanying file 
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if !defined(BOOST_WAVE_WAVE_LEXERTL_CONFIG_HPP_INCLUDED)
#define BOOST_WAVE_WAVE_LEXERTL_CONFIG_HPP_INCLUDED

///////////////////////////////////////////////////////////////////////////////
//  If the BOOST_WAVE_LEXERTL_USE_STATIC_TABLES constant is defined to be not 
//  equal to zero, the lexer will use static pre-compiled dfa tables (as 
//  included in the file: wave_lexertl_tables.hpp). Enabling the static tables
//  makes the code compilable even without having the lexertl library 
//  available.
#if !defined(BOOST_WAVE_LEXERTL_USE_STATIC_TABLES)
#define BOOST_WAVE_LEXERTL_USE_STATIC_TABLES 0
#endif

///////////////////////////////////////////////////////////////////////////////
//  If the dfa tables have to be generated at runtime, and the constant
//  BOOST_WAVE_LEXERTL_GENERATE_CPP_CODE is defined to be not equal to zero, 
//  the lexer will write C++ code for static DFA tables. This is useful for
//  generating the static tables required for the 
//  BOOST_WAVE_LEXERTL_USE_STATIC_TABLES as described above.
#if BOOST_WAVE_LEXERTL_USE_STATIC_TABLES != 0
#if !defined(BOOST_WAVE_LEXERTL_GENERATE_CPP_CODE)
#define BOOST_WAVE_LEXERTL_GENERATE_CPP_CODE 0
#endif
#endif

#endif // !BOOST_WAVE_WAVE_LEXERTL_CONFIG_HPP_INCLUDED
