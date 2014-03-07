
// Copyright Aleksey Gurtovoy 2002-2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: tuple_from_list.cpp 49268 2008-10-11 06:26:17Z agurtovoy $
// $Date: 2008-10-10 23:26:17 -0700 (Fri, 10 Oct 2008) $
// $Revision: 49268 $

#include <boost/mpl/reverse_fold.hpp>
#include <boost/mpl/list.hpp>
#include <boost/tuple/tuple.hpp>

#include <iostream>

using namespace boost::mpl;

template< typename Types > struct tuple_gen
    : reverse_fold<
          Types
        , boost::tuples::null_type
        , boost::tuples::cons<_2,_1>
        >
{
};

int main()
{
    tuple_gen< list<int,char const*,bool> >::type t;
    
    boost::get<0>(t) = -1;
    boost::get<1>(t) = "text";
    boost::get<2>(t) = false;

    std::cout
        << boost::get<0>(t) << '\n'
        << boost::get<1>(t) << '\n'
        << boost::get<2>(t) << '\n'
        ;

    return 0;
}
