#include <boost/config.hpp>

#if defined(BOOST_MSVC)
#pragma warning(disable: 4786)  // identifier truncated in debug info
#pragma warning(disable: 4710)  // function not inlined
#pragma warning(disable: 4711)  // function selected for automatic inline expansion
#pragma warning(disable: 4514)  // unreferenced inline removed
#endif

//  ref_fn_test.cpp: ref( f )
//
//  Copyright (c) 2008 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include <boost/ref.hpp>
#include <boost/detail/lightweight_test.hpp>


void f0()
{
}

void f1(int)
{
}

void f2(int, int)
{
}

void f3(int, int, int)
{
}

void f4(int, int, int, int)
{
}

void f5(int, int, int, int, int)
{
}

void f6(int, int, int, int, int, int)
{
}

void f7(int, int, int, int, int, int, int)
{
}

void f8(int, int, int, int, int, int, int, int)
{
}

void f9(int, int, int, int, int, int, int, int, int)
{
}

#define BOOST_TEST_REF( f ) BOOST_TEST( &boost::ref( f ).get() == &f )

int main()
{
    int v = 0;
    BOOST_TEST_REF( v );

    BOOST_TEST_REF( f0 );
    BOOST_TEST_REF( f1 );
    BOOST_TEST_REF( f2 );
    BOOST_TEST_REF( f3 );
    BOOST_TEST_REF( f4 );
    BOOST_TEST_REF( f5 );
    BOOST_TEST_REF( f6 );
    BOOST_TEST_REF( f7 );
    BOOST_TEST_REF( f8 );
    BOOST_TEST_REF( f9 );

    return boost::report_errors();
}
