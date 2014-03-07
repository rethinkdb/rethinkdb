#include <boost/config.hpp>

#if defined(BOOST_MSVC)
# pragma warning(disable: 4786)  // identifier truncated in debug info
# pragma warning(disable: 4710)  // function not inlined
# pragma warning(disable: 4711)  // function selected for automatic inline expansion
# pragma warning(disable: 4514)  // unreferenced inline removed
# pragma warning(disable: 4100)  // unreferenced formal parameter (it is referenced!)
#endif

// Copyright (c) 2006 Douglas Gregor <doug.gregor@gmail.com>
// Copyright (c) 2006 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/bind.hpp>
#include <boost/visit_each.hpp>

#if defined(BOOST_MSVC) && (BOOST_MSVC < 1300)
# pragma warning(push, 3)
#endif

#include <iostream>
#include <typeinfo>

#if defined(BOOST_MSVC) && (BOOST_MSVC < 1300)
# pragma warning(pop)
#endif

#include <boost/detail/lightweight_test.hpp>

struct visitor
{
    int hash;

    visitor(): hash( 0 )
    {
    }

    template<typename T> void operator()( T const & t )
    {
        std::cout << "visitor::operator()( T ): " << typeid( t ).name() << std::endl;
    }

    void operator()( int const & t )
    {
        std::cout << "visitor::operator()( int ): " << t << std::endl;
        hash = hash * 10 + t;
    }
};

int f( int x, int y, int z )
{
    return x + y + z;
}

int main()
{
    visitor vis;

    boost::visit_each( vis, boost::bind( f, 3, _1, 4 ) );

    BOOST_TEST( vis.hash == 34 );

    return boost::report_errors();
}
