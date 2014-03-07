/*
Copyright Redshift Software, Inc. 2008-2013
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_COMPILER_HP_ACC_H
#define BOOST_PREDEF_COMPILER_HP_ACC_H

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

/*`
[heading `BOOST_COMP_HPACC`]

HP aC++ compiler.
Version number available as major, minor, and patch.

[table
    [[__predef_symbol__] [__predef_version__]]

    [[`__HP_aCC`] [__predef_detection__]]

    [[`__HP_aCC`] [V.R.P]]
    ]
 */

#define BOOST_COMP_HPACC BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if defined(__HP_aCC)
#   undef BOOST_COMP_HPACC
#   if !defined(BOOST_COMP_HPACC) && (__HP_aCC > 1)
#       define BOOST_COMP_HPACC BOOST_PREDEF_MAKE_10_VVRRPP(__HP_aCC)
#   endif
#   if !defined(BOOST_COMP_HPACC)
#       define BOOST_COMP_HPACC BOOST_VERSION_NUMBER_AVAILABLE
#   endif
#endif

#if BOOST_COMP_HPACC
#   define BOOST_COMP_HPACC_AVAILABLE
#endif

#define BOOST_COMP_HPACC_NAME "HP aC++"

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_HPACC,BOOST_COMP_HPACC_NAME)


#endif
