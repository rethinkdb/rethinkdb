/*
 [auto_generated]
 libs/numeric/odeint/examples/bind_member_functions.hpp

 [begin_description]
 tba.
 [end_description]

 Copyright 2009-2012 Karsten Ahnert
 Copyright 2009-2012 Mario Mulansky

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <iostream>

#include <boost/numeric/odeint.hpp>

namespace odeint = boost::numeric::odeint;

typedef boost::array< double , 3 > state_type;

//[ ode_wrapper
template< class Obj , class Mem >
class ode_wrapper
{
    Obj m_obj;
    Mem m_mem;

public:

    ode_wrapper( Obj obj , Mem mem ) : m_obj( obj ) , m_mem( mem ) { }

    template< class State , class Deriv , class Time >
    void operator()( const State &x , Deriv &dxdt , Time t )
    {
        (m_obj.*m_mem)( x , dxdt , t );
    }
};

template< class Obj , class Mem >
ode_wrapper< Obj , Mem > make_ode_wrapper( Obj obj , Mem mem )
{
    return ode_wrapper< Obj , Mem >( obj , mem );
}
//]


template< class Obj , class Mem >
class observer_wrapper
{
    Obj m_obj;
    Mem m_mem;

public:

    observer_wrapper( Obj obj , Mem mem ) : m_obj( obj ) , m_mem( mem ) { }

    template< class State , class Time >
    void operator()( const State &x , Time t )
    {
        (m_obj.*m_mem)( x , t );
    }
};

template< class Obj , class Mem >
observer_wrapper< Obj , Mem > make_observer_wrapper( Obj obj , Mem mem )
{
    return observer_wrapper< Obj , Mem >( obj , mem );
}



//[ bind_member_function
struct lorenz
{
    void ode( const state_type &x , state_type &dxdt , double t ) const
    {
        dxdt[0] = 10.0 * ( x[1] - x[0] );
        dxdt[1] = 28.0 * x[0] - x[1] - x[0] * x[2];
        dxdt[2] = -8.0 / 3.0 * x[2] + x[0] * x[1];
    }
};

int main( int argc , char *argv[] )
{
    using namespace boost::numeric::odeint;
    state_type x = {{ 10.0 , 10.0 , 10.0 }};
    integrate_const( runge_kutta4< state_type >() , make_ode_wrapper( lorenz() , &lorenz::ode ) ,
                     x , 0.0 , 10.0 , 0.01 );
    return 0;
}
//]


/*
struct lorenz
{
    void ode( const state_type &x , state_type &dxdt , double t ) const
    {
        dxdt[0] = 10.0 * ( x[1] - x[0] );
        dxdt[1] = 28.0 * x[0] - x[1] - x[0] * x[2];
        dxdt[2] = -8.0 / 3.0 * x[2] + x[0] * x[1];
    }

    void obs( const state_type &x , double t ) const
    {
        std::cout << t << " " << x[0] << " " << x[1] << " " << x[2] << "\n";
    }
};

int main( int argc , char *argv[] )
{
    using namespace boost::numeric::odeint;

    state_type x = {{ 10.0 , 10.0 , 10.0 }};
    integrate_const( runge_kutta4< state_type >() ,
                     make_ode_wrapper( lorenz() , &lorenz::ode ) ,
                     x , 0.0 , 10.0 , 0.01 ,
                     make_observer_wrapper( lorenz() , &lorenz::obs ) );

    return 0;
}
*/
