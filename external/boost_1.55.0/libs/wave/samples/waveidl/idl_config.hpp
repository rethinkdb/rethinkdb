/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library

    Sample: IDL oriented preprocessor
            Global application configuration of the Wave driver command
    
    http://www.boost.org/

    Copyright (c) 2001-2010 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if !defined(IDL_CONFIG_HPP_012D7524_FF3F_482F_9123_91966C72F4EA_INCLUDED)
#define IDL_CONFIG_HPP_012D7524_FF3F_482F_9123_91966C72F4EA_INCLUDED

///////////////////////////////////////////////////////////////////////////////
//  Uncomment the following, if you need debug output, the 
//  BOOST_SPIRIT_DEBUG_FLAGS constants below help to fine control the amount of 
//  the generated debug output
//#define BOOST_SPIRIT_DEBUG

///////////////////////////////////////////////////////////////////////////////
//  debug rules, subrules and grammars only, for possible flags see 
//  spirit/debug.hpp
#if defined(BOOST_SPIRIT_DEBUG)

#define BOOST_SPIRIT_DEBUG_FLAGS ( \
        BOOST_SPIRIT_DEBUG_FLAGS_NODES | \
        BOOST_SPIRIT_DEBUG_FLAGS_CLOSURES \
    ) \
    /**/

///////////////////////////////////////////////////////////////////////////////
//  debug flags for the pp-iterator library, possible flags (defined in 
//  wave_config.hpp):
//
//  #define BOOST_SPIRIT_DEBUG_FLAGS_CPP_GRAMMAR            0x0001
//  #define BOOST_SPIRIT_DEBUG_FLAGS_TIME_CONVERSION        0x0002
//  #define BOOST_SPIRIT_DEBUG_FLAGS_CPP_EXPR_GRAMMAR       0x0004
//  #define BOOST_SPIRIT_DEBUG_FLAGS_INTLIT_GRAMMAR         0x0008
//  #define BOOST_SPIRIT_DEBUG_FLAGS_CHLIT_GRAMMAR          0x0010
//  #define BOOST_SPIRIT_DEBUG_FLAGS_DEFINED_GRAMMAR        0x0020
//  #define BOOST_SPIRIT_DEBUG_FLAGS_PREDEF_MACROS_GRAMMAR  0x0040

#define BOOST_SPIRIT_DEBUG_FLAGS_CPP (\
        /* insert the required flags from above */ \
    ) \
    /**/
#endif 

///////////////////////////////////////////////////////////////////////////////
//  Now include the cofiguration stuff for the Wave library itself
#include <boost/wave/wave_config.hpp>

///////////////////////////////////////////////////////////////////////////////
//  MSVC specific #pragma's
#if defined(BOOST_MSVC)
#pragma warning (disable: 4355) // 'this' used in base member initializer list
#pragma warning (disable: 4800) // forcing value to bool 'true' or 'false'
#pragma inline_depth(255)
#pragma inline_recursion(on)
#endif // defined(BOOST_MSVC)

#endif // !defined(IDL_CONFIG_HPP_012D7524_FF3F_482F_9123_91966C72F4EA_INCLUDED)
