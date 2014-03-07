/*
 libs/numeric/odeint/examples/stochastic_euler.hpp

 Copyright 2009-2012 Karsten Ahnert
 Copyright 2009-2012 Mario Mulansky

 Stochastic euler stepper example and Ornstein-Uhlenbeck process

 Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#include <vector>
#include <iostream>
#include <boost/random.hpp>
#include <boost/array.hpp>

#include <boost/numeric/odeint.hpp>


/*
//[ stochastic_euler_class_definition
template< size_t N > class stochastic_euler
{
public:

    typedef boost::array< double , N > state_type;
    typedef boost::array< double , N > deriv_type;
    typedef double value_type;
    typedef double time_type;
    typedef unsigned short order_type;
    typedef boost::numeric::odeint::stepper_tag stepper_category;

    static order_type order( void ) { return 1; }

    // ...
};
//]
*/


/*
//[ stochastic_euler_do_step
template< size_t N > class stochastic_euler
{
public:

    // ...

    template< class System >
    void do_step( System system , state_type &x , time_type t , time_type dt ) const
    {
        deriv_type det , stoch ;
        system.first( x , det );
        system.second( x , stoch );
        for( size_t i=0 ; i<x.size() ; ++i )
            x[i] += dt * det[i] + sqrt( dt ) * stoch[i];
    }
};
//]
*/




//[ stochastic_euler_class
template< size_t N >
class stochastic_euler
{
public:

    typedef boost::array< double , N > state_type;
    typedef boost::array< double , N > deriv_type;
    typedef double value_type;
    typedef double time_type;
    typedef unsigned short order_type;

    typedef boost::numeric::odeint::stepper_tag stepper_category;

    static order_type order( void ) { return 1; }

    template< class System >
    void do_step( System system , state_type &x , time_type t , time_type dt ) const
    {
        deriv_type det , stoch ;
        system.first( x , det );
        system.second( x , stoch );
        for( size_t i=0 ; i<x.size() ; ++i )
            x[i] += dt * det[i] + sqrt( dt ) * stoch[i];
    }
};
//]



//[ stochastic_euler_ornstein_uhlenbeck_def
const static size_t N = 1;
typedef boost::array< double , N > state_type;

struct ornstein_det
{
    void operator()( const state_type &x , state_type &dxdt ) const
    {
        dxdt[0] = -x[0];
    }
};

struct ornstein_stoch
{
    boost::mt19937 m_rng;
    boost::normal_distribution<> m_dist;

    ornstein_stoch( double sigma ) : m_rng() , m_dist( 0.0 , sigma ) { }

    void operator()( const state_type &x , state_type &dxdt )
    {
        dxdt[0] = m_dist( m_rng );
    }
};
//]

struct streaming_observer
{
    template< class State >
    void operator()( const State &x , double t ) const
    {
        std::cout << t << "\t" << x[0] << "\n";
    }
};


int main( int argc , char **argv )
{
    using namespace std;
    using namespace boost::numeric::odeint;

    //[ ornstein_uhlenbeck_main
    double dt = 0.1;
    state_type x = {{ 1.0 }};
    integrate_const( stochastic_euler< N >() , make_pair( ornstein_det() , ornstein_stoch( 1.0 ) ) ,
            x , 0.0 , 10.0 , dt , streaming_observer() );
    //]
    return 0;
}
