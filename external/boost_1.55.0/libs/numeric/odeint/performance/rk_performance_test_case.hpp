/*
 * rk_performance_test_case.hpp
 *
 * Copyright 2009-2012 Karsten Ahnert
 * Copyright 2009-2012 Mario Mulansky
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or
 * copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#include <iostream>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>
#include <boost/timer.hpp>

#define tab "\t"

using namespace std;
using namespace boost::accumulators;

typedef accumulator_set<
    double , stats< tag::mean , tag::variance >
    > accumulator_type;

ostream& operator<<( ostream& out , accumulator_type &acc )
{
    out << boost::accumulators::mean( acc ) << tab;
//    out << boost::accumulators::variance( acc ) << tab;
    return out;
}

typedef boost::timer timer_type;


template< class Stepper >
void run( Stepper &stepper , const size_t num_of_steps = 20000000 , const double dt = 1E-10 )
{
    const size_t loops = 20;

    accumulator_type acc;
    timer_type timer;

    srand( 12312354 );

    // transient
    //stepper.reset_init_cond( );
    //for( size_t i = 0 ; i < num_of_steps ; ++i )
    //    stepper.do_step( dt );

    for( size_t n=0 ; n<loops+1 ; ++n )
    {
        stepper.reset_init_cond( );

        timer.restart();
        for( size_t i = 0 ; i < num_of_steps ; ++i )
            stepper.do_step( dt );
        if( n>0 )
        {   // take first run as transient
            acc(timer.elapsed());
            clog.precision(8);
            clog.width(10);
            clog << acc << " " << stepper.state(0) << endl;
        }
    }
    cout << acc << endl;
}
