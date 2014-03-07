
// Copyright Aleksey Gurtovoy 2001-2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: joint_view.cpp 49268 2008-10-11 06:26:17Z agurtovoy $
// $Date: 2008-10-10 23:26:17 -0700 (Fri, 10 Oct 2008) $
// $Revision: 49268 $

#include <boost/mpl/joint_view.hpp>

#include <boost/mpl/range_c.hpp>
#include <boost/mpl/list.hpp>
#include <boost/mpl/equal.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/aux_/test.hpp>


MPL_TEST_CASE()
{
    typedef joint_view<
          range_c<int,0,10>
        , range_c<int,10,15>
        > numbers;

    typedef range_c<int,0,15> answer;

    MPL_ASSERT(( equal<numbers,answer> ));
    MPL_ASSERT_RELATION( size<numbers>::value, ==, 15 );
}

template< typename View > struct test_is_empty
{
    typedef typename begin<View>::type first_;
    typedef typename end<View>::type last_;
    
    MPL_ASSERT_RELATION( size<View>::value, ==, 0 );
    MPL_ASSERT(( is_same< first_,last_> ));
    
    MPL_ASSERT_INSTANTIATION( View );
    MPL_ASSERT_INSTANTIATION( first_ );
    MPL_ASSERT_INSTANTIATION( last_ );
};

MPL_TEST_CASE()
{
    test_is_empty< joint_view< list0<>,list0<> > >();
    test_is_empty< joint_view< list<>,list0<> > >();
    test_is_empty< joint_view< list<>,list<> > >();
    test_is_empty< joint_view< list<>, joint_view< list0<>,list0<> > > >();
}
