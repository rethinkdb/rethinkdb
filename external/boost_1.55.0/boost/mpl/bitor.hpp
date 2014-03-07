
#ifndef BOOST_MPL_BITOR_HPP_INCLUDED
#define BOOST_MPL_BITOR_HPP_INCLUDED

// Copyright Aleksey Gurtovoy 2000-2009
// Copyright Jaap Suter 2003
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: bitor.hpp 63520 2010-07-02 08:59:55Z agurtovoy $
// $Date: 2010-07-02 01:59:55 -0700 (Fri, 02 Jul 2010) $
// $Revision: 63520 $

// agurt, 23/jan/10: workaround a conflict with <iso646.h> header's 
// macros, see http://tinyurl.com/ycwdxco; 'defined(bitor)'
// has to be checked in a separate condition, otherwise GCC complains 
// about 'bitor' being an alternative token
#if defined(_MSC_VER) 
#ifndef __GCCXML__
#if defined(bitor)
#   pragma push_macro("bitor")
#   undef bitor
#   define bitor(x)
#endif
#endif
#endif

#define AUX778076_OP_NAME   bitor_
#define AUX778076_OP_PREFIX bitor
#define AUX778076_OP_TOKEN  |
#include <boost/mpl/aux_/arithmetic_op.hpp>

#if defined(_MSC_VER)
#ifndef __GCCXML__
#if defined(bitor)
#   pragma pop_macro("bitor")
#endif
#endif
#endif

#endif // BOOST_MPL_BITOR_HPP_INCLUDED
