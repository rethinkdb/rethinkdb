
// Copyright David Abrahams 2003-2004
// Copyright Aleksey Gurtovoy 2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: pair_view.cpp 49268 2008-10-11 06:26:17Z agurtovoy $
// $Date: 2008-10-10 23:26:17 -0700 (Fri, 10 Oct 2008) $
// $Revision: 49268 $

#include <boost/mpl/pair_view.hpp>
#include <boost/mpl/vector/vector50_c.hpp>
#include <boost/mpl/range_c.hpp>
#include <boost/mpl/distance.hpp>
#include <boost/mpl/aux_/test.hpp>


MPL_TEST_CASE()
{
    typedef range_c<int,0,10> r;
    typedef vector10_c<int,9,8,7,6,5,4,3,2,1,10> v;
    
    typedef pair_view<r,v> view;
    typedef begin<view>::type first_;
    typedef end<view>::type last_;

    MPL_ASSERT(( is_same< first_::category, mpl::random_access_iterator_tag > ));
    
    MPL_ASSERT(( is_same< advance_c<first_,0>::type, first_ > ));
    MPL_ASSERT(( is_same< advance_c<last_,0>::type, last_ > ));
    MPL_ASSERT(( is_same< advance_c<first_,10>::type, last_ > ));
    MPL_ASSERT(( is_same< advance_c<last_,-10>::type, first_ > ));
    
    typedef advance_c<first_,5>::type iter;
    
    MPL_ASSERT(( is_same< 
          deref<iter>::type
        , mpl::pair< integral_c<int,5>,integral_c<int,4> >
        > )); 
    
}
