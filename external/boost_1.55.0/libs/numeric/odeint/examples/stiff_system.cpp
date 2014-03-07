/*
 * rosenbrock4.cpp
 *
 * Copyright 2009-2012 Karsten Ahnert
 * Copyright 2009-2012 Mario Mulansky
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or
 * copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <iostream>
#include <fstream>
#include <utility>

#include <boost/numeric/odeint.hpp>

#include <boost/phoenix/core.hpp>

#include <boost/phoenix/core.hpp>
#include <boost/phoenix/operator.hpp>

using namespace std;
using namespace boost::numeric::odeint;
namespace phoenix = boost::phoenix;



//[ stiff_system_definition
typedef boost::numeric::ublas::vector< double > vector_type;
typedef boost::numeric::ublas::matrix< double > matrix_type;

struct stiff_system
{
    void operator()( const vector_type &x , vector_type &dxdt , double /* t */ )
    {
        dxdt[ 0 ] = -101.0 * x[ 0 ] - 100.0 * x[ 1 ];
        dxdt[ 1 ] = x[ 0 ];
    }
};

struct stiff_system_jacobi
{
    void operator()( const vector_type & /* x */ , matrix_type &J , const double & /* t */ , vector_type &dfdt )
    {
        J( 0 , 0 ) = -101.0;
        J( 0 , 1 ) = -100.0;
        J( 1 , 0 ) = 1.0;
        J( 1 , 1 ) = 0.0;
        dfdt[0] = 0.0;
        dfdt[1] = 0.0;
    }
};
//]



/*
//[ stiff_system_alternative_definition
typedef boost::numeric::ublas::vector< double > vector_type;
typedef boost::numeric::ublas::matrix< double > matrix_type;

struct stiff_system
{
    template< class State >
    void operator()( const State &x , State &dxdt , double t )
    {
        ...
    }
};

struct stiff_system_jacobi
{
    template< class State , class Matrix >
    void operator()( const State &x , Matrix &J , const double &t , State &dfdt )
    {
        ...
    }
};
//]
 */



int main( int argc , char **argv )
{
//    typedef rosenbrock4< double > stepper_type;
//    typedef rosenbrock4_controller< stepper_type > controlled_stepper_type;
//    typedef rosenbrock4_dense_output< controlled_stepper_type > dense_output_type;
    //[ integrate_stiff_system
    vector_type x( 2 , 1.0 );

    size_t num_of_steps = integrate_const( make_dense_output< rosenbrock4< double > >( 1.0e-6 , 1.0e-6 ) ,
            make_pair( stiff_system() , stiff_system_jacobi() ) ,
            x , 0.0 , 50.0 , 0.01 ,
            cout << phoenix::arg_names::arg2 << " " << phoenix::arg_names::arg1[0] << "\n" );
    //]
    clog << num_of_steps << endl;



//    typedef runge_kutta_dopri5< vector_type > dopri5_type;
//    typedef controlled_runge_kutta< dopri5_type > controlled_dopri5_type;
//    typedef dense_output_runge_kutta< controlled_dopri5_type > dense_output_dopri5_type;
    //[ integrate_stiff_system_alternative

    vector_type x2( 2 , 1.0 );

    size_t num_of_steps2 = integrate_const( make_dense_output< runge_kutta_dopri5< vector_type > >( 1.0e-6 , 1.0e-6 ) ,
            stiff_system() , x2 , 0.0 , 50.0 , 0.01 ,
            cout << phoenix::arg_names::arg2 << " " << phoenix::arg_names::arg1[0] << "\n" );
    //]
    clog << num_of_steps2 << endl;


    return 0;
}
