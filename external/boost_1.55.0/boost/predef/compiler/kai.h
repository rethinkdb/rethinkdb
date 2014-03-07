/*
Copyright Redshift Software, Inc. 2008-2013
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_COMPILER_KAI_H
#define BOOST_PREDEF_COMPILER_KAI_H

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

/*`
[heading `BOOST_COMP_KCC`]

Kai C++ compiler.
Version number available as major, minor, and patch.

[table
    [[__predef_symbol__] [__predef_version__]]

    [[`__KCC`] [__predef_detection__]]

    [[`__KCC_VERSION`] [V.R.P]]
    ]
 */

#define BOOST_COMP_KCC BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if defined(__KCC)
#   undef BOOST_COMP_KCC
#   define BOOST_COMP_KCC BOOST_PREDEF_MAKE_0X_VRPP(__KCC_VERSION)
#endif

#if BOOST_COMP_KCC
#   define BOOST_COMP_KCC_AVAILABLE
#endif

#define BOOST_COMP_KCC_NAME "Kai C++"

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_KCC,BOOST_COMP_KCC_NAME)


#endif
