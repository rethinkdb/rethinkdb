/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if !defined(CPP_VERSION_HPP_CE4FE67F_63F9_468D_8364_C855F89D3C5D_INCLUDED)
#define CPP_VERSION_HPP_CE4FE67F_63F9_468D_8364_C855F89D3C5D_INCLUDED

#include <boost/wave/wave_version.hpp>

#define CPP_VERSION_MAJOR           BOOST_WAVE_VERSION_MAJOR
#define CPP_VERSION_MINOR           BOOST_WAVE_VERSION_MINOR
#define CPP_VERSION_SUBMINOR        BOOST_WAVE_VERSION_SUBMINOR
#define CPP_VERSION_FULL            BOOST_WAVE_VERSION

#define CPP_VERSION_FULL_STR        BOOST_PP_STRINGIZE(CPP_VERSION_FULL)

#define CPP_VERSION_DATE            20120523L
#define CPP_VERSION_DATE_STR        "20120523"

#endif // !defined(CPP_VERSION_HPP_CE4FE67F_63F9_468D_8364_C855F89D3C5D_INCLUDED)
