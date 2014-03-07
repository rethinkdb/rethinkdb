
// Copyright Aleksey Gurtovoy 2000-2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: advance.cpp 49268 2008-10-11 06:26:17Z agurtovoy $
// $Date: 2008-10-10 23:26:17 -0700 (Fri, 10 Oct 2008) $
// $Revision: 49268 $

#include <boost/mpl/advance.hpp>
#include <boost/mpl/iterator_tags.hpp>
#include <boost/mpl/aux_/test.hpp>

template< int pos > struct iter
{
    typedef mpl::bidirectional_iterator_tag category;
    typedef iter<(pos + 1)> next;
    typedef iter<(pos - 1)> prior;
    typedef int_<pos> type;
};

#if BOOST_WORKAROUND(__MWERKS__, BOOST_TESTED_AT(0x3003))
namespace boost { namespace mpl {
template< int pos, typename Default > struct tag< iter<pos>,Default > : void_ {};
}}
#endif

typedef iter<0> first;
typedef iter<10> last;

MPL_TEST_CASE()
{
    typedef mpl::advance<first,int_<10> >::type iter1;
    typedef advance_c<first,10>::type           iter2;

    MPL_ASSERT(( is_same<iter1, last> ));
    MPL_ASSERT(( is_same<iter2, last> ));
}

MPL_TEST_CASE()
{
    typedef mpl::advance<last,int_<-10> >::type iter1;
    typedef advance_c<last,-10>::type           iter2;

    MPL_ASSERT(( is_same<iter1, first> ));
    MPL_ASSERT(( is_same<iter2, first> ));
}
