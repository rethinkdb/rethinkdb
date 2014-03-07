
// Copyright Aleksey Gurtovoy 2002-2010
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: zip_view.cpp 61591 2010-04-26 21:31:09Z agurtovoy $
// $Date: 2010-04-26 14:31:09 -0700 (Mon, 26 Apr 2010) $
// $Revision: 61591 $

#include <boost/mpl/zip_view.hpp>

#include <boost/mpl/transform_view.hpp>
#include <boost/mpl/filter_view.hpp>
#include <boost/mpl/range_c.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/at.hpp>
#include <boost/mpl/equal.hpp>
#include <boost/mpl/equal_to.hpp>
#include <boost/mpl/unpack_args.hpp>
#include <boost/mpl/math/is_even.hpp>

#include <boost/mpl/aux_/test.hpp>

#include <boost/type_traits/is_same.hpp>


MPL_TEST_CASE()
{
    typedef transform_view<
          zip_view< vector< range_c<int,0,10>, range_c<int,10,20> > >
        , unpack_args< plus<> >
        > result;

    MPL_ASSERT(( equal< 
          result
        , filter_view< range_c<int,10,30>, is_even<_> >
        , equal_to<_,_>
        > ));

    MPL_ASSERT(( boost::is_same< zip_view<vector<> >, zip_view<vector<> >::type > ));
}
