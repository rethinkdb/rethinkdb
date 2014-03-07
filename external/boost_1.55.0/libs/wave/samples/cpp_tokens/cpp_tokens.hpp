/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    
    Sample: Print out the preprocessed tokens returned by the Wave iterator

    This sample shows, how it is possible to use a custom lexer object and a 
    custom token type with the Wave library. 
    
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost 
    Software License, Version 1.0. (See accompanying file 
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if !defined(CPP_TOKENS_HPP_D6A31137_CE14_4869_9779_6357E2C43187_INCLUDED)
#define CPP_TOKENS_HPP_D6A31137_CE14_4869_9779_6357E2C43187_INCLUDED

///////////////////////////////////////////////////////////////////////////////
//  include often used files from the stdlib
#include <iostream>
#include <fstream>
#include <string>

///////////////////////////////////////////////////////////////////////////////
//  include boost config
#include <boost/config.hpp>     //  global configuration information

///////////////////////////////////////////////////////////////////////////////
//  configure this app here (global configuration constants)
#include "cpp_tokens_config.hpp"

///////////////////////////////////////////////////////////////////////////////
//  include required boost libraries
#include <boost/assert.hpp>
#include <boost/pool/pool_alloc.hpp>

#endif // !defined(CPP_TOKENS_HPP_D6A31137_CE14_4869_9779_6357E2C43187_INCLUDED)
