//  (C) Copyright Gennadiy Rozental 2005-2008.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.

// Boost.Test
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>
#include <boost/test/logged_expectations.hpp>
#include <boost/test/mock_object.hpp>
using namespace boost::itest;

// Boost
#include <boost/bind.hpp>

//____________________________________________________________________________//

// Collaborators interfaces
class Stove {
public:
    virtual void    light() = 0;
    virtual void    set_temperature( int temp ) = 0;
    virtual void    set_cook_time( int time_in_sec ) = 0;
    virtual void    auto_clean() = 0;
};

class Microwave {
public:
    virtual int     get_max_power() = 0;
    virtual void    set_power_level( int l ) = 0;
    virtual void    set_time( int time_in_sec ) = 0;
    virtual void    start() = 0;
};

class Timer {
public:
    virtual void    wait( int wait_time ) = 0;
};

//____________________________________________________________________________//

// Collaborators mocks
class MockStove : public ::boost::itest::mock_object<0,Stove> {
public:
    virtual void    light()
    {
        BOOST_ITEST_SCOPE( Stove::light );
    }
    virtual void    set_temperature( int temp )
    {
        BOOST_ITEST_SCOPE( Stove::set_temperature );

        BOOST_ITEST_DATA_FLOW( temp );
    }
    virtual void    set_cook_time( int time_in_sec )
    {
        BOOST_ITEST_SCOPE( Stove::set_cook_time );

        BOOST_ITEST_DATA_FLOW( time_in_sec );
    }
    virtual void    auto_clean()
    {
        BOOST_ITEST_SCOPE( Stove::auto_clean );
    }
};

class MockMicrowave : public ::boost::itest::mock_object<0,Microwave> {
public:
    virtual int     get_max_power()
    {
        BOOST_ITEST_SCOPE( Microwave::get_max_power );

        return BOOST_ITEST_RETURN( int, 1000 );
    }
    virtual void    set_power_level( int l )
    {
        BOOST_ITEST_SCOPE( Microwave::set_power_level );

        BOOST_ITEST_DATA_FLOW( l );
    }
    virtual void    set_time( int time_in_sec )
    {
        BOOST_ITEST_SCOPE( Microwave::set_time );

        BOOST_ITEST_DATA_FLOW( time_in_sec );
    }
    virtual void    start()
    {
        BOOST_ITEST_SCOPE( Microwave::start );
    }
};

class MockTimer : public ::boost::itest::mock_object<0,Timer> {
public:
    virtual void    wait( int wait_time )
    {
        BOOST_ITEST_SCOPE( Timer::wait );

        BOOST_ITEST_DATA_FLOW( wait_time );
    }
};

//____________________________________________________________________________//

// Class under test
class kitchen_robot {
public:
    void make_grilled_chicken( Timer& t, Stove& s, Microwave& m )
    {
        m.set_power_level( 600 / (m.get_max_power()/5) + 1 );
        m.set_time( 15 * 60 ); // 15 min
        m.start(); // defrost

        t.wait( 15 * 60 );

        s.set_cook_time( 2 * 60 * 60 ); // 2 hours
        s.light();

        s.set_temperature( 450 );
        t.wait( 15 * 60 ); // 15 min - preheat

        s.set_temperature( 400 );
        t.wait( 90 * 60 ); // 1 hour 30 min - cook

        s.set_temperature( 250 );
        t.wait( 90 * 60 ); // 15 min - almost done

        s.auto_clean(); // done
    }
};

//____________________________________________________________________________//

BOOST_TEST_LOGGED_EXPECTATIONS( test_grilled_chiken_recept )
{
    kitchen_robot kr;

    MockTimer t;
    MockStove s;
    MockMicrowave m;


    kr.make_grilled_chicken( t, s, m );
}

//____________________________________________________________________________//

// EOF
