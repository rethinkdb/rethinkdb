/*
Copyright Redshift Software, Inc. 2008-2013
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_COMPILER_EKOPATH_H
#define BOOST_PREDEF_COMPILER_EKOPATH_H

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

/*`
[heading `BOOST_COMP_PATH`]

[@http://en.wikipedia.org/wiki/PathScale EKOpath] compiler.
Version number available as major, minor, and patch.

[table
    [[__predef_symbol__] [__predef_version__]]

    [[`__PATHCC__`] [__predef_detection__]]

    [[`__PATHCC__`, `__PATHCC_MINOR__`, `__PATHCC_PATCHLEVEL__`] [V.R.P]]
    ]
 */

#define BOOST_COMP_PATH BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if defined(__PATHCC__)
#   undef BOOST_COMP_PATH
#   define BOOST_COMP_PATH \
        BOOST_VERSION_NUMBER(__PATHCC__,__PATHCC_MINOR__,__PATHCC_PATCHLEVEL__)
#endif

#if BOOST_COMP_PATH
#   define BOOST_COMP_PATH_AVAILABLE
#endif

#define BOOST_COMP_PATH_NAME "EKOpath"

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_PATH,BOOST_COMP_PATH_NAME)


#endif
