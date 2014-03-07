/*
Copyright Redshift Software, Inc. 2008-2013
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_COMPILER_MICROTEC_H
#define BOOST_PREDEF_COMPILER_MICROTEC_H

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

/*`
[heading `BOOST_COMP_MRI`]

[@http://www.mentor.com/microtec/ Microtec C/C++] compiler.

[table
    [[__predef_symbol__] [__predef_version__]]

    [[`_MRI`] [__predef_detection__]]
    ]
 */

#define BOOST_COMP_MRI BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if defined(_MRI)
#   undef BOOST_COMP_MRI
#   define BOOST_COMP_MRI BOOST_VERSION_NUMBER_AVAILABLE
#endif

#if BOOST_COMP_MRI
#   define BOOST_COMP_MRI_AVAILABLE
#endif

#define BOOST_COMP_MRI_NAME "Microtec C/C++"

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_MRI,BOOST_COMP_MRI_NAME)


#endif
