/*
Copyright Redshift Software, Inc. 2008-2013
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_COMPILER_VISUALC_H
#define BOOST_PREDEF_COMPILER_VISUALC_H

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

/*`
[heading `BOOST_COMP_MSVC`]

[@http://en.wikipedia.org/wiki/Visual_studio Microsoft Visual C/C++] compiler.
Version number available as major, minor, and patch.

[table
    [[__predef_symbol__] [__predef_version__]]

    [[`_MSC_VER`] [__predef_detection__]]

    [[`_MSC_FULL_VER`] [V.R.P]]
    [[`_MSC_VER`] [V.R.0]]
    ]
 */

#define BOOST_COMP_MSVC BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if defined(_MSC_VER)
#   undef BOOST_COMP_MSVC
#   if !defined (_MSC_FULL_VER)
#       define BOOST_COMP_MSVC_BUILD 0
#   else
        /* how many digits does the build number have? */
#       if _MSC_FULL_VER / 10000 == _MSC_VER
            /* four digits */
#           define BOOST_COMP_MSVC_BUILD (_MSC_FULL_VER % 10000)
#       elif _MSC_FULL_VER / 100000 == _MSC_VER
            /* five digits */
#           define BOOST_COMP_MSVC_BUILD (_MSC_FULL_VER % 100000)
#       else
#           error "Cannot determine build number from _MSC_FULL_VER"
#       endif
#   endif
#   define BOOST_COMP_MSVC BOOST_VERSION_NUMBER(\
        _MSC_VER/100-6,\
        _MSC_VER%100,\
        BOOST_COMP_MSVC_BUILD)
#endif

#if BOOST_COMP_MSVC
#   define BOOST_COMP_MSVC_AVAILABLE
#endif

#define BOOST_COMP_MSVC_NAME "Microsoft Visual C/C++"

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_MSVC,BOOST_COMP_MSVC_NAME)


#endif
