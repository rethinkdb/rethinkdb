/*
Copyright Redshift Software, Inc. 2008-2013
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_COMPILER_COMEAU_H
#define BOOST_PREDEF_COMPILER_COMEAU_H

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

#define BOOST_COMP_COMO BOOST_VERSION_NUMBER_NOT_AVAILABLE

/*`
[heading `BOOST_COMP_COMO`]

[@http://en.wikipedia.org/wiki/Comeau_C/C%2B%2B Comeau C++] compiler.
Version number available as major, minor, and patch.

[table
    [[__predef_symbol__] [__predef_version__]]

    [[`__COMO__`] [__predef_detection__]]

    [[`__COMO_VERSION__`] [V.R.P]]
    ]
 */

#if defined(__COMO__)
#   undef BOOST_COMP_COMO
#   if !defined(BOOST_COMP_COMO) && defined(__CONO_VERSION__)
#       define BOOST_COMP_COMO BOOST_PREDEF_MAKE_0X_VRP(__COMO_VERSION__)
#   endif
#   if !defined(BOOST_COMP_COMO)
#       define BOOST_COMP_COMO BOOST_VERSION_NUMBER_AVAILABLE
#   endif
#endif

#if BOOST_COMP_COMO
#   define BOOST_COMP_COMO_AVAILABLE
#endif

#define BOOST_COMP_COMO_NAME "Comeau C++"

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_COMO,BOOST_COMP_COMO_NAME)


#endif
