//  (C) Copyright Gennadiy Rozental 2001-2008.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//  File        : $RCSfile$
//
//  Version     : $Revision: 63441 $
//
//  Description : as a central place for global configuration switches
// ***************************************************************************

#ifndef BOOST_TEST_CONFIG_HPP_071894GER
#define BOOST_TEST_CONFIG_HPP_071894GER

// Boost
#include <boost/config.hpp> // compilers workarounds
#include <boost/detail/workaround.hpp>

//____________________________________________________________________________//

#if BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x570)) || \
    BOOST_WORKAROUND(__IBMCPP__, BOOST_TESTED_AT(600))     || \
    (defined __sgi && BOOST_WORKAROUND(_COMPILER_VERSION, BOOST_TESTED_AT(730)))
#  define BOOST_TEST_SHIFTED_LINE
#endif

//____________________________________________________________________________//

#if defined(BOOST_MSVC) || (defined(__BORLANDC__) && !defined(BOOST_DISABLE_WIN32))
#  define BOOST_TEST_CALL_DECL __cdecl
#else
#  define BOOST_TEST_CALL_DECL /**/
#endif

//____________________________________________________________________________//

#if !defined(BOOST_NO_STD_LOCALE) &&            \
    !BOOST_WORKAROUND(BOOST_MSVC, < 1310)  &&   \
    !defined(__MWERKS__) 
#  define BOOST_TEST_USE_STD_LOCALE 1
#endif

//____________________________________________________________________________//

#if BOOST_WORKAROUND(__BORLANDC__, <= 0x570)            || \
    BOOST_WORKAROUND( __COMO__, <= 0x433 )              || \
    BOOST_WORKAROUND( __INTEL_COMPILER, <= 800 )        || \
    defined(__sgi) && _COMPILER_VERSION <= 730          || \
    BOOST_WORKAROUND(__IBMCPP__, BOOST_TESTED_AT(600))  || \
    defined(__DECCXX)                                   || \
    defined(__DMC__)
#  define BOOST_TEST_NO_PROTECTED_USING
#endif

//____________________________________________________________________________//

#if defined(__GNUC__) || BOOST_WORKAROUND(BOOST_MSVC, == 1400)
#define BOOST_TEST_PROTECTED_VIRTUAL virtual
#else
#define BOOST_TEST_PROTECTED_VIRTUAL
#endif

//____________________________________________________________________________//

#if !BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x564)) && \
    !BOOST_WORKAROUND(BOOST_MSVC, <1310) && \
    !BOOST_WORKAROUND(__SUNPRO_CC, BOOST_TESTED_AT(0x530))
#  define BOOST_TEST_SUPPORT_INTERACTION_TESTING 1
#endif

//____________________________________________________________________________//

#if defined(BOOST_ALL_DYN_LINK) && !defined(BOOST_TEST_DYN_LINK)
#  define BOOST_TEST_DYN_LINK
#endif

#if defined(BOOST_TEST_INCLUDED)
#  undef BOOST_TEST_DYN_LINK
#endif

#if defined(BOOST_TEST_DYN_LINK)
#  define BOOST_TEST_ALTERNATIVE_INIT_API

#  ifdef BOOST_TEST_SOURCE
#    define BOOST_TEST_DECL BOOST_SYMBOL_EXPORT
#  else
#    define BOOST_TEST_DECL BOOST_SYMBOL_IMPORT
#  endif  // BOOST_TEST_SOURCE
#else
#  define BOOST_TEST_DECL
#endif

#if !defined(BOOST_TEST_MAIN) && defined(BOOST_AUTO_TEST_MAIN)
#define BOOST_TEST_MAIN BOOST_AUTO_TEST_MAIN
#endif

#if !defined(BOOST_TEST_MAIN) && defined(BOOST_TEST_MODULE)
#define BOOST_TEST_MAIN BOOST_TEST_MODULE
#endif

#endif // BOOST_TEST_CONFIG_HPP_071894GER
