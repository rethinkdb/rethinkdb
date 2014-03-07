#include <boost/config.hpp>

#if defined(BOOST_MSVC)
#pragma warning(disable: 4786)  // identifier truncated in debug info
#pragma warning(disable: 4710)  // function not inlined
#pragma warning(disable: 4711)  // function selected for automatic inline expansion
#pragma warning(disable: 4514)  // unreferenced inline removed
#endif

//
//  bind_const_test.cpp - test const bind objects
//
//  Copyright (c) 2001-2004 Peter Dimov and Multi Media Ltd.
//  Copyright (c) 2001 David Abrahams
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/bind.hpp>
#include <boost/ref.hpp>

#if defined(BOOST_MSVC) && (BOOST_MSVC < 1300)
#pragma warning(push, 3)
#endif

#include <iostream>

#if defined(BOOST_MSVC) && (BOOST_MSVC < 1300)
#pragma warning(pop)
#endif

#include <boost/detail/lightweight_test.hpp>

//

long f_0()
{
    return 17041L;
}

long f_1(long a)
{
    return a;
}

long f_2(long a, long b)
{
    return a + 10 * b;
}

long f_3(long a, long b, long c)
{
    return a + 10 * b + 100 * c;
}

long f_4(long a, long b, long c, long d)
{
    return a + 10 * b + 100 * c + 1000 * d;
}

long f_5(long a, long b, long c, long d, long e)
{
    return a + 10 * b + 100 * c + 1000 * d + 10000 * e;
}

long f_6(long a, long b, long c, long d, long e, long f)
{
    return a + 10 * b + 100 * c + 1000 * d + 10000 * e + 100000 * f;
}

long f_7(long a, long b, long c, long d, long e, long f, long g)
{
    return a + 10 * b + 100 * c + 1000 * d + 10000 * e + 100000 * f + 1000000 * g;
}

long f_8(long a, long b, long c, long d, long e, long f, long g, long h)
{
    return a + 10 * b + 100 * c + 1000 * d + 10000 * e + 100000 * f + 1000000 * g + 10000000 * h;
}

long f_9(long a, long b, long c, long d, long e, long f, long g, long h, long i)
{
    return a + 10 * b + 100 * c + 1000 * d + 10000 * e + 100000 * f + 1000000 * g + 10000000 * h + 100000000 * i;
}

long global_result;

void fv_0()
{
    global_result = 17041L;
}

void fv_1(long a)
{
    global_result = a;
}

void fv_2(long a, long b)
{
    global_result = a + 10 * b;
}

void fv_3(long a, long b, long c)
{
    global_result = a + 10 * b + 100 * c;
}

void fv_4(long a, long b, long c, long d)
{
    global_result = a + 10 * b + 100 * c + 1000 * d;
}

void fv_5(long a, long b, long c, long d, long e)
{
    global_result = a + 10 * b + 100 * c + 1000 * d + 10000 * e;
}

void fv_6(long a, long b, long c, long d, long e, long f)
{
    global_result = a + 10 * b + 100 * c + 1000 * d + 10000 * e + 100000 * f;
}

void fv_7(long a, long b, long c, long d, long e, long f, long g)
{
    global_result = a + 10 * b + 100 * c + 1000 * d + 10000 * e + 100000 * f + 1000000 * g;
}

void fv_8(long a, long b, long c, long d, long e, long f, long g, long h)
{
    global_result = a + 10 * b + 100 * c + 1000 * d + 10000 * e + 100000 * f + 1000000 * g + 10000000 * h;
}

void fv_9(long a, long b, long c, long d, long e, long f, long g, long h, long i)
{
    global_result = a + 10 * b + 100 * c + 1000 * d + 10000 * e + 100000 * f + 1000000 * g + 10000000 * h + 100000000 * i;
}

template<class F, class A> long test(F const & f, A const & a)
{
    return f(a);
}

template<class F, class A> long testv(F const & f, A const & a)
{
    f(a);
    return global_result;
}

void function_test()
{
    using namespace boost;

    int const i = 1;

    BOOST_TEST( test( bind(f_0), i ) == 17041L );
    BOOST_TEST( test( bind(f_1, _1), i ) == 1L );
    BOOST_TEST( test( bind(f_2, _1, 2), i ) == 21L );
    BOOST_TEST( test( bind(f_3, _1, 2, 3), i ) == 321L );
    BOOST_TEST( test( bind(f_4, _1, 2, 3, 4), i ) == 4321L );
    BOOST_TEST( test( bind(f_5, _1, 2, 3, 4, 5), i ) == 54321L );
    BOOST_TEST( test( bind(f_6, _1, 2, 3, 4, 5, 6), i ) == 654321L );
    BOOST_TEST( test( bind(f_7, _1, 2, 3, 4, 5, 6, 7), i ) == 7654321L );
    BOOST_TEST( test( bind(f_8, _1, 2, 3, 4, 5, 6, 7, 8), i ) == 87654321L );
    BOOST_TEST( test( bind(f_9, _1, 2, 3, 4, 5, 6, 7, 8, 9), i ) == 987654321L );

    BOOST_TEST( testv( bind(fv_0), i ) == 17041L );
    BOOST_TEST( testv( bind(fv_1, _1), i ) == 1L );
    BOOST_TEST( testv( bind(fv_2, _1, 2), i ) == 21L );
    BOOST_TEST( testv( bind(fv_3, _1, 2, 3), i ) == 321L );
    BOOST_TEST( testv( bind(fv_4, _1, 2, 3, 4), i ) == 4321L );
    BOOST_TEST( testv( bind(fv_5, _1, 2, 3, 4, 5), i ) == 54321L );
    BOOST_TEST( testv( bind(fv_6, _1, 2, 3, 4, 5, 6), i ) == 654321L );
    BOOST_TEST( testv( bind(fv_7, _1, 2, 3, 4, 5, 6, 7), i ) == 7654321L );
    BOOST_TEST( testv( bind(fv_8, _1, 2, 3, 4, 5, 6, 7, 8), i ) == 87654321L );
    BOOST_TEST( testv( bind(fv_9, _1, 2, 3, 4, 5, 6, 7, 8, 9), i ) == 987654321L );
}

int main()
{
    function_test();
    return boost::report_errors();
}
