
// Copyright Aleksey Gurtovoy 2001-2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: integer.cpp 49268 2008-10-11 06:26:17Z agurtovoy $
// $Date: 2008-10-10 23:26:17 -0700 (Fri, 10 Oct 2008) $
// $Revision: 49268 $

#include <boost/mpl/multiplies.hpp>
#include <boost/mpl/list.hpp>
#include <boost/mpl/lower_bound.hpp>
#include <boost/mpl/transform_view.hpp>
#include <boost/mpl/sizeof.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/mpl/base.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/deref.hpp>
#include <boost/mpl/begin_end.hpp>
#include <boost/mpl/assert.hpp>

#include <boost/type_traits/is_same.hpp>

namespace mpl = boost::mpl;
using namespace mpl::placeholders;

template< int bit_size >
class big_int
{
    // ...
};

template< int bit_size >
struct integer
{
    typedef mpl::list<char,short,int,long> builtins_;
    typedef typename mpl::base< typename mpl::lower_bound<
          mpl::transform_view< builtins_
            , mpl::multiplies< mpl::sizeof_<_1>, mpl::int_<8> >
            >
        , mpl::int_<bit_size>
        >::type >::type iter_;

    typedef typename mpl::end<builtins_>::type last_;
    typedef typename mpl::eval_if<
          boost::is_same<iter_,last_>
        , mpl::identity< big_int<bit_size> >
        , mpl::deref<iter_>
        >::type type;
};

typedef integer<1>::type int1;
typedef integer<5>::type int5;
typedef integer<15>::type int15;
typedef integer<32>::type int32;
typedef integer<100>::type int100;

BOOST_MPL_ASSERT(( boost::is_same< int1, char > ));
BOOST_MPL_ASSERT(( boost::is_same< int5, char > ));
BOOST_MPL_ASSERT(( boost::is_same< int15, short > ));
BOOST_MPL_ASSERT(( boost::is_same< int32, int > ));
BOOST_MPL_ASSERT(( boost::is_same< int100, big_int<100> > ));
