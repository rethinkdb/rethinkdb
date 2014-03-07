
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

#include <boost/array.hpp>
#include <boost/assert.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/utility.hpp>

#include <boost/context/all.hpp>

#include "../example/simple_stack_allocator.hpp"

namespace ctx = boost::context;

typedef ctx::simple_stack_allocator<
    8 * 1024 * 1024, // 8MB
    64 * 1024, // 64kB
    8 * 1024 // 8kB
>       stack_allocator;

ctx::fcontext_t fcm;
ctx::fcontext_t * fc = 0;
int value1 = 0;
std::string value2;
double value3 = 0.;

void f1( intptr_t)
{
    ++value1;
    ctx::jump_fcontext( fc, & fcm, 0);
}

void f3( intptr_t)
{
    ++value1;
    ctx::jump_fcontext( fc, & fcm, 0);
    ++value1;
    ctx::jump_fcontext( fc, & fcm, 0);
}

void f4( intptr_t)
{
    ctx::jump_fcontext( fc, & fcm, 7);
}

void f5( intptr_t arg)
{
    ctx::jump_fcontext( fc, & fcm, arg);
}

void f6( intptr_t arg)
{
    std::pair< int, int > data = * ( std::pair< int, int > * ) arg;
    int res = data.first + data.second;
    data = * ( std::pair< int, int > *)
        ctx::jump_fcontext( fc, & fcm, ( intptr_t) res);
    res = data.first + data.second;
    ctx::jump_fcontext( fc, & fcm, ( intptr_t) res);
}

void f7( intptr_t arg)
{
    try
    { throw std::runtime_error( ( char *) arg); }
    catch ( std::runtime_error const& e)
    { value2 = e.what(); }
    ctx::jump_fcontext( fc, & fcm, arg);
}

void f8( intptr_t arg)
{
    double d = * ( double *) arg;
    d += 3.45;
    value3 = d;
    ctx::jump_fcontext( fc, & fcm, 0);
}

void test_setup()
{
    stack_allocator alloc;

    void * sp = alloc.allocate( stack_allocator::minimum_stacksize() );
    fc = ctx::make_fcontext( sp, stack_allocator::minimum_stacksize(), f1);
    BOOST_CHECK( fc);
    BOOST_CHECK_EQUAL( sp, fc->fc_stack.sp);
    BOOST_CHECK_EQUAL( stack_allocator::minimum_stacksize(), fc->fc_stack.size);
}

void test_start()
{
    value1 = 0;

    stack_allocator alloc;

    void * sp = alloc.allocate( stack_allocator::minimum_stacksize() );
    fc = ctx::make_fcontext( sp, stack_allocator::minimum_stacksize(), f1);
    BOOST_CHECK( fc);
    BOOST_CHECK_EQUAL( sp, fc->fc_stack.sp);
    BOOST_CHECK_EQUAL( stack_allocator::minimum_stacksize(), fc->fc_stack.size);

    BOOST_CHECK_EQUAL( 0, value1);
    ctx::jump_fcontext( & fcm, fc, 0);
    BOOST_CHECK_EQUAL( 1, value1);
}

void test_jump()
{
    value1 = 0;

    stack_allocator alloc;

    void * sp = alloc.allocate( stack_allocator::minimum_stacksize() );
    fc = ctx::make_fcontext( sp, stack_allocator::minimum_stacksize(), f3);
    BOOST_CHECK( fc);
    BOOST_CHECK_EQUAL( sp, fc->fc_stack.sp);
    BOOST_CHECK_EQUAL( stack_allocator::minimum_stacksize(), fc->fc_stack.size);

    BOOST_CHECK_EQUAL( 0, value1);
    ctx::jump_fcontext( & fcm, fc, 0);
    BOOST_CHECK_EQUAL( 1, value1);
    ctx::jump_fcontext( & fcm, fc, 0);
    BOOST_CHECK_EQUAL( 2, value1);
}

