
// Copyright Aleksey Gurtovoy 2002-2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: inherit_linearly.cpp 49268 2008-10-11 06:26:17Z agurtovoy $
// $Date: 2008-10-10 23:26:17 -0700 (Fri, 10 Oct 2008) $
// $Revision: 49268 $

#include <boost/mpl/inherit_linearly.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/list.hpp>

#include <iostream>

namespace mpl = boost::mpl;
using namespace mpl::placeholders;

template< typename Base, typename T >
struct tuple_part
    : Base
{
    typedef tuple_part type; // note the typedef
    typedef typename Base::index::next index;

    friend T& field(tuple_part& t, index) { return t.field_; }
    T field_;
};

struct empty_tuple
{
    typedef mpl::int_<-1> index;
};


typedef mpl::inherit_linearly<
      mpl::list<int,char const*,bool>
    , tuple_part<_,_>
    , empty_tuple
    >::type my_tuple;
    

int main()
{
    my_tuple t;
    
    field(t, mpl::int_<0>()) = -1;
    field(t, mpl::int_<1>()) = "text";
    field(t, mpl::int_<2>()) = false;

    std::cout
        << field(t, mpl::int_<0>()) << '\n'
        << field(t, mpl::int_<1>()) << '\n'
        << field(t, mpl::int_<2>()) << '\n'
        ;

    return 0;
}
