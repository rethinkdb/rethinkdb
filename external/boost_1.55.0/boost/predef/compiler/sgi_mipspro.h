/*
Copyright Redshift Software, Inc. 2008-2013
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_COMPILER_SGI_MIPSPRO_H
#define BOOST_PREDEF_COMPILER_SGI_MIPSPRO_H

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

/*`
[heading `BOOST_COMP_SGI`]

[@http://en.wikipedia.org/wiki/MIPSpro SGI MIPSpro] compiler.
Version number available as major, minor, and patch.

[table
    [[__predef_symbol__] [__predef_version__]]

    [[`__sgi`] [__predef_detection__]]
    [[`sgi`] [__predef_detection__]]

    [[`_SGI_COMPILER_VERSION`] [V.R.P]]
    [[`_COMPILER_VERSION`] [V.R.P]]
    ]
 */

#define BOOST_COMP_SGI BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if defined(__sgi) || defined(sgi)
#   undef BOOST_COMP_SGI
#   if !defined(BOOST_COMP_SGI) && defined(_SGI_COMPILER_VERSION)
#       define BOOST_COMP_SGI BOOST_PREDEF_MAKE_10_VRP(_SGI_COMPILER_VERSION)
#   endif
#   if !defined(BOOST_COMP_SGI) && defined(_COMPILER_VERSION)
#       define BOOST_COMP_SGI BOOST_PREDEF_MAKE_10_VRP(_COMPILER_VERSION)
#   endif
#   if !defined(BOOST_COMP_SGI)
#       define BOOST_COMP_SGI BOOST_VERSION_NUMBER_AVAILABLE
#   endif
#endif

#if BOOST_COMP_SGI
#   define BOOST_COMP_SGI_AVAILABLE
#endif

#define BOOST_COMP_SGI_NAME "SGI MIPSpro"

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_SGI,BOOST_COMP_SGI_NAME)


#endif
