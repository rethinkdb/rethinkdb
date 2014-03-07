/*
Copyright Redshift Software, Inc. 2008-2013
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_COMPILER_CLANG_H
#define BOOST_PREDEF_COMPILER_CLANG_H

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

/*`
[heading `BOOST_COMP_CLANG`]

[@http://en.wikipedia.org/wiki/Clang Clang] compiler.
Version number available as major, minor, and patch.

[table
    [[__predef_symbol__] [__predef_version__]]

    [[`__clang__`] [__predef_detection__]]

    [[`__clang_major__`, `__clang_minor__`, `__clang_patchlevel__`] [V.R.P]]
    ]
 */

#define BOOST_COMP_CLANG BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if defined(__clang__)
#   undef BOOST_COMP_CLANG
#   define BOOST_COMP_CLANG BOOST_VERSION_NUMBER(__clang_major__,__clang_minor__,__clang_patchlevel__)
#endif

#if BOOST_COMP_CLANG
#   define BOOST_COMP_CLANG_AVAILABLE
#endif

#define BOOST_COMP_CLANG_NAME "Clang"

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_CLANG,BOOST_COMP_CLANG_NAME)


#endif
