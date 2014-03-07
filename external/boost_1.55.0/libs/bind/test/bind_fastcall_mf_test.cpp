#include <boost/config.hpp>

#if defined(BOOST_MSVC)
#pragma warning(disable: 4786)  // identifier truncated in debug info
#pragma warning(disable: 4710)  // function not inlined
#pragma warning(disable: 4711)  // function selected for automatic inline expansion
#pragma warning(disable: 4514)  // unreferenced inline removed
#endif

//
//  bind_stdcall_mf_test.cpp - test for bind.hpp + __stdcall (member functions)
//
//  Copyright (c) 2001 Peter Dimov and Multi Media Ltd.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define BOOST_MEM_FN_ENABLE_FASTCALL

#include <boost/bind.hpp>

#if defined(BOOST_MSVC) && (BOOST_MSVC < 1300)
#pragma warning(push, 3)
#endif

#include <iostream>

#if defined(BOOST_MSVC) && (BOOST_MSVC < 1300)
#pragma warning(pop)
#endif

#include <boost/detail/lightweight_test.hpp>

struct X
{
    mutable unsigned int hash;

    X(): hash(0) {}

    void __fastcall f0() { f1(17); }
    void __fastcall g0() const { g1(17); }

    void __fastcall f1(int a1) { hash = (hash * 17041 + a1) % 32768; }
    void __fastcall g1(int a1) const { hash = (hash * 17041 + a1 * 2) % 32768; }

    void __fastcall f2(int a1, int a2) { f1(a1); f1(a2); }
    void __fastcall g2(int a1, int a2) const { g1(a1); g1(a2); }

    void __fastcall f3(int a1, int a2, int a3) { f2(a1, a2); f1(a3); }
    void __fastcall g3(int a1, int a2, int a3) const { g2(a1, a2); g1(a3); }

    void __fastcall f4(int a1, int a2, int a3, int a4) { f3(a1, a2, a3); f1(a4); }
    void __fastcall g4(int a1, int a2, int a3, int a4) const { g3(a1, a2, a3); g1(a4); }

    void __fastcall f5(int a1, int a2, int a3, int a4, int a5) { f4(a1, a2, a3, a4); f1(a5); }
    void __fastcall g5(int a1, int a2, int a3, int a4, int a5) const { g4(a1, a2, a3, a4); g1(a5); }

    void __fastcall f6(int a1, int a2, int a3, int a4, int a5, int a6) { f5(a1, a2, a3, a4, a5); f1(a6); }
    void __fastcall g6(int a1, int a2, int a3, int a4, int a5, int a6) const { g5(a1, a2, a3, a4, a5); g1(a6); }

    void __fastcall f7(int a1, int a2, int a3, int a4, int a5, int a6, int a7) { f6(a1, a2, a3, a4, a5, a6); f1(a7); }
    void __fastcall g7(int a1, int a2, int a3, int a4, int a5, int a6, int a7) const { g6(a1, a2, a3, a4, a5, a6); g1(a7); }

    void __fastcall f8(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8) { f7(a1, a2, a3, a4, a5, a6, a7); f1(a8); }
    void __fastcall g8(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8) const { g7(a1, a2, a3, a4, a5, a6, a7); g1(a8); }
};

void member_function_test()
{
    using namespace boost;

    X x;

    // 0

    bind(&X::f0, &x)();
    bind(&X::f0, ref(x))();

    bind(&X::g0, &x)();
    bind(&X::g0, x)();
    bind(&X::g0, ref(x))();

    // 1

    bind(&X::f1, &x, 1)();
    bind(&X::f1, ref(x), 1)();

    bind(&X::g1, &x, 1)();
    bind(&X::g1, x, 1)();
    bind(&X::g1, ref(x), 1)();

    // 2

    bind(&X::f2, &x, 1, 2)();
    bind(&X::f2, ref(x), 1, 2)();

    bind(&X::g2, &x, 1, 2)();
    bind(&X::g2, x, 1, 2)();
    bind(&X::g2, ref(x), 1, 2)();

    // 3

    bind(&X::f3, &x, 1, 2, 3)();
    bind(&X::f3, ref(x), 1, 2, 3)();

    bind(&X::g3, &x, 1, 2, 3)();
    bind(&X::g3, x, 1, 2, 3)();
    bind(&X::g3, ref(x), 1, 2, 3)();

    // 4

    bind(&X::f4, &x, 1, 2, 3, 4)();
    bind(&X::f4, ref(x), 1, 2, 3, 4)();

    bind(&X::g4, &x, 1, 2, 3, 4)();
    bind(&X::g4, x, 1, 2, 3, 4)();
    bind(&X::g4, ref(x), 1, 2, 3, 4)();

    // 5

    bind(&X::f5, &x, 1, 2, 3, 4, 5)();
    bind(&X::f5, ref(x), 1, 2, 3, 4, 5)();

    bind(&X::g5, &x, 1, 2, 3, 4, 5)();
    bind(&X::g5, x, 1, 2, 3, 4, 5)();
    bind(&X::g5, ref(x), 1, 2, 3, 4, 5)();

    // 6

    bind(&X::f6, &x, 1, 2, 3, 4, 5, 6)();
    bind(&X::f6, ref(x), 1, 2, 3, 4, 5, 6)();

    bind(&X::g6, &x, 1, 2, 3, 4, 5, 6)();
    bind(&X::g6, x, 1, 2, 3, 4, 5, 6)();
    bind(&X::g6, ref(x), 1, 2, 3, 4, 5, 6)();

    // 7

    bind(&X::f7, &x, 1, 2, 3, 4, 5, 6, 7)();
    bind(&X::f7, ref(x), 1, 2, 3, 4, 5, 6, 7)();

    bind(&X::g7, &x, 1, 2, 3, 4, 5, 6, 7)();
    bind(&X::g7, x, 1, 2, 3, 4, 5, 6, 7)();
    bind(&X::g7, ref(x), 1, 2, 3, 4, 5, 6, 7)();

    // 8

    bind(&X::f8, &x, 1, 2, 3, 4, 5, 6, 7, 8)();
    bind(&X::f8, ref(x), 1, 2, 3, 4, 5, 6, 7, 8)();

    bind(&X::g8, &x, 1, 2, 3, 4, 5, 6, 7, 8)();
    bind(&X::g8, x, 1, 2, 3, 4, 5, 6, 7, 8)();
    bind(&X::g8, ref(x), 1, 2, 3, 4, 5, 6, 7, 8)();

    BOOST_TEST( x.hash == 23558 );
}

int main()
{
    member_function_test();
    return boost::report_errors();
}
