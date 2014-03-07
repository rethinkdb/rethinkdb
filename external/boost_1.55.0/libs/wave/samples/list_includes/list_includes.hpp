/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library

    Sample: List include dependencies of a given source file
            Configuration data
        
    http://www.boost.org/

    Copyright (c) 2001-2010 Hartmut Kaiser. Distributed under the Boost 
    Software License, Version 1.0. (See accompanying file 
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if !defined(LIST_INCLUDES_HPP_843DB412_3AA8_4BCF_8081_AA4A5FDE0BE7_INCLUDED)
#define LIST_INCLUDES_HPP_843DB412_3AA8_4BCF_8081_AA4A5FDE0BE7_INCLUDED

///////////////////////////////////////////////////////////////////////////////
//  include often used files from the stdlib
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <set>

///////////////////////////////////////////////////////////////////////////////
//  include boost config
#include <boost/config.hpp>     //  global configuration information

///////////////////////////////////////////////////////////////////////////////
//  build version
#include "list_includes_version.hpp"

///////////////////////////////////////////////////////////////////////////////
//  configure this app here (global configuration constants)
#include "list_includes_config.hpp"

///////////////////////////////////////////////////////////////////////////////
//  include required boost libraries
#include <boost/assert.hpp>
#include <boost/pool/pool_alloc.hpp>

#endif // !defined(LIST_INCLUDES_HPP_843DB412_3AA8_4BCF_8081_AA4A5FDE0BE7_INCLUDED)
