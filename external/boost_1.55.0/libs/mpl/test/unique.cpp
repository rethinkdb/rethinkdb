
// Copyright Aleksey Gurtovoy 2000-2004
// Copyright David Abrahams 2003-2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: unique.cpp 49268 2008-10-11 06:26:17Z agurtovoy $
// $Date: 2008-10-10 23:26:17 -0700 (Fri, 10 Oct 2008) $
// $Revision: 49268 $

#include <boost/mpl/unique.hpp>
#include <boost/mpl/list.hpp>
#include <boost/mpl/equal.hpp>
#include <boost/type_traits/is_same.hpp>

#include <boost/mpl/aux_/test.hpp>

MPL_TEST_CASE()
{
    typedef list<int,float,float,char,int,int,int,double> types;
    typedef unique< types, is_same<_1,_2> >::type result;

    typedef list<int,float,char,int,double>::type answer;
    MPL_ASSERT(( equal< result,answer > ));
}
