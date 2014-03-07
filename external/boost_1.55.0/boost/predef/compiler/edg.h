/*
Copyright Redshift Software, Inc. 2008-2013
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_COMPILER_EDG_H
#define BOOST_PREDEF_COMPILER_EDG_H

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

/*`
[heading `BOOST_COMP_EDG`]

[@http://en.wikipedia.org/wiki/Edison_Design_Group EDG C++ Frontend] compiler.
Version number available as major, minor, and patch.

[table
    [[__predef_symbol__] [__predef_version__]]

    [[`__EDG__`] [__predef_detection__]]

    [[`__EDG_VERSION__`] [V.R.0]]
    ]
 */

#define BOOST_COMP_EDG BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if defined(__EDG__)
#   undef BOOST_COMP_EDG
#   define BOOST_COMP_EDG BOOST_PREDEF_MAKE_10_VRR(__EDG_VERSION__)
#endif

#if BOOST_COMP_EDG
#   define BOOST_COMP_EDG_AVAILABLE
#endif

#define BOOST_COMP_EDG_NAME "EDG C++ Frontend"

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_EDG,BOOST_COMP_EDG_NAME)


#endif
