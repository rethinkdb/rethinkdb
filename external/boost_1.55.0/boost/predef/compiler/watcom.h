/*
Copyright Redshift Software, Inc. 2008-2013
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_COMPILER_WATCOM_H
#define BOOST_PREDEF_COMPILER_WATCOM_H

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

/*`
[heading `BOOST_COMP_WATCOM`]

[@http://en.wikipedia.org/wiki/Watcom Watcom C++] compiler.
Version number available as major, and minor.

[table
    [[__predef_symbol__] [__predef_version__]]

    [[`__WATCOMC__`] [__predef_detection__]]

    [[`__WATCOMC__`] [V.R.P]]
    ]
 */

#define BOOST_COMP_WATCOM BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if defined(__WATCOMC__)
#   undef BOOST_COMP_WATCOM
#   define BOOST_COMP_WATCOM BOOST_PREDEF_MAKE_10_VVRR(__WATCOMC__)
#endif

#if BOOST_COMP_WATCOM
#   define BOOST_COMP_WATCOM_AVAILABLE
#endif

#define BOOST_COMP_WATCOM_NAME "Watcom C++"

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_WATCOM,BOOST_COMP_WATCOM_NAME)


#endif
