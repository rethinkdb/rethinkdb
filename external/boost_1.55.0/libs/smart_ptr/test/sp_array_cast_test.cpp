//
//  sp_array_cast_test.cpp
//
//  Copyright (c) 2012 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//

#include <boost/shared_ptr.hpp>
#include <boost/detail/lightweight_test.hpp>

struct X
{
};

void static_cast_test()
{
    {
        boost::shared_ptr<void> pv;

        boost::shared_ptr<int[]> pi = boost::static_pointer_cast<int[]>( pv );
        BOOST_TEST( pi.get() == 0 );

        boost::shared_ptr<int[3]> pi2 = boost::static_pointer_cast<int[3]>( pv );
        BOOST_TEST( pi2.get() == 0 );

        boost::shared_ptr<X[]> px = boost::static_pointer_cast<X[]>( pv );
        BOOST_TEST( px.get() == 0 );

        boost::shared_ptr<X[5]> px2 = boost::static_pointer_cast<X[5]>( pv );
        BOOST_TEST( px2.get() == 0 );
    }

    {
        boost::shared_ptr<int[]> pi( new int[2] );
        boost::shared_ptr<void> pv( pi );

        boost::shared_ptr<int[]> pi2 = boost::static_pointer_cast<int[]>( pv );
        BOOST_TEST(pi.get() == pi2.get());
        BOOST_TEST(!(pi < pi2 || pi2 < pi));

        boost::shared_ptr<int[2]> pi3 = boost::static_pointer_cast<int[2]>( pv );
        BOOST_TEST(pi.get() == pi3.get());
        BOOST_TEST(!(pi < pi3 || pi3 < pi));

        boost::shared_ptr<void> pv2( pi3 );

        boost::shared_ptr<int[]> pi4 = boost::static_pointer_cast<int[]>( pv2 );
        BOOST_TEST(pi.get() == pi4.get());
        BOOST_TEST(!(pi < pi4 || pi4 < pi));
    }

    {
        boost::shared_ptr<X[]> px( new X[4] );
        boost::shared_ptr<void> pv( px );

        boost::shared_ptr<X[]> px2 = boost::static_pointer_cast<X[]>( pv );
        BOOST_TEST(px.get() == px2.get());
        BOOST_TEST(!(px < px2 || px2 < px));

        boost::shared_ptr<X[4]> px3 = boost::static_pointer_cast<X[4]>( pv );
        BOOST_TEST(px.get() == px3.get());
        BOOST_TEST(!(px < px3 || px3 < px));

        boost::shared_ptr<void> pv2( px3 );

        boost::shared_ptr<X[]> px4 = boost::static_pointer_cast<X[]>( pv2 );
        BOOST_TEST(px.get() == px4.get());
        BOOST_TEST(!(px < px4 || px4 < px));
    }
}

void const_cast_test()
{
    {
        boost::shared_ptr<int const volatile[]> px;

        boost::shared_ptr<int[]> px2 = boost::const_pointer_cast<int[]>(px);
        BOOST_TEST( px2.get() == 0 );
    }

    {
        boost::shared_ptr<int const volatile[2]> px;

        boost::shared_ptr<int[2]> px2 = boost::const_pointer_cast<int[2]>(px);
        BOOST_TEST( px2.get() == 0 );
    }

    {
        boost::shared_ptr<X const volatile[]> px;

        boost::shared_ptr<X[]> px2 = boost::const_pointer_cast<X[]>(px);
        BOOST_TEST( px2.get() == 0 );
    }

    {
        boost::shared_ptr<X const volatile[5]> px;

        boost::shared_ptr<X[5]> px2 = boost::const_pointer_cast<X[5]>(px);
        BOOST_TEST( px2.get() == 0 );
    }

    {
        boost::shared_ptr<int const volatile[]> px( new int[3] );

        boost::shared_ptr<int[]> px2 = boost::const_pointer_cast<int[]>(px);
        BOOST_TEST(px.get() == px2.get());
        BOOST_TEST(!(px < px2 || px2 < px));
    }

    {
        boost::shared_ptr<int const volatile[3]> px( new int[3] );

        boost::shared_ptr<int[3]> px2 = boost::const_pointer_cast<int[3]>(px);
        BOOST_TEST(px.get() == px2.get());
        BOOST_TEST(!(px < px2 || px2 < px));
    }

    {
        boost::shared_ptr<X const volatile[]> px( new X[4] );

        boost::shared_ptr<X[]> px2 = boost::const_pointer_cast<X[]>(px);
        BOOST_TEST(px.get() == px2.get());
        BOOST_TEST(!(px < px2 || px2 < px));
    }

    {
        boost::shared_ptr<X const volatile[4]> px( new X[4] );

        boost::shared_ptr<X[4]> px2 = boost::const_pointer_cast<X[4]>(px);
        BOOST_TEST(px.get() == px2.get());
        BOOST_TEST(!(px < px2 || px2 < px));
    }
}

void reinterpret_cast_test()
{
    {
        boost::shared_ptr<int[]> pi;
        BOOST_TEST( pi.get() == 0 );

        boost::shared_ptr<int[3]> pi2 = boost::reinterpret_pointer_cast<int[3]>( pi );
        BOOST_TEST( pi2.get() == 0 );

        boost::shared_ptr<int[6]> pi3 = boost::reinterpret_pointer_cast<int[6]>( pi2 );
        BOOST_TEST( pi3.get() == 0 );
    }

    {
        boost::shared_ptr<X[]> px;
        BOOST_TEST( px.get() == 0 );

        boost::shared_ptr<X[5]> px2 = boost::reinterpret_pointer_cast<X[5]>( px );
        BOOST_TEST( px2.get() == 0 );

        boost::shared_ptr<X[9]> px3 = boost::reinterpret_pointer_cast<X[9]>( px2 );
        BOOST_TEST( px3.get() == 0 );
    }

    {
        boost::shared_ptr<int[]> pi( new int[2] );

        boost::shared_ptr<int[2]> pi2 = boost::reinterpret_pointer_cast<int[2]>( pi );
        BOOST_TEST(pi.get() == pi2.get());
        BOOST_TEST(!(pi < pi2 || pi2 < pi));

        boost::shared_ptr<int[1]> pi3 = boost::reinterpret_pointer_cast<int[1]>( pi2 );
        BOOST_TEST(pi.get() == pi3.get());
        BOOST_TEST(!(pi < pi3 || pi3 < pi));

        boost::shared_ptr<int[]> pi4 = boost::reinterpret_pointer_cast<int[]>( pi3 );
        BOOST_TEST(pi.get() == pi4.get());
        BOOST_TEST(!(pi < pi4 || pi4 < pi));
    }

    {
        boost::shared_ptr<X[]> px( new X[4] );

        boost::shared_ptr<X[7]> px2 = boost::reinterpret_pointer_cast<X[7]>( px );
        BOOST_TEST(px.get() == px2.get());
        BOOST_TEST(!(px < px2 || px2 < px));

        boost::shared_ptr<X[4]> px3 = boost::reinterpret_pointer_cast<X[4]>( px2 );
        BOOST_TEST(px.get() == px3.get());
        BOOST_TEST(!(px < px3 || px3 < px));

        boost::shared_ptr<X[]> px4 = boost::reinterpret_pointer_cast<X[]>( px3 );
        BOOST_TEST(px.get() == px4.get());
        BOOST_TEST(!(px < px4 || px4 < px));
    }
}

int main()
{
    static_cast_test();
    const_cast_test();
    reinterpret_cast_test();

    return boost::report_errors();
}
