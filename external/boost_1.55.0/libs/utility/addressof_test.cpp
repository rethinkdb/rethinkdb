// Copyright (C) 2002 Brad King (brad.king@kitware.com) 
//                    Douglas Gregor (gregod@cs.rpi.edu)
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org


#include <boost/utility/addressof.hpp>

#if defined(BOOST_MSVC) && (BOOST_MSVC < 1300)
#pragma warning(push, 3)
#endif

#include <iostream>

#if defined(BOOST_MSVC) && (BOOST_MSVC < 1300)
#pragma warning(pop)
#endif

#include <boost/detail/lightweight_test.hpp>

template<class T> void scalar_test( T * = 0 )
{
    T* px = new T();

    T& x = *px;
    BOOST_TEST( boost::addressof(x) == px );

    const T& cx = *px;
    const T* pcx = boost::addressof(cx);
    BOOST_TEST( pcx == px );

    volatile T& vx = *px;
    volatile T* pvx = boost::addressof(vx);
    BOOST_TEST( pvx == px );

    const volatile T& cvx = *px;
    const volatile T* pcvx = boost::addressof(cvx);
    BOOST_TEST( pcvx == px );

    delete px;
}

template<class T> void array_test( T * = 0 )
{
    T nrg[3] = {1,2,3};
    T (*pnrg)[3] = &nrg;
    BOOST_TEST( boost::addressof(nrg) == pnrg );

    T const cnrg[3] = {1,2,3};
    T const (*pcnrg)[3] = &cnrg;
    BOOST_TEST( boost::addressof(cnrg) == pcnrg );
}

struct addressable
{
    addressable( int = 0 )
    {
    }
};

struct useless_type {};

class nonaddressable {
public:

    nonaddressable( int = 0 )
    {
    }

    void dummy(); // Silence GCC warning: all member of class are private

private:

    useless_type operator&() const;
};

int main()
{
    scalar_test<char>();
    scalar_test<int>();
    scalar_test<addressable>();
    scalar_test<nonaddressable>();

    array_test<char>();
    array_test<int>();
    array_test<addressable>();
    array_test<nonaddressable>();

    return boost::report_errors();
}
