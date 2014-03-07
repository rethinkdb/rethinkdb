//
//  Copyright (C) 2008 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
#define BOOST_THREAD_VERSION 3

#include <boost/thread/thread_only.hpp>
#include <boost/detail/lightweight_test.hpp>

struct X
{
    mutable unsigned int hash;

    X(): hash(0) {}

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

int main()
{
    X x;

    // 0

    boost::thread( &X::f0, &x ).join();
    boost::thread( &X::f0, boost::ref(x) ).join();

    boost::thread( &X::g0, &x ).join();
    boost::thread( &X::g0, x ).join();
    boost::thread( &X::g0, boost::ref(x) ).join();

    // 1

    boost::thread( &X::f1, &x, 1 ).join();
    boost::thread( &X::f1, boost::ref(x), 1 ).join();

    boost::thread( &X::g1, &x, 1 ).join();
    boost::thread( &X::g1, x, 1 ).join();
    boost::thread( &X::g1, boost::ref(x), 1 ).join();

    // 2

    boost::thread( &X::f2, &x, 1, 2 ).join();
    boost::thread( &X::f2, boost::ref(x), 1, 2 ).join();

    boost::thread( &X::g2, &x, 1, 2 ).join();
    boost::thread( &X::g2, x, 1, 2 ).join();
    boost::thread( &X::g2, boost::ref(x), 1, 2 ).join();

    // 3

    boost::thread( &X::f3, &x, 1, 2, 3 ).join();
    boost::thread( &X::f3, boost::ref(x), 1, 2, 3 ).join();

    boost::thread( &X::g3, &x, 1, 2, 3 ).join();
    boost::thread( &X::g3, x, 1, 2, 3 ).join();
    boost::thread( &X::g3, boost::ref(x), 1, 2, 3 ).join();

    // 4

    boost::thread( &X::f4, &x, 1, 2, 3, 4 ).join();
    boost::thread( &X::f4, boost::ref(x), 1, 2, 3, 4 ).join();

    boost::thread( &X::g4, &x, 1, 2, 3, 4 ).join();
    boost::thread( &X::g4, x, 1, 2, 3, 4 ).join();
    boost::thread( &X::g4, boost::ref(x), 1, 2, 3, 4 ).join();

    // 5

    boost::thread( &X::f5, &x, 1, 2, 3, 4, 5 ).join();
    boost::thread( &X::f5, boost::ref(x), 1, 2, 3, 4, 5 ).join();

    boost::thread( &X::g5, &x, 1, 2, 3, 4, 5 ).join();
    boost::thread( &X::g5, x, 1, 2, 3, 4, 5 ).join();
    boost::thread( &X::g5, boost::ref(x), 1, 2, 3, 4, 5 ).join();

    // 6

    boost::thread( &X::f6, &x, 1, 2, 3, 4, 5, 6 ).join();
    boost::thread( &X::f6, boost::ref(x), 1, 2, 3, 4, 5, 6 ).join();

    boost::thread( &X::g6, &x, 1, 2, 3, 4, 5, 6 ).join();
    boost::thread( &X::g6, x, 1, 2, 3, 4, 5, 6 ).join();
    boost::thread( &X::g6, boost::ref(x), 1, 2, 3, 4, 5, 6 ).join();

    // 7

    boost::thread( &X::f7, &x, 1, 2, 3, 4, 5, 6, 7).join();
    boost::thread( &X::f7, boost::ref(x), 1, 2, 3, 4, 5, 6, 7).join();

    boost::thread( &X::g7, &x, 1, 2, 3, 4, 5, 6, 7).join();
    boost::thread( &X::g7, x, 1, 2, 3, 4, 5, 6, 7).join();
    boost::thread( &X::g7, boost::ref(x), 1, 2, 3, 4, 5, 6, 7).join();

    // 8

    boost::thread( &X::f8, &x, 1, 2, 3, 4, 5, 6, 7, 8 ).join();
    boost::thread( &X::f8, boost::ref(x), 1, 2, 3, 4, 5, 6, 7, 8 ).join();

    boost::thread( &X::g8, &x, 1, 2, 3, 4, 5, 6, 7, 8 ).join();
    boost::thread( &X::g8, x, 1, 2, 3, 4, 5, 6, 7, 8 ).join();
    boost::thread( &X::g8, boost::ref(x), 1, 2, 3, 4, 5, 6, 7, 8 ).join();

    BOOST_TEST( x.hash == 23558 );

    return boost::report_errors();
}
