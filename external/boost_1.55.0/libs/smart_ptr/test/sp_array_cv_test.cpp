//
//  sp_array_cv_test.cpp
//
//  Copyright (c) 2012 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//

#include <boost/shared_ptr.hpp>

struct X
{
};

class B
{
};

class D: public B
{
};

#define TEST_CONV( T, U ) \
    { \
        boost::shared_ptr< T > p1; \
        boost::shared_ptr< U > p2( p1 ); \
        p2 = p1; \
        boost::shared_ptr< U > p3 = boost::shared_ptr< T >(); \
        p3 = boost::shared_ptr< T >(); \
    }

#define TEST_CV_TRUE( T, U ) \
    TEST_CONV( T, U ) \
    TEST_CONV( T, const U ) \
    TEST_CONV( T, volatile U ) \
    TEST_CONV( T, const volatile U ) \
    TEST_CONV( const T, const U ) \
    TEST_CONV( const T, const volatile U ) \
    TEST_CONV( volatile T, volatile U ) \
    TEST_CONV( volatile T, const volatile U ) \
    TEST_CONV( const volatile T, const volatile U )

int main()
{
    TEST_CV_TRUE( X, X )
    TEST_CV_TRUE( X, void )
    TEST_CV_TRUE( D, B )

    TEST_CV_TRUE( X[], X[] )
    TEST_CV_TRUE( X[3], X[3] )

    TEST_CV_TRUE( X[3], X[] )

    TEST_CV_TRUE( X[], void )
    TEST_CV_TRUE( X[3], void )

    return 0;
}
