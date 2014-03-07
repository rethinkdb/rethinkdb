/*
Copyright Redshift Software, Inc. 2008-2013
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_COMPILER_PALM_H
#define BOOST_PREDEF_COMPILER_PALM_H

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

/*`
[heading `BOOST_COMP_PALM`]

Palm C/C++ compiler.
Version number available as major, minor, and patch.

[table
    [[__predef_symbol__] [__predef_version__]]

    [[`_PACC_VER`] [__predef_detection__]]

    [[`_PACC_VER`] [V.R.P]]
    ]
 */

#define BOOST_COMP_PALM BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if defined(_PACC_VER)
#   undef BOOST_COMP_PALM
#   define BOOST_COMP_PALM BOOST_PREDEF_MAKE_0X_VRRPP000(_PACC_VER)
#endif

#if BOOST_COMP_PALM
#   define BOOST_COMP_PALM_AVAILABLE
#endif

#define BOOST_COMP_PALM_NAME "Palm C/C++"

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_PALM,BOOST_COMP_PALM_NAME)


#endif