void test_result()
{
    stack_allocator alloc;

    void * sp = alloc.allocate( stack_allocator::minimum_stacksize() );
    fc = ctx::make_fcontext( sp, stack_allocator::minimum_stacksize(), f4);
    BOOST_CHECK( fc);
    BOOST_CHECK_EQUAL( sp, fc->fc_stack.sp);
    BOOST_CHECK_EQUAL( stack_allocator::minimum_stacksize(), fc->fc_stack.size);

    int result = ( int) ctx::jump_fcontext( & fcm, fc, 0);
    BOOST_CHECK_EQUAL( 7, result);
}

void test_arg()
{
    stack_allocator alloc;

    int i = 7;
    void * sp = alloc.allocate( stack_allocator::minimum_stacksize() );
    fc = ctx::make_fcontext( sp, stack_allocator::minimum_stacksize(), f5);
    BOOST_CHECK( fc);
    BOOST_CHECK_EQUAL( sp, fc->fc_stack.sp);
    BOOST_CHECK_EQUAL( stack_allocator::minimum_stacksize(), fc->fc_stack.size);

    int result = ( int) ctx::jump_fcontext( & fcm, fc, i);
    BOOST_CHECK_EQUAL( i, result);
}

void test_transfer()
{
    stack_allocator alloc;

    std::pair< int, int > data = std::make_pair( 3, 7);
    void * sp = alloc.allocate( stack_allocator::minimum_stacksize() );
    fc = ctx::make_fcontext( sp, stack_allocator::minimum_stacksize(), f6);
    BOOST_CHECK( fc);
    BOOST_CHECK_EQUAL( sp, fc->fc_stack.sp);
    BOOST_CHECK_EQUAL( stack_allocator::minimum_stacksize(), fc->fc_stack.size);

    int result = ( int) ctx::jump_fcontext( & fcm, fc, ( intptr_t) & data);
    BOOST_CHECK_EQUAL( 10, result);
    data = std::make_pair( 7, 7);
    result = ( int) ctx::jump_fcontext( & fcm, fc, ( intptr_t) & data);
    BOOST_CHECK_EQUAL( 14, result);
}

void test_exception()
{
    stack_allocator alloc;

    const char * what = "hello world";
    void * sp = alloc.allocate( stack_allocator::default_stacksize() );
    fc = ctx::make_fcontext( sp, stack_allocator::default_stacksize(), f7);
    BOOST_CHECK( fc);
    BOOST_CHECK_EQUAL( sp, fc->fc_stack.sp);
    BOOST_CHECK_EQUAL( stack_allocator::default_stacksize(), fc->fc_stack.size);

    ctx::jump_fcontext( & fcm, fc, ( intptr_t) what);
    BOOST_CHECK_EQUAL( std::string( what), value2);
}

void test_fp()
{
    stack_allocator alloc;

    double d = 7.13;
    void * sp = alloc.allocate( stack_allocator::minimum_stacksize() );
    fc = ctx::make_fcontext( sp, stack_allocator::minimum_stacksize(), f8);
    BOOST_CHECK( fc);
    BOOST_CHECK_EQUAL( sp, fc->fc_stack.sp);
    BOOST_CHECK_EQUAL( stack_allocator::minimum_stacksize(), fc->fc_stack.size);

    ctx::jump_fcontext( & fcm, fc, (intptr_t) & d);
    BOOST_CHECK_EQUAL( 10.58, value3);
}

boost::unit_test::test_suite * init_unit_test_suite( int, char* [])
{
    boost::unit_test::test_suite * test =
        BOOST_TEST_SUITE("Boost.Context: context test suite");

    test->add( BOOST_TEST_CASE( & test_setup) );
    test->add( BOOST_TEST_CASE( & test_start) );
    test->add( BOOST_TEST_CASE( & test_jump) );
    test->add( BOOST_TEST_CASE( & test_result) );
    test->add( BOOST_TEST_CASE( & test_arg) );
    test->add( BOOST_TEST_CASE( & test_transfer) );
    test->add( BOOST_TEST_CASE( & test_exception) );
    test->add( BOOST_TEST_CASE( & test_fp) );

    return test;
}
