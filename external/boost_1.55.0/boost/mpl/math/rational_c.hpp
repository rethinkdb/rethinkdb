
#ifndef BOOST_MPL_RATIONAL_C_HPP_INCLUDED
#define BOOST_MPL_RATIONAL_C_HPP_INCLUDED

// Copyright Aleksey Gurtovoy 2000-2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: rational_c.hpp 49267 2008-10-11 06:19:02Z agurtovoy $
// $Date: 2008-10-10 23:19:02 -0700 (Fri, 10 Oct 2008) $
// $Revision: 49267 $

#include <boost/mpl/aux_/config/static_constant.hpp>

namespace boost { namespace mpl {

template<
      typename IntegerType
    , IntegerType N
    , IntegerType D = 1
    >
struct rational_c
{
    BOOST_STATIC_CONSTANT(IntegerType, numerator = N);
    BOOST_STATIC_CONSTANT(IntegerType, denominator = D);
    
    typedef rational_c<IntegerType,N,D> type;
    rational_c() {}
};

}}

#endif // BOOST_MPL_RATIONAL_C_HPP_INCLUDED
