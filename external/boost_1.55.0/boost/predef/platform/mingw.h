/*
Copyright Redshift Software, Inc. 2008-2013
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_COMPILER_MINGW_H
#define BOOST_PREDEF_COMPILER_MINGW_H

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

/*`
[heading `BOOST_PLAT_MINGW`]

[@http://en.wikipedia.org/wiki/MinGW MinGW] platform.
Version number available as major, minor, and patch.

[table
    [[__predef_symbol__] [__predef_version__]]

    [[`__MINGW32__`] [__predef_detection__]]
    [[`__MINGW64__`] [__predef_detection__]]

    [[`__MINGW64_VERSION_MAJOR`, `__MINGW64_VERSION_MINOR`] [V.R.0]]
    [[`__MINGW32_VERSION_MAJOR`, `__MINGW32_VERSION_MINOR`] [V.R.0]]
    ]
 */

#define BOOST_PLAT_MINGW BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if defined(__MINGW32__) || defined(__MINGW64__)
#   include <_mingw.h>
#   undef BOOST_PLAT_MINGW
#   if !defined(BOOST_PLAT_MINGW) && (defined(__MINGW64_VERSION_MAJOR) && defined(__MINGW64_VERSION_MINOR))
#       define BOOST_PLAT_MINGW \
            BOOST_VERSION_NUMBER(__MINGW64_VERSION_MAJOR,__MINGW64_VERSION_MINOR,0)
#   endif
#   if !defined(BOOST_PLAT_MINGW) && (defined(__MINGW32_VERSION_MAJOR) && defined(__MINGW32_VERSION_MINOR))
#       define BOOST_PLAT_MINGW \
            BOOST_VERSION_NUMBER(__MINGW32_MAJOR_VERSION,__MINGW32_MINOR_VERSION,0)
#   endif
#   if !defined(BOOST_PLAT_MINGW)
#       define BOOST_PLAT_MINGW BOOST_VERSION_NUMBER_AVAILABLE
#   endif
#endif

#if BOOST_PLAT_MINGW
#   define BOOST_PLAT_MINGW_AVAILABLE
#endif

#define BOOST_PLAT_MINGW_NAME "MinGW"

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_PLAT_MINGW,BOOST_PLAT_MINGW_NAME)


#endif
