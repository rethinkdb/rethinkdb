/*
Copyright Redshift Software, Inc. 2008-2013
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_COMPILER_GCC_H
#define BOOST_PREDEF_COMPILER_GCC_H

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

/*`
[heading `BOOST_COMP_GNUC`]

[@http://en.wikipedia.org/wiki/GNU_Compiler_Collection Gnu GCC C/C++] compiler.
Version number available as major, minor, and patch (if available).

[table
    [[__predef_symbol__] [__predef_version__]]

    [[`__GNUC__`] [__predef_detection__]]

    [[`__GNUC__`, `__GNUC_MINOR__`, `__GNUC_PATCHLEVEL__`] [V.R.P]]
    [[`__GNUC__`, `__GNUC_MINOR__`] [V.R.0]]
    ]
 */

#define BOOST_COMP_GNUC BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if defined(__GNUC__)
#   undef BOOST_COMP_GNUC
#   if !defined(BOOST_COMP_GNUC) && defined(__GNUC_PATCHLEVEL__)
#       define BOOST_COMP_GNUC \
            BOOST_VERSION_NUMBER(__GNUC__,__GNUC_MINOR__,__GNUC_PATCHLEVEL__)
#   endif
#   if !defined(BOOST_COMP_GNUC)
#       define BOOST_COMP_GNUC \
            BOOST_VERSION_NUMBER(__GNUC__,__GNUC_MINOR__,0)
#   endif
#endif

#if BOOST_COMP_GNUC
#   define BOOST_COMP_GNUC_AVAILABLE
#endif

#define BOOST_COMP_GNUC_NAME "Gnu GCC C/C++"

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_GNUC,BOOST_COMP_GNUC_NAME)


#endif
