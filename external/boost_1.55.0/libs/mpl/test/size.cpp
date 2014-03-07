
// Copyright Aleksey Gurtovoy 2000-2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: size.cpp 49268 2008-10-11 06:26:17Z agurtovoy $
// $Date: 2008-10-10 23:26:17 -0700 (Fri, 10 Oct 2008) $
// $Revision: 49268 $

#include <boost/mpl/size.hpp>

#include <boost/mpl/list.hpp>
#include <boost/mpl/aux_/test.hpp>

MPL_TEST_CASE()
{
    typedef list<> empty_list;
    typedef list<char,short,int,long> list;
    
    MPL_ASSERT_RELATION( size<empty_list>::value, ==, 0 );
    MPL_ASSERT_RELATION( size<list>::value, ==, 4 );
}
