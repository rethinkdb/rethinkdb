//
//  verify_test.cpp - a test for BOOST_VERIFY
//
//  Copyright (c) 2002 Peter Dimov and Multi Media Ltd.
//  Copyright (c) 2007 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/detail/lightweight_test.hpp>

#include <boost/assert.hpp>

int f( int & x )
{
    return ++x;
}

void test_default()
{
    int x = 1;

    BOOST_VERIFY( 1 );
    BOOST_VERIFY( x == 1 );
    BOOST_VERIFY( ++x );
    BOOST_VERIFY( f(x) );
    BOOST_VERIFY( &x );

    BOOST_TEST( x == 3 );
}

#define BOOST_DISABLE_ASSERTS
#include <boost/assert.hpp>

void test_disabled()
{
    int x = 1;

    BOOST_VERIFY( 1 );
    BOOST_VERIFY( x == 1 );
    BOOST_VERIFY( ++x );
    BOOST_VERIFY( f(x) );
    BOOST_VERIFY( &x );

    BOOST_TEST( x == 3 );

    BOOST_VERIFY( 0 );
    BOOST_VERIFY( !x );
    BOOST_VERIFY( x == 0 );
    BOOST_VERIFY( !++x );
    BOOST_VERIFY( !f(x) );

    BOOST_TEST( x == 5 );

    void * p = 0;
    BOOST_VERIFY( p );
}

#undef BOOST_DISABLE_ASSERTS

#define BOOST_ENABLE_ASSERT_HANDLER
#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <cstdio>

int handler_invoked = 0;

void boost::assertion_failed(char const * expr, char const * function, char const * file, long line)
{
#if !defined(BOOST_NO_STDC_NAMESPACE)
    using std::printf;
#endif

    printf("Expression: %s\nFunction: %s\nFile: %s\nLine: %ld\n\n", expr, function, file, line);
    ++handler_invoked;
}

struct X
{
    static bool f()
    {
        BOOST_VERIFY( 0 );
        return false;
    }
};

void test_handler()
{
    int x = 1;

    BOOST_VERIFY( 1 );
    BOOST_VERIFY( x == 1 );
    BOOST_VERIFY( ++x );
    BOOST_VERIFY( f(x) );
    BOOST_VERIFY( &x );

    BOOST_TEST( x == 3 );

    BOOST_VERIFY( 0 );
    BOOST_VERIFY( !x );
    BOOST_VERIFY( x == 0 );
    BOOST_VERIFY( !++x );
    BOOST_VERIFY( !f(x) );

    BOOST_TEST( x == 5 );

    void * p = 0;
    BOOST_VERIFY( p );

    BOOST_VERIFY( X::f() );

    BOOST_TEST( handler_invoked == 8 );
}

#undef BOOST_ENABLE_ASSERT_HANDLER

int main()
{
    test_default();
    test_disabled();
    test_handler();

    return boost::report_errors();
}
