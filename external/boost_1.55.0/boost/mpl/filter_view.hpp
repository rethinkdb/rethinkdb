
#ifndef BOOST_MPL_FILTER_VIEW_HPP_INCLUDED
#define BOOST_MPL_FILTER_VIEW_HPP_INCLUDED

// Copyright Aleksey Gurtovoy 2000-2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: filter_view.hpp 49267 2008-10-11 06:19:02Z agurtovoy $
// $Date: 2008-10-10 23:19:02 -0700 (Fri, 10 Oct 2008) $
// $Revision: 49267 $

#include <boost/mpl/begin_end.hpp>
#include <boost/mpl/lambda.hpp>
#include <boost/mpl/aux_/filter_iter.hpp>
#include <boost/mpl/aux_/na_spec.hpp>

namespace boost { namespace mpl {

template<
      typename BOOST_MPL_AUX_NA_PARAM(Sequence)
    , typename BOOST_MPL_AUX_NA_PARAM(Predicate)
    >
struct filter_view
{
 private:    
    typedef typename lambda<Predicate>::type pred_;
    typedef typename begin<Sequence>::type first_;
    typedef typename end<Sequence>::type last_;

 public:
    struct tag;
    typedef filter_view type;
    typedef typename aux::next_filter_iter< first_,last_,pred_ >::type begin;
    typedef aux::filter_iter< last_,last_,pred_ > end;
};

BOOST_MPL_AUX_NA_SPEC(2, filter_view)

}}

#endif // BOOST_MPL_FILTER_VIEW_HPP_INCLUDED
