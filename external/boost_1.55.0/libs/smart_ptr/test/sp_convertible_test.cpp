#include <boost/config.hpp>

//  sp_convertible_test.cpp
//
//  Copyright (c) 2012 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include <boost/detail/lightweight_test.hpp>
#include <boost/smart_ptr/detail/sp_convertible.hpp>

//

class X;

class B
{
};

class D: public B
{
};

#define TEST_CV_TRUE( T, U ) \
    BOOST_TEST(( sp_convertible< T, U >::value == true )); \
    BOOST_TEST(( sp_convertible< T, const U >::value == true )); \
    BOOST_TEST(( sp_convertible< T, volatile U >::value == true )); \
    BOOST_TEST(( sp_convertible< T, const volatile U >::value == true )); \
    BOOST_TEST(( sp_convertible< const T, U >::value == false )); \
    BOOST_TEST(( sp_convertible< const T, const U >::value == true )); \
    BOOST_TEST(( sp_convertible< const T, volatile U >::value == false )); \
    BOOST_TEST(( sp_convertible< const T, const volatile U >::value == true )); \
    BOOST_TEST(( sp_convertible< volatile T, U >::value == false )); \
    BOOST_TEST(( sp_convertible< volatile T, const U >::value == false )); \
    BOOST_TEST(( sp_convertible< volatile T, volatile U >::value == true )); \
    BOOST_TEST(( sp_convertible< volatile T, const volatile U >::value == true )); \
    BOOST_TEST(( sp_convertible< const volatile T, U >::value == false )); \
    BOOST_TEST(( sp_convertible< const volatile T, const U >::value == false )); \
    BOOST_TEST(( sp_convertible< const volatile T, volatile U >::value == false )); \
    BOOST_TEST(( sp_convertible< const volatile T, const volatile U >::value == true ));

#define TEST_CV_FALSE( T, U ) \
    BOOST_TEST(( sp_convertible< T, U >::value == false )); \
    BOOST_TEST(( sp_convertible< T, const U >::value == false )); \
    BOOST_TEST(( sp_convertible< T, volatile U >::value == false )); \
    BOOST_TEST(( sp_convertible< T, const volatile U >::value == false )); \
    BOOST_TEST(( sp_convertible< const T, U >::value == false )); \
    BOOST_TEST(( sp_convertible< const T, const U >::value == false )); \
    BOOST_TEST(( sp_convertible< const T, volatile U >::value == false )); \
    BOOST_TEST(( sp_convertible< const T, const volatile U >::value == false )); \
    BOOST_TEST(( sp_convertible< volatile T, U >::value == false )); \
    BOOST_TEST(( sp_convertible< volatile T, const U >::value == false )); \
    BOOST_TEST(( sp_convertible< volatile T, volatile U >::value == false )); \
    BOOST_TEST(( sp_convertible< volatile T, const volatile U >::value == false )); \
    BOOST_TEST(( sp_convertible< const volatile T, U >::value == false )); \
    BOOST_TEST(( sp_convertible< const volatile T, const U >::value == false )); \
    BOOST_TEST(( sp_convertible< const volatile T, volatile U >::value == false )); \
    BOOST_TEST(( sp_convertible< const volatile T, const volatile U >::value == false ));

int main()
{
#if !defined( BOOST_SP_NO_SP_CONVERTIBLE )

    using boost::detail::sp_convertible;

    TEST_CV_TRUE( X, X )
    TEST_CV_TRUE( X, void )
    TEST_CV_FALSE( void, X )
    TEST_CV_TRUE( D, B )
    TEST_CV_FALSE( B, D )

    TEST_CV_TRUE( X[], X[] )
    TEST_CV_FALSE( D[], B[] )

    TEST_CV_TRUE( X[3], X[3] )
    TEST_CV_FALSE( X[3], X[4] )
    TEST_CV_FALSE( D[3], B[3] )

    TEST_CV_TRUE( X[3], X[] )
    TEST_CV_FALSE( X[], X[3] )

    TEST_CV_TRUE( X[], void )
    TEST_CV_FALSE( void, X[] )

    TEST_CV_TRUE( X[3], void )
    TEST_CV_FALSE( void, X[3] )

#endif

    return boost::report_errors();
}
