
#ifndef BOOST_MPL_REPLACE_IF_HPP_INCLUDED
#define BOOST_MPL_REPLACE_IF_HPP_INCLUDED

// Copyright Aleksey Gurtovoy 2000-2004
// Copyright John R. Bandela 2000-2002
// Copyright David Abrahams 2003-2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: replace_if.hpp 49267 2008-10-11 06:19:02Z agurtovoy $
// $Date: 2008-10-10 23:19:02 -0700 (Fri, 10 Oct 2008) $
// $Revision: 49267 $

#include <boost/mpl/transform.hpp>
#include <boost/mpl/apply.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/aux_/inserter_algorithm.hpp>
#include <boost/mpl/aux_/config/forwarding.hpp>

namespace boost { namespace mpl {

namespace aux {

template< typename Predicate, typename T >
struct replace_if_op
{
    template< typename U > struct apply
#if !defined(BOOST_MPL_CFG_NO_NESTED_FORWARDING)
        : if_<
              typename apply1<Predicate,U>::type
            , T
            , U
            >
    {
#else
    {
        typedef typename if_<
              typename apply1<Predicate,U>::type
            , T
            , U
            >::type type;
#endif
    };
};


template<
      typename Sequence
    , typename Predicate
    , typename T
    , typename Inserter
    >
struct replace_if_impl
    : transform1_impl<
          Sequence
        , protect< aux::replace_if_op<Predicate,T> >
        , Inserter
        >
{
};

template<
      typename Sequence
    , typename Predicate
    , typename T
    , typename Inserter
    >
struct reverse_replace_if_impl
    : reverse_transform1_impl<
          Sequence
        , protect< aux::replace_if_op<Predicate,T> >
        , Inserter
        >
{
};

} // namespace aux

BOOST_MPL_AUX_INSERTER_ALGORITHM_DEF(4, replace_if)

}}

#endif // BOOST_MPL_REPLACE_IF_HPP_INCLUDED
