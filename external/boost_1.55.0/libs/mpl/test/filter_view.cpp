
// Copyright Aleksey Gurtovoy 2001-2004
//
// Distributed under the Boost Software License,Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: filter_view.cpp 49268 2008-10-11 06:26:17Z agurtovoy $
// $Date: 2008-10-10 23:26:17 -0700 (Fri, 10 Oct 2008) $
// $Revision: 49268 $

#include <boost/static_assert.hpp>
#include <boost/mpl/filter_view.hpp>
#include <boost/mpl/transform_view.hpp>
#include <boost/mpl/max_element.hpp>
#include <boost/mpl/list.hpp>
#include <boost/mpl/sizeof.hpp>
#include <boost/mpl/deref.hpp>
#include <boost/mpl/aux_/test.hpp>

#include <boost/type_traits/is_float.hpp>
#include <boost/type_traits/is_same.hpp>

MPL_TEST_CASE()
{
    typedef mpl::list<int,float,long,float,char[50],long double,char> types;
    typedef mpl::max_element<
          mpl::transform_view<
              mpl::filter_view< types,boost::is_float<_> >
            , mpl::sizeof_<_>
            >
        >::type iter;

    MPL_ASSERT((is_same<mpl::deref<iter::base>::type, long double>));
}
