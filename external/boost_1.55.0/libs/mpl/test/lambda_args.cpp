
// Copyright Aleksey Gurtovoy 2001-2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: lambda_args.cpp 49268 2008-10-11 06:26:17Z agurtovoy $
// $Date: 2008-10-10 23:26:17 -0700 (Fri, 10 Oct 2008) $
// $Revision: 49268 $

#include <boost/mpl/lambda.hpp>
#include <boost/mpl/apply.hpp>
#include <boost/mpl/aux_/test.hpp>
#include <boost/mpl/aux_/config/gcc.hpp>
#include <boost/mpl/aux_/config/workaround.hpp>

typedef int UDT::* mem_ptr;
typedef int (UDT::* mem_fun_ptr)();

#define AUX_LAMBDA_TEST(T) \
    { MPL_ASSERT(( apply1<lambda< is_same<_,T> >::type, T> )); } \
    { MPL_ASSERT(( apply1<lambda< is_same<T,_> >::type, T>  )); } \
    { MPL_ASSERT(( apply2<lambda< is_same<_,_> >::type, T, T> )); } \
/**/

MPL_TEST_CASE()
{
    AUX_LAMBDA_TEST( UDT );
    AUX_LAMBDA_TEST( abstract );
    AUX_LAMBDA_TEST( noncopyable );
    AUX_LAMBDA_TEST( incomplete );
    AUX_LAMBDA_TEST( int );
    AUX_LAMBDA_TEST( void );
    AUX_LAMBDA_TEST( double );
    AUX_LAMBDA_TEST( int& );
    AUX_LAMBDA_TEST( int* );
#if !BOOST_WORKAROUND(BOOST_MPL_CFG_GCC, <= 0x0295) \
    && !BOOST_WORKAROUND(__BORLANDC__, < 0x600)
    AUX_LAMBDA_TEST( int[] );
#endif
    AUX_LAMBDA_TEST( int[10] );
    AUX_LAMBDA_TEST( int (*)() )
    AUX_LAMBDA_TEST( mem_ptr );
    AUX_LAMBDA_TEST( mem_fun_ptr );
}
