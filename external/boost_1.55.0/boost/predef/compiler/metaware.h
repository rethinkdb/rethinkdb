/*
Copyright Redshift Software, Inc. 2008-2013
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_COMPILER_METAWARE_H
#define BOOST_PREDEF_COMPILER_METAWARE_H

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

/*`
[heading `BOOST_COMP_HIGHC`]

MetaWare High C/C++ compiler.

[table
    [[__predef_symbol__] [__predef_version__]]

    [[`__HIGHC__`] [__predef_detection__]]
    ]
 */

#define BOOST_COMP_HIGHC BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if defined(__HIGHC__)
#   undef BOOST_COMP_HIGHC
#   define BOOST_COMP_HIGHC BOOST_VERSION_NUMBER_AVAILABLE
#endif

#if BOOST_COMP_HIGHC
#   define BOOST_COMP_HIGHC_AVAILABLE
#endif

#define BOOST_COMP_HIGHC_NAME "MetaWare High C/C++"

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_HIGHC,BOOST_COMP_HIGHC_NAME)


#endif
