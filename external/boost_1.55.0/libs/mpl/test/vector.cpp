
// Copyright Aleksey Gurtovoy 2000-2005
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: vector.cpp 49268 2008-10-11 06:26:17Z agurtovoy $
// $Date: 2008-10-10 23:26:17 -0700 (Fri, 10 Oct 2008) $
// $Revision: 49268 $

#include <boost/mpl/vector.hpp>
#include <boost/mpl/vector/vector10.hpp>
#include <boost/mpl/equal.hpp>
#include <boost/mpl/push_back.hpp>
#include <boost/mpl/pop_back.hpp>
#include <boost/mpl/push_front.hpp>
#include <boost/mpl/pop_front.hpp>
#include <boost/mpl/back.hpp>
#include <boost/mpl/front.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/empty.hpp>

#include <boost/mpl/aux_/test.hpp>


MPL_TEST_CASE()
{
    typedef vector0<> v0;
    typedef vector1<char> v1;
    typedef vector2<char,long> v2;
    typedef vector9<char,char,char,char,char,char,char,char,int> v9;

    MPL_ASSERT(( equal< v0,v0::type > ));
    MPL_ASSERT(( equal< v1,v1::type > ));
    MPL_ASSERT(( equal< v2,v2::type > ));
    MPL_ASSERT(( equal< v9,v9::type > ));

    MPL_ASSERT_RELATION( size<v0>::value, ==, 0 );
    MPL_ASSERT_RELATION( size<v1>::value, ==, 1 );
    MPL_ASSERT_RELATION( size<v2>::value, ==, 2 );
    MPL_ASSERT_RELATION( size<v9>::value, ==, 9 );

    MPL_ASSERT(( empty<v0> ));
    MPL_ASSERT_NOT(( empty<v1> ));
    MPL_ASSERT_NOT(( empty<v2> ));
    MPL_ASSERT_NOT(( empty<v9> ));

    MPL_ASSERT(( is_same< front<v1>::type,char > ));
    MPL_ASSERT(( is_same< back<v1>::type,char > ));
    MPL_ASSERT(( is_same< front<v2>::type,char > ));
    MPL_ASSERT(( is_same< back<v2>::type,long > ));
    MPL_ASSERT(( is_same< front<v9>::type,char > ));
    MPL_ASSERT(( is_same< back<v9>::type,int > ));
}


MPL_TEST_CASE()
{
    typedef vector2<char,long> v2;
    
    typedef begin<v2>::type i1;
    typedef next<i1>::type  i2;
    typedef next<i2>::type  i3;
    
    MPL_ASSERT(( is_same<deref<i1>::type,char> ));
    MPL_ASSERT(( is_same<deref<i2>::type,long> ));
    MPL_ASSERT(( is_same< i3, end<v2>::type > ));
}

MPL_TEST_CASE()
{
    typedef vector0<> v0;

    typedef push_back<v0,int>::type     v1;
    typedef push_front<v1,char>::type   v2;
    typedef push_back<v2,long>::type    v3;

    MPL_ASSERT(( is_same< back<v1>::type,int > ));
    MPL_ASSERT(( is_same< back<v2>::type,int > ));
    MPL_ASSERT(( is_same< front<v2>::type,char > ));
    MPL_ASSERT(( is_same< back<v3>::type,long > ));

    MPL_ASSERT(( equal< v1,v1::type > ));
    MPL_ASSERT(( equal< v2,v2::type > ));
    MPL_ASSERT(( equal< v3,v3::type > ));
}

MPL_TEST_CASE()
{
    typedef vector9<char,bool,char,char,char,char,bool,long,int> v9;

    typedef pop_back<v9>::type  v8;
    typedef pop_front<v8>::type v7;

    MPL_ASSERT(( is_same< back<v9>::type,int > ));
    MPL_ASSERT(( is_same< back<v8>::type,long > ));
    MPL_ASSERT(( is_same< back<v7>::type,long > ));
    MPL_ASSERT(( is_same< front<v7>::type,bool > ));

    MPL_ASSERT(( equal< v9,v9::type > ));
    MPL_ASSERT(( equal< v8,v8::type > ));
    MPL_ASSERT(( equal< v7,v7::type > ));
}

MPL_TEST_CASE()
{
    typedef vector<> v0;
    typedef vector<char> v1;
    typedef vector<char,long> v2;
    typedef vector<char,char,char,char,char,char,char,char,int> v9;

    MPL_ASSERT(( equal< v0,v0::type > ));
    MPL_ASSERT(( equal< v1,v1::type > ));
    MPL_ASSERT(( equal< v2,v2::type > ));
    MPL_ASSERT(( equal< v9,v9::type > ));

    MPL_ASSERT_RELATION( size<v0>::value, ==, 0 );
    MPL_ASSERT_RELATION( size<v1>::value, ==, 1 );
    MPL_ASSERT_RELATION( size<v2>::value, ==, 2 );
    MPL_ASSERT_RELATION( size<v9>::value, ==, 9 );
}
