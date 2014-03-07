/*
Copyright Redshift Software, Inc. 2008-2013
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_ARCHITECTURE_ARM_H
#define BOOST_PREDEF_ARCHITECTURE_ARM_H

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

/*`
[heading `BOOST_ARCH_ARM`]

[@http://en.wikipedia.org/wiki/ARM_architecture ARM] architecture.

[table
    [[__predef_symbol__] [__predef_version__]]

    [[`__arm__`] [__predef_detection__]]
    [[`__thumb__`] [__predef_detection__]]
    [[`__TARGET_ARCH_ARM`] [__predef_detection__]]
    [[`__TARGET_ARCH_THUMB`] [__predef_detection__]]

    [[`__TARGET_ARCH_ARM`] [V.0.0]]
    [[`__TARGET_ARCH_THUMB`] [V.0.0]]
    ]
 */

#define BOOST_ARCH_ARM BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if defined(__arm__) || defined(__thumb__) || \
    defined(__TARGET_ARCH_ARM) || defined(__TARGET_ARCH_THUMB)
#   undef BOOST_ARCH_ARM
#   if !defined(BOOST_ARCH_ARM) && defined(__TARGET_ARCH_ARM)
#       define BOOST_ARCH_ARM BOOST_VERSION_NUMBER(__TARGET_ARCH_ARM,0,0)
#   endif
#   if !defined(BOOST_ARCH_ARM) && defined(__TARGET_ARCH_THUMB)
#       define BOOST_ARCH_ARM BOOST_VERSION_NUMBER(__TARGET_ARCH_THUMB,0,0)
#   endif
#   if !defined(BOOST_ARCH_ARM)
#       define BOOST_ARCH_ARM BOOST_VERSION_NUMBER_AVAILABLE
#   endif
#endif

#if BOOST_ARCH_ARM
#   define BOOST_ARCH_ARM_AVAILABLE
#endif

#define BOOST_ARCH_ARM_NAME "ARM"

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_ARCH_ARM,BOOST_ARCH_ARM_NAME)


#endif
