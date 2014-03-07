
#ifndef BOOST_MPL_EMPTY_BASE_HPP_INCLUDED
#define BOOST_MPL_EMPTY_BASE_HPP_INCLUDED

// Copyright Aleksey Gurtovoy 2001-2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: empty_base.hpp 49267 2008-10-11 06:19:02Z agurtovoy $
// $Date: 2008-10-10 23:19:02 -0700 (Fri, 10 Oct 2008) $
// $Revision: 49267 $

#include <boost/mpl/bool.hpp>
#include <boost/mpl/aux_/config/msvc.hpp>
#include <boost/mpl/aux_/config/workaround.hpp>

#include <boost/type_traits/is_empty.hpp>

// should be always the last #include directive
#include <boost/type_traits/detail/bool_trait_def.hpp>

namespace boost { namespace mpl {

// empty base class, guaranteed to have no members; inheritance from
// 'empty_base' through the 'inherit' metafunction is a no-op - see 
// "mpl/inherit.hpp> header for the details
struct empty_base {};

template< typename T >
struct is_empty_base
    : false_
{
#if BOOST_WORKAROUND(BOOST_MSVC, < 1300)
    using false_::value;
#endif
};

template<>
struct is_empty_base<empty_base>
    : true_
{
#if BOOST_WORKAROUND(BOOST_MSVC, < 1300)
    using true_::value;
#endif
};

}}

namespace boost {
BOOST_TT_AUX_BOOL_TRAIT_SPEC1(is_empty, mpl::empty_base, true)
}

#include <boost/type_traits/detail/bool_trait_undef.hpp>

#endif // BOOST_MPL_EMPTY_BASE_HPP_INCLUDED
