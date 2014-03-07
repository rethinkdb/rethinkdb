/*==============================================================================
    Copyright (c) 2005, 2008 Peter Dimov
    Copyright (c) 2005-2010 Joel de Guzman
    Copyright (c) 2010 Thomas Heller

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#include <boost/config.hpp>

#if defined(BOOST_MSVC)
#pragma warning(disable: 4786)  // identifier truncated in debug info
#pragma warning(disable: 4710)  // function not inlined
#pragma warning(disable: 4711)  // function selected for automatic inline expansion
#pragma warning(disable: 4514)  // unreferenced inline removed
#endif

#include <boost/phoenix/core.hpp>
#include <boost/phoenix/bind.hpp>

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

    typedef int result_type;

    X(): hash(0) {}

    int operator()() const { operator()(17); return 0; }
    int operator()(int a1) const { hash = (hash * 17041 + a1 * 2) % 32768; return 0; }
    int operator()(int a1, int a2) const { operator()(a1); operator()(a2); return 0; }
    int operator()(int a1, int a2, int a3) const { operator()(a1, a2); operator()(a3); return 0; }
    int operator()(int a1, int a2, int a3, int a4) const { operator()(a1, a2, a3); operator()(a4); return 0; }
    int operator()(int a1, int a2, int a3, int a4, int a5) const { operator()(a1, a2, a3, a4); operator()(a5); return 0; }
    int operator()(int a1, int a2, int a3, int a4, int a5, int a6) const { operator()(a1, a2, a3, a4, a5); operator()(a6); return 0; }
    int operator()(int a1, int a2, int a3, int a4, int a5, int a6, int a7) const { operator()(a1, a2, a3, a4, a5, a6); operator()(a7); return 0; }
    int operator()(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8) const { operator()(a1, a2, a3, a4, a5, a6, a7); operator()(a8); return 0; }
    int operator()(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9) const { operator()(a1, a2, a3, a4, a5, a6, a7, a8); operator()(a9); return 0; }
};

void function_object_test()
{
    using boost::phoenix::bind;
    using boost::phoenix::ref;

    X x;

    bind(ref(x) )();
    bind(ref(x), 1 )();
    bind(ref(x), 1, 2 )();
    bind(ref(x), 1, 2, 3 )();
    bind(ref(x), 1, 2, 3, 4 )();
    bind(ref(x), 1, 2, 3, 4, 5 )();
    bind(ref(x), 1, 2, 3, 4, 5, 6 )();
    bind(ref(x), 1, 2, 3, 4, 5, 6, 7)();
    bind(ref(x), 1, 2, 3, 4, 5, 6, 7, 8 )();
    bind(ref(x), 1, 2, 3, 4, 5, 6, 7, 8, 9 )();

    BOOST_TEST( x.hash == 9932 );
}

int main()
{
    function_object_test();
    return boost::report_errors();
}
