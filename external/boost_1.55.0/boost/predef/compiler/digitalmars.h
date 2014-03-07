/*
Copyright Redshift Software, Inc. 2008-2013
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_COMPILER_DIGITALMARS_H
#define BOOST_PREDEF_COMPILER_DIGITALMARS_H

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

/*`
[heading `BOOST_COMP_DMC`]

[@http://en.wikipedia.org/wiki/Digital_Mars Digital Mars] compiler.
Version number available as major, minor, and patch.

[table
    [[__predef_symbol__] [__predef_version__]]

    [[`__DMC__`] [__predef_detection__]]

    [[`__DMC__`] [V.R.P]]
    ]
 */

#define BOOST_COMP_DMC BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if defined(__DMC__)
#   undef BOOST_COMP_DMC
#   define BOOST_COMP_DMC BOOST_PREDEF_MAKE_0X_VRP(__DMC__)
#endif

#if BOOST_COMP_DMC
#   define BOOST_COMP_DMC_AVAILABLE
#endif

#define BOOST_COMP_DMC_NAME "Digital Mars"

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_DMC,BOOST_COMP_DMC_NAME)


#endif
