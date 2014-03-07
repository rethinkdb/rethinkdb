
#ifndef BOOST_MPL_ERASE_HPP_INCLUDED
#define BOOST_MPL_ERASE_HPP_INCLUDED

// Copyright Aleksey Gurtovoy 2000-2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: erase.hpp 49267 2008-10-11 06:19:02Z agurtovoy $
// $Date: 2008-10-10 23:19:02 -0700 (Fri, 10 Oct 2008) $
// $Revision: 49267 $

#include <boost/mpl/erase_fwd.hpp>
#include <boost/mpl/sequence_tag.hpp>
#include <boost/mpl/aux_/erase_impl.hpp>
#include <boost/mpl/aux_/na_spec.hpp>
#include <boost/mpl/aux_/lambda_support.hpp>
#include <boost/mpl/aux_/config/msvc_typename.hpp>

namespace boost { namespace mpl {

template<
      typename BOOST_MPL_AUX_NA_PARAM(Sequence)
    , typename BOOST_MPL_AUX_NA_PARAM(First)
    , typename BOOST_MPL_AUX_NA_PARAM(Last)
    >
struct erase
    : erase_impl< typename sequence_tag<Sequence>::type >
        ::template apply< Sequence,First,Last >
{
    BOOST_MPL_AUX_LAMBDA_SUPPORT(3,erase,(Sequence,First,Last))
};

BOOST_MPL_AUX_NA_SPEC(3,erase)

}}

#endif // BOOST_MPL_ERASE_HPP_INCLUDED
