/*
Copyright Redshift Software, Inc. 2008-2013
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_COMPILER_INTEL_H
#define BOOST_PREDEF_COMPILER_INTEL_H

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

/*`
[heading `BOOST_COMP_INTEL`]

[@http://en.wikipedia.org/wiki/Intel_C%2B%2B Intel C/C++] compiler.
Version number available as major, minor, and patch.

[table
    [[__predef_symbol__] [__predef_version__]]

    [[`__INTEL_COMPILER`] [__predef_detection__]]
    [[`__ICL`] [__predef_detection__]]
    [[`__ICC`] [__predef_detection__]]
    [[`__ECC`] [__predef_detection__]]

    [[`__INTEL_COMPILER`] [V.R.P]]
    ]
 */

#define BOOST_COMP_INTEL BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if defined(__INTEL_COMPILER) || defined(__ICL) || defined(__ICC) || \
    defined(__ECC)
#   undef BOOST_COMP_INTEL
#   if !defined(BOOST_COMP_INTEL) && defined(__INTEL_COMPILER)
#       define BOOST_COMP_INTEL BOOST_PREDEF_MAKE_10_VRP(__INTEL_COMPILER)
#   endif
#   if !defined(BOOST_COMP_INTEL)
#       define BOOST_COMP_INTEL BOOST_VERSION_NUMBER_AVAILABLE
#   endif
#endif

#if BOOST_COMP_INTEL
#   define BOOST_COMP_INTEL_AVAILABLE
#endif

#define BOOST_COMP_INTEL_NAME "Intel C/C++"

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_INTEL,BOOST_COMP_INTEL_NAME)


#endif
