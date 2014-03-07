/*
Copyright Redshift Software, Inc. 2008-2013
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_COMPILER_PGI_H
#define BOOST_PREDEF_COMPILER_PGI_H

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

/*`
[heading `BOOST_COMP_PGI`]

[@http://en.wikipedia.org/wiki/The_Portland_Group Portland Group C/C++] compiler.

[table
    [[__predef_symbol__] [__predef_version__]]

    [[`__PGI`] [__predef_detection__]]

    [[`__PGIC__`, `__PGIC_MINOR__`, `__PGIC_PATCHLEVEL__`] [V.R.P]]
    ]
 */

#define BOOST_COMP_PGI BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if defined(__PGI)
#   undef BOOST_COMP_PGI
#   if !defined(BOOST_COMP_PGI) && (defined(__PGIC__) && defined(__PGIC_MINOR__) && defined(__PGIC_PATCHLEVEL__))
#       define BOOST_COMP_PGI BOOST_VERSION_NUMBER(__PGIC__,__PGIC_MINOR__,__PGIC_PATCHLEVEL__)
#   endif
#   if !defined(BOOST_COMP_PGI)
#       define BOOST_COMP_PGI BOOST_VERSION_NUMBER_AVAILABLE
#   endif
#endif

#if BOOST_COMP_PGI
#   define BOOST_COMP_PGI_AVAILABLE
#endif

#define BOOST_COMP_PGI_NAME "Portland Group C/C++"

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_PGI,BOOST_COMP_PGI_NAME)


#endif
