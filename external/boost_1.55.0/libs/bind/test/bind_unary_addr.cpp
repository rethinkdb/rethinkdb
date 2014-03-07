#include <boost/config.hpp>

#if defined(BOOST_MSVC)
#pragma warning(disable: 4786)  // identifier truncated in debug info
#pragma warning(disable: 4710)  // function not inlined
#pragma warning(disable: 4711)  // function selected for automatic inline expansion
#pragma warning(disable: 4514)  // unreferenced inline removed
#endif

//
//  bind_unary_addr.cpp
//
//  Copyright (c) 2005 Peter Dimov
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

class X
{
private:

    void operator& ();
    void operator& () const;

public:

    void operator()()
    {
    }

    void operator()() const
    {
    }

    void operator()(int)
    {
    }

    void operator()(int) const
    {
    }

    void operator()(int, int)
    {
    }

    void operator()(int, int) const
    {
    }

    void operator()(int, int, int)
    {
    }

    void operator()(int, int, int) const
    {
    }

    void operator()(int, int, int, int)
    {
    }

    void operator()(int, int, int, int) const
    {
    }

    void operator()(int, int, int, int, int)
    {
    }

    void operator()(int, int, int, int, int) const
    {
    }

    void operator()(int, int, int, int, int, int)
    {
    }

    void operator()(int, int, int, int, int, int) const
    {
    }

    void operator()(int, int, int, int, int, int, int)
    {
    }

    void operator()(int, int, int, int, int, int, int) const
    {
    }

    void operator()(int, int, int, int, int, int, int, int)
    {
    }

    void operator()(int, int, int, int, int, int, int, int) const
    {
    }

    void operator()(int, int, int, int, int, int, int, int, int)
    {
    }

    void operator()(int, int, int, int, int, int, int, int, int) const
    {
    }
};

template<class F> void test_const( F const & f )
{
    f();
}

template<class F> void test( F f )
{
    f();
    test_const( f );
}

int main()
{
    test( boost::bind<void>( X() ) );
    test( boost::bind<void>( X(), 1 ) );
    test( boost::bind<void>( X(), 1, 2 ) );
    test( boost::bind<void>( X(), 1, 2, 3 ) );
    test( boost::bind<void>( X(), 1, 2, 3, 4 ) );
    test( boost::bind<void>( X(), 1, 2, 3, 4, 5 ) );
    test( boost::bind<void>( X(), 1, 2, 3, 4, 5, 6 ) );
    test( boost::bind<void>( X(), 1, 2, 3, 4, 5, 6, 7 ) );
    test( boost::bind<void>( X(), 1, 2, 3, 4, 5, 6, 7, 8 ) );
    test( boost::bind<void>( X(), 1, 2, 3, 4, 5, 6, 7, 8, 9 ) );

    return 0;
}
