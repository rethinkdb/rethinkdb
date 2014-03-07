// Boost.Function library

//  Copyright Douglas Gregor 2004.
//  Copyright 2005 Peter Dimov

//  Use, modification and distribution is subject to
//  the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <boost/function.hpp>
#include <boost/detail/lightweight_test.hpp>

static int forty_two()
{
    return 42;
}

struct Seventeen
{
    int operator()() const
    {
        return 17;
    }
};

bool operator==(const Seventeen&, const Seventeen&)
{
    return true;
}

struct ReturnInt
{
    explicit ReturnInt(int value) : value(value)
    {
    }

    int operator()() const
    {
        return value;
    }

    int value;
};

bool operator==(const ReturnInt& x, const ReturnInt& y)
{
    return x.value == y.value;
}

bool operator!=(const ReturnInt& x, const ReturnInt& y)
{
    return x.value != y.value;
}

int main()
{
    boost::function0<int> fn;

    fn = &forty_two;

    BOOST_TEST( fn() == 42 );

    BOOST_TEST( fn.contains(&forty_two) );
    BOOST_TEST( !fn.contains( Seventeen() ) );
    BOOST_TEST( !fn.contains( ReturnInt(0) ) );
    BOOST_TEST( !fn.contains( ReturnInt(12) ) );

    fn = Seventeen();

    BOOST_TEST( fn() == 17 );

    BOOST_TEST( !fn.contains( &forty_two ) );
    BOOST_TEST( fn.contains( Seventeen() ) );
    BOOST_TEST( !fn.contains( ReturnInt(0) ) );
    BOOST_TEST( !fn.contains( ReturnInt(12) ) );

    fn = ReturnInt(12);

    BOOST_TEST( fn() == 12 );

    BOOST_TEST( !fn.contains( &forty_two ) );
    BOOST_TEST( !fn.contains( Seventeen() ) );
    BOOST_TEST( !fn.contains( ReturnInt(0) ) );
    BOOST_TEST( fn.contains( ReturnInt(12) ) );

    return boost::report_errors();
}
