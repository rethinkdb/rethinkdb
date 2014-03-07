/*
Copyright Redshift Software, Inc. 2008-2013
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_COMPILER_GREENHILLS_H
#define BOOST_PREDEF_COMPILER_GREENHILLS_H

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

/*`
[heading `BOOST_COMP_GHS`]

[@http://en.wikipedia.org/wiki/Green_Hills_Software Green Hills C/C++] compiler.
Version number available as major, minor, and patch.

[table
    [[__predef_symbol__] [__predef_version__]]

    [[`__ghs`] [__predef_detection__]]
    [[`__ghs__`] [__predef_detection__]]

    [[`__GHS_VERSION_NUMBER__`] [V.R.P]]
    [[`__ghs`] [V.R.P]]
    ]
 */

#define BOOST_COMP_GHS BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if defined(__ghs) || defined(__ghs__)
#   undef BOOST_COMP_GHS
#   if !defined(BOOST_COMP_GHS) && defined(__GHS_VERSION_NUMBER__)
#       define BOOST_COMP_GHS BOOST_PREDEF_MAKE_10_VRP(__GHS_VERSION_NUMBER__)
#   endif
#   if !defined(BOOST_COMP_GHS) && defined(__ghs)
#       define BOOST_COMP_GHS BOOST_PREDEF_MAKE_10_VRP(__ghs)
#   endif
#   if !defined(BOOST_COMP_GHS)
#       define BOOST_COMP_GHS BOOST_VERSION_NUMBER_AVAILABLE
#   endif
#endif

#if BOOST_COMP_GHS
#   define BOOST_COMP_GHS_AVAILABLE
#endif

#define BOOST_COMP_GHS_NAME "Green Hills C/C++"

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_GHS,BOOST_COMP_GHS_NAME)


#endif
