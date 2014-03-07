/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library

    Sample: Collect token statistics from the analysed files
        
    http://www.boost.org/

    Copyright (c) 2001-2010 Hartmut Kaiser. Distributed under the Boost 
    Software License, Version 1.0. (See accompanying file 
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if !defined(TOKEN_STATISTICS_HPP)
#define TOKEN_STATISTICS_HPP

///////////////////////////////////////////////////////////////////////////////
//  include often used files from the stdlib
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
//  include boost config
#include <boost/config.hpp>     //  global configuration information
#include <boost/assert.hpp>

///////////////////////////////////////////////////////////////////////////////
//  build version
#include "token_statistics_version.hpp"

///////////////////////////////////////////////////////////////////////////////
//  Now include the configuration stuff for the Wave library itself
#include <boost/wave/wave_config.hpp>

///////////////////////////////////////////////////////////////////////////////
//  MSVC specific #pragma's
#if defined(BOOST_MSVC)
#pragma warning (disable: 4355) // 'this' used in base member initializer list
#pragma warning (disable: 4800) // forcing value to bool 'true' or 'false'
#pragma inline_depth(255)
#pragma inline_recursion(on)
#endif // defined(BOOST_MSVC)

///////////////////////////////////////////////////////////////////////////////
//  include required boost libraries
#include <boost/pool/pool_alloc.hpp>

#endif // !defined(TOKEN_STATISTICS_HPP)
