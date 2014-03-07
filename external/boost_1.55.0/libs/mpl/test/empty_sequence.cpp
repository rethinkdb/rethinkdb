
// Copyright Aleksey Gurtovoy 2004
// Copyright Alexander Nasonov 2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: empty_sequence.cpp 49268 2008-10-11 06:26:17Z agurtovoy $
// $Date: 2008-10-10 23:26:17 -0700 (Fri, 10 Oct 2008) $
// $Revision: 49268 $

#include <boost/mpl/empty_sequence.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/distance.hpp>
#include <boost/mpl/advance.hpp>
#include <boost/mpl/begin_end.hpp>
#include <boost/mpl/aux_/test.hpp>

#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/add_pointer.hpp>

MPL_TEST_CASE()
{
    typedef begin<empty_sequence>::type begin;
    typedef end<empty_sequence>::type end;

    MPL_ASSERT(( is_same<begin,end> ));
    MPL_ASSERT_RELATION( (mpl::distance<begin,end>::value), ==, 0 );
    MPL_ASSERT_RELATION( size<empty_sequence>::value, ==, 0 );

    typedef advance_c<begin,0>::type advanced;
    MPL_ASSERT(( is_same<advanced,end> ));
}
