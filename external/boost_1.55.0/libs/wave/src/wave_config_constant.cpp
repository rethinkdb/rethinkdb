/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    Persistent application configuration
    
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#define BOOST_WAVE_SOURCE 1

// disable stupid compiler warnings
#include <boost/config/warning_disable.hpp>

#include <cstring>
#include <boost/preprocessor/stringize.hpp>

#include <boost/wave/wave_config.hpp>
#include <boost/wave/wave_config_constant.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace wave {

    ///////////////////////////////////////////////////////////////////////////
    //  Call this function to test the configuration of the calling application
    //  against the configuration of the linked library.
    BOOST_WAVE_DECL bool 
    test_configuration(unsigned int config, char const* pragma_keyword, 
        char const* string_type_str)
    {
        if (NULL == pragma_keyword || NULL == string_type_str)
            return false;

        using namespace std;;   // some systems have strcmp in namespace std
        if (config != BOOST_WAVE_CONFIG ||
            strcmp(pragma_keyword, BOOST_WAVE_PRAGMA_KEYWORD) ||
            strcmp(string_type_str, BOOST_PP_STRINGIZE((BOOST_WAVE_STRINGTYPE))))
        {
            return false;
        } 
        return true;
    }

///////////////////////////////////////////////////////////////////////////////
}}  // namespace boost::wave

