#include <boost/config.hpp>

#if defined(BOOST_MSVC)
#pragma warning(disable: 4786)  // identifier truncated in debug info
#pragma warning(disable: 4710)  // function not inlined
#pragma warning(disable: 4711)  // function selected for automatic inline expansion
#pragma warning(disable: 4514)  // unreferenced inline removed
#endif

//
//  bind_fn2_test.cpp - test for functions w/ the type<> syntax
//
//  Copyright (c) 2005, 2008 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/bind.hpp>

#if defined(BOOST_MSVC) && (BOOST_MSVC < 1300)
#pragma warning(push, 3)
#endif

#include <iostream>

#if defined(BOOST_MSVC) && (BOOST_MSVC < 1300)
#pragma warning(pop)
#endif

#include <boost/detail/lightweight_test.hpp>

long global_result;

// long

long f_0()
{
    return global_result = 17041L;
}

long f_1(long a)
{
    return global_result = a;
}

long f_2(long a, long b)
{
    return global_result = a + 10 * b;
}

long f_3(long a, long b, long c)
{
    return global_result = a + 10 * b + 100 * c;
}

long f_4(long a, long b, long c, long d)
{
    return global_result = a + 10 * b + 100 * c + 1000 * d;
}

long f_5(long a, long b, long c, long d, long e)
{
    return global_result = a + 10 * b + 100 * c + 1000 * d + 10000 * e;
}

long f_6(long a, long b, long c, long d, long e, long f)
{
    return global_result = a + 10 * b + 100 * c + 1000 * d + 10000 * e + 100000 * f;
}

long f_7(long a, long b, long c, long d, long e, long f, long g)
{
    return global_result = a + 10 * b + 100 * c + 1000 * d + 10000 * e + 100000 * f + 1000000 * g;
}

long f_8(long a, long b, long c, long d, long e, long f, long g, long h)
{
    return global_result = a + 10 * b + 100 * c + 1000 * d + 10000 * e + 100000 * f + 1000000 * g + 10000000 * h;
}

long f_9(long a, long b, long c, long d, long e, long f, long g, long h, long i)
{
    return global_result = a + 10 * b + 100 * c + 1000 * d + 10000 * e + 100000 * f + 1000000 * g + 10000000 * h + 100000000 * i;
}

// void

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

void function_test()
{
    using namespace boost;

    bind( type<void>(), f_0 )(); BOOST_TEST( global_result == 17041L );
    bind( type<void>(), f_1, 1 )(); BOOST_TEST( global_result == 1L );
    bind( type<void>(), f_2, 1, 2 )(); BOOST_TEST( global_result == 21L );
    bind( type<void>(), f_3, 1, 2, 3 )(); BOOST_TEST( global_result == 321L );
    bind( type<void>(), f_4, 1, 2, 3, 4 )(); BOOST_TEST( global_result == 4321L );
    bind( type<void>(), f_5, 1, 2, 3, 4, 5 )(); BOOST_TEST( global_result == 54321L );
    bind( type<void>(), f_6, 1, 2, 3, 4, 5, 6 )(); BOOST_TEST( global_result == 654321L );
    bind( type<void>(), f_7, 1, 2, 3, 4, 5, 6, 7 )(); BOOST_TEST( global_result == 7654321L );
    bind( type<void>(), f_8, 1, 2, 3, 4, 5, 6, 7, 8 )(); BOOST_TEST( global_result == 87654321L );
    bind( type<void>(), f_9, 1, 2, 3, 4, 5, 6, 7, 8, 9 )(); BOOST_TEST( global_result == 987654321L );

    bind( type<void>(), fv_0 )(); BOOST_TEST( global_result == 17041L );
    bind( type<void>(), fv_1, 1 )(); BOOST_TEST( global_result == 1L );
    bind( type<void>(), fv_2, 1, 2 )(); BOOST_TEST( global_result == 21L );
    bind( type<void>(), fv_3, 1, 2, 3 )(); BOOST_TEST( global_result == 321L );
    bind( type<void>(), fv_4, 1, 2, 3, 4 )(); BOOST_TEST( global_result == 4321L );
    bind( type<void>(), fv_5, 1, 2, 3, 4, 5 )(); BOOST_TEST( global_result == 54321L );
    bind( type<void>(), fv_6, 1, 2, 3, 4, 5, 6 )(); BOOST_TEST( global_result == 654321L );
    bind( type<void>(), fv_7, 1, 2, 3, 4, 5, 6, 7 )(); BOOST_TEST( global_result == 7654321L );
    bind( type<void>(), fv_8, 1, 2, 3, 4, 5, 6, 7, 8 )(); BOOST_TEST( global_result == 87654321L );
    bind( type<void>(), fv_9, 1, 2, 3, 4, 5, 6, 7, 8, 9 )(); BOOST_TEST( global_result == 987654321L );
}

int main()
{
    function_test();
    return boost::report_errors();
}
