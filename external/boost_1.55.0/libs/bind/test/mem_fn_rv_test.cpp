#include <boost/config.hpp>

#if defined(BOOST_MSVC)
#pragma warning(disable: 4786)  // identifier truncated in debug info
#pragma warning(disable: 4710)  // function not inlined
#pragma warning(disable: 4711)  // function selected for automatic inline expansion
#pragma warning(disable: 4514)  // unreferenced inline removed
#endif

//
//  mem_fn_test.cpp - mem_fn.hpp with rvalues
//
//  Copyright (c) 2001, 2002 Peter Dimov and Multi Media Ltd.
//  Copyright (c) 2005 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/mem_fn.hpp>
#include <boost/shared_ptr.hpp>

#if defined(BOOST_MSVC) && (BOOST_MSVC < 1300)
#pragma warning(push, 3)
#endif

#include <iostream>

#if defined(BOOST_MSVC) && (BOOST_MSVC < 1300)
#pragma warning(pop)
#endif

unsigned int hash = 0;

struct X
{
    int f0() { f1(17); return 0; }
    int g0() const { g1(17); return 0; }

    int f1(int a1) { hash = (hash * 17041 + a1) % 32768; return 0; }
    int g1(int a1) const { hash = (hash * 17041 + a1 * 2) % 32768; return 0; }

    int f2(int a1, int a2) { f1(a1); f1(a2); return 0; }
    int g2(int a1, int a2) const { g1(a1); g1(a2); return 0; }

    int f3(int a1, int a2, int a3) { f2(a1, a2); f1(a3); return 0; }
    int g3(int a1, int a2, int a3) const { g2(a1, a2); g1(a3); return 0; }

    int f4(int a1, int a2, int a3, int a4) { f3(a1, a2, a3); f1(a4); return 0; }
    int g4(int a1, int a2, int a3, int a4) const { g3(a1, a2, a3); g1(a4); return 0; }

    int f5(int a1, int a2, int a3, int a4, int a5) { f4(a1, a2, a3, a4); f1(a5); return 0; }
    int g5(int a1, int a2, int a3, int a4, int a5) const { g4(a1, a2, a3, a4); g1(a5); return 0; }

    int f6(int a1, int a2, int a3, int a4, int a5, int a6) { f5(a1, a2, a3, a4, a5); f1(a6); return 0; }
    int g6(int a1, int a2, int a3, int a4, int a5, int a6) const { g5(a1, a2, a3, a4, a5); g1(a6); return 0; }

    int f7(int a1, int a2, int a3, int a4, int a5, int a6, int a7) { f6(a1, a2, a3, a4, a5, a6); f1(a7); return 0; }
    int g7(int a1, int a2, int a3, int a4, int a5, int a6, int a7) const { g6(a1, a2, a3, a4, a5, a6); g1(a7); return 0; }

    int f8(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8) { f7(a1, a2, a3, a4, a5, a6, a7); f1(a8); return 0; }
    int g8(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8) const { g7(a1, a2, a3, a4, a5, a6, a7); g1(a8); return 0; }
};

int detect_errors(bool x)
{
    if( x )
    {
        std::cerr << "no errors detected.\n";
        return 0;
    }
    else
    {
        std::cerr << "test failed.\n";
        return 1;
    }
}

boost::shared_ptr<X> make()
{
    return boost::shared_ptr<X>( new X );
}

int main()
{
    using boost::mem_fn;

    mem_fn(&X::f0)( make() );
    mem_fn(&X::g0)( make() );

    mem_fn(&X::f1)( make(), 1 );
    mem_fn(&X::g1)( make(), 1 );

    mem_fn(&X::f2)( make(), 1, 2 );
    mem_fn(&X::g2)( make(), 1, 2 );

    mem_fn(&X::f3)( make(), 1, 2, 3 );
    mem_fn(&X::g3)( make(), 1, 2, 3 );

    mem_fn(&X::f4)( make(), 1, 2, 3, 4 );
    mem_fn(&X::g4)( make(), 1, 2, 3, 4 );

    mem_fn(&X::f5)( make(), 1, 2, 3, 4, 5 );
    mem_fn(&X::g5)( make(), 1, 2, 3, 4, 5 );

    mem_fn(&X::f6)( make(), 1, 2, 3, 4, 5, 6 );
    mem_fn(&X::g6)( make(), 1, 2, 3, 4, 5, 6 );

    mem_fn(&X::f7)( make(), 1, 2, 3, 4, 5, 6, 7 );
    mem_fn(&X::g7)( make(), 1, 2, 3, 4, 5, 6, 7 );

    mem_fn(&X::f8)( make(), 1, 2, 3, 4, 5, 6, 7, 8 );
    mem_fn(&X::g8)( make(), 1, 2, 3, 4, 5, 6, 7, 8 );

    return detect_errors( hash == 2155 );
}
