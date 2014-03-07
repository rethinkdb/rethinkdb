
// Copyright Aleksey Gurtovoy 2000-2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: largest_int.cpp 49268 2008-10-11 06:26:17Z agurtovoy $
// $Date: 2008-10-10 23:26:17 -0700 (Fri, 10 Oct 2008) $
// $Revision: 49268 $

#include <boost/mpl/aux_/largest_int.hpp>
#include <boost/mpl/aux_/test.hpp>


MPL_TEST_CASE()
{
    MPL_ASSERT(( is_same< mpl::aux::largest_int<bool,bool>::type, bool > ));
    MPL_ASSERT(( is_same< mpl::aux::largest_int<bool,char>::type, char > ));
    MPL_ASSERT(( is_same< mpl::aux::largest_int<char,bool>::type, char > ));
    MPL_ASSERT(( is_same< mpl::aux::largest_int<int,unsigned>::type, unsigned > ));
    MPL_ASSERT(( is_same< mpl::aux::largest_int<unsigned,long>::type, long > ));
}
