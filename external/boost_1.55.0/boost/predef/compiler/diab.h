/*
Copyright Redshift Software, Inc. 2008-2013
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_COMPILER_DIAB_H
#define BOOST_PREDEF_COMPILER_DIAB_H

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

/*`
[heading `BOOST_COMP_DIAB`]

[@http://www.windriver.com/products/development_suite/wind_river_compiler/ Diab C/C++] compiler.
Version number available as major, minor, and patch.

[table
    [[__predef_symbol__] [__predef_version__]]

    [[`__DCC__`] [__predef_detection__]]

    [[`__VERSION_NUMBER__`] [V.R.P]]
    ]
 */

#define BOOST_COMP_DIAB BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if defined(__DCC__)
#   undef BOOST_COMP_DIAB
#   define BOOST_COMP_DIAB BOOST_PREDEF_MAKE_10_VRPP(__VERSION_NUMBER__)
#endif

#if BOOST_COMP_DIAB
#   define BOOST_COMP_DIAB_AVAILABLE
#endif

#define BOOST_COMP_DIAB_NAME "Diab C/C++"

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_DIAB,BOOST_COMP_DIAB_NAME)


#endif
