/*
Copyright Redshift Software, Inc. 2008-2013
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_COMPILER_DIGNUS_H
#define BOOST_PREDEF_COMPILER_DIGNUS_H

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

/*`
[heading `BOOST_COMP_SYSC`]

[@http://www.dignus.com/dcxx/ Dignus Systems/C++] compiler.
Version number available as major, minor, and patch.

[table
    [[__predef_symbol__] [__predef_version__]]

    [[`__SYSC__`] [__predef_detection__]]

    [[`__SYSC_VER__`] [V.R.P]]
    ]
 */

#define BOOST_COMP_SYSC BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if defined(__SYSC__)
#   undef BOOST_COMP_SYSC
#   define BOOST_COMP_SYSC BOOST_PREDEF_MAKE_10_VRRPP(__SYSC_VER__)
#endif

#if BOOST_COMP_SYSC
#   define BOOST_COMP_SYSC_AVAILABLE
#endif

#define BOOST_COMP_SYSC_NAME "Dignus Systems/C++"

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_SYSC,BOOST_COMP_SYSC_NAME)


#endif
