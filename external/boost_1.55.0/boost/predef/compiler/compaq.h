/*
Copyright Redshift Software, Inc. 2008-2013
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_COMPILER_COMPAQ_H
#define BOOST_PREDEF_COMPILER_COMPAQ_H

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

/*`
[heading `BOOST_COMP_DEC`]

[@http://www.openvms.compaq.com/openvms/brochures/deccplus/ Compaq C/C++] compiler.
Version number available as major, minor, and patch.

[table
    [[__predef_symbol__] [__predef_version__]]

    [[`__DECCXX`] [__predef_detection__]]
    [[`__DECC`] [__predef_detection__]]

    [[`__DECCXX_VER`] [V.R.P]]
    [[`__DECC_VER`] [V.R.P]]
    ]
 */

#define BOOST_COMP_DEC BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if defined(__DECC) || defined(__DECCXX)
#   undef BOOST_COMP_DEC
#   if !defined(BOOST_COMP_DEC) && defined(__DECCXX_VER)
#       define BOOST_COMP_DEC BOOST_PREDEF_MAKE_10_VVRR0PP00(__DECCXX_VER)
#   endif
#   if !defined(BOOST_COMP_DEC) && defined(__DECC_VER)
#       define BOOST_COMP_DEC BOOST_PREDEF_MAKE_10_VVRR0PP00(__DECC_VER)
#   endif
#   if !defined(BOOST_COMP_DEC)
#       define BOOST_COM_DEV BOOST_VERSION_NUMBER_AVAILABLE
#   endif
#endif

#if BOOST_COMP_DEC
#   define BOOST_COMP_DEC_AVAILABLE
#endif

#define BOOST_COMP_DEC_NAME "Compaq C/C++"

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_DEC,BOOST_COMP_DEC_NAME)


#endif
