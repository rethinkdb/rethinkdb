
// Copyright Aleksey Gurtovoy 2000-2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: logical.cpp 49268 2008-10-11 06:26:17Z agurtovoy $
// $Date: 2008-10-10 23:26:17 -0700 (Fri, 10 Oct 2008) $
// $Revision: 49268 $

#include <boost/mpl/logical.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/aux_/test.hpp>

struct unknown;

using mpl::true_;
using mpl::false_;

MPL_TEST_CASE()
{
    MPL_ASSERT(( mpl::and_< true_,true_ > ));
    MPL_ASSERT_NOT(( mpl::and_< false_,true_ > ));
    MPL_ASSERT_NOT(( mpl::and_< true_,false_ > ));
    MPL_ASSERT_NOT(( mpl::and_< false_,false_ > ));
    MPL_ASSERT_NOT(( mpl::and_< false_,unknown > ));
    MPL_ASSERT_NOT(( mpl::and_< false_,unknown,unknown > ));

    MPL_ASSERT(( mpl::or_< true_,true_ > ));
    MPL_ASSERT(( mpl::or_< false_,true_ > ));
    MPL_ASSERT(( mpl::or_< true_,false_ > ));
    MPL_ASSERT_NOT(( mpl::or_< false_,false_ > ));
    MPL_ASSERT(( mpl::or_< true_,unknown > ));
    MPL_ASSERT(( mpl::or_< true_,unknown,unknown > ));

    MPL_ASSERT_NOT(( mpl::not_< true_ > ));
    MPL_ASSERT(( mpl::not_< false_ > ));
}
