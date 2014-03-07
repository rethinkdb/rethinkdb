#include <boost/config.hpp>

//  wp_convertible_test.cpp
//
//  Copyright (c) 2008 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include <boost/detail/lightweight_test.hpp>
#include <boost/weak_ptr.hpp>

//

class incomplete;

struct X
{
};

struct Y
{
};

struct Z: public X
{
};

int f( boost::weak_ptr<void const> )
{
    return 1;
}

int f( boost::weak_ptr<int> )
{
    return 2;
}

int f( boost::weak_ptr<incomplete> )
{
    return 3;
}

int g( boost::weak_ptr<X> )
{
    return 4;
}

int g( boost::weak_ptr<Y> )
{
    return 5;
}

int g( boost::weak_ptr<incomplete> )
{
    return 6;
}

int main()
{
    BOOST_TEST( 1 == f( boost::weak_ptr<double>() ) );
    BOOST_TEST( 1 == f( boost::shared_ptr<double>() ) );
    BOOST_TEST( 4 == g( boost::weak_ptr<Z>() ) );
    BOOST_TEST( 4 == g( boost::shared_ptr<Z>() ) );

    return boost::report_errors();
}
