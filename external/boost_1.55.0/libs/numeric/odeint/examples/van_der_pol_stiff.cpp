/*
 * van_der_pol_stiff.cpp
 *  
 * Created on: Dec 12, 2011
 *
 * Copyright 2011 Rajeev Singh
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
#include <boost/phoenix/operator.hpp>

using namespace std;
using namespace boost::numeric::odeint;
namespace phoenix = boost::phoenix;

const double mu = 1000.0;


typedef boost::numeric::ublas::vector< double > vector_type;
typedef boost::numeric::ublas::matrix< double > matrix_type;

struct vdp_stiff
{
    void operator()( const vector_type &x , vector_type &dxdt , double t )
    {
        dxdt[0] = x[1];
        dxdt[1] = -x[0] - mu * x[1] * (x[0]*x[0]-1.0);
    }
};

struct vdp_stiff_jacobi
{
    void operator()( const vector_type &x , matrix_type &J , const double &t , vector_type &dfdt )
    {
        J(0, 0) = 0.0;
        J(0, 1) = 1.0;
        J(1, 0) = -1.0 - 2.0*mu * x[0] * x[1];
        J(1, 1) = -mu * ( x[0] * x[0] - 1.0);

        dfdt[0] = 0.0;
        dfdt[1] = 0.0;
    }
};


int main( int argc , char **argv )
{
    //[ integrate_stiff_system
    vector_type x( 2 );
    /* initialize random seed: */
    srand ( time(NULL) );

    // initial conditions
    for (int i=0; i<2; i++)
        x[i] = 1.0; //(1.0 * rand()) / RAND_MAX;

    size_t num_of_steps = integrate_const( make_dense_output< rosenbrock4< double > >( 1.0e-6 , 1.0e-6 ) ,
            make_pair( vdp_stiff() , vdp_stiff_jacobi() ) ,
            x , 0.0 , 1000.0 , 1.0
            , cout << phoenix::arg_names::arg2 << " " << phoenix::arg_names::arg1[0] << " " << phoenix::arg_names::arg1[1] << "\n"
            );
    //]
    clog << num_of_steps << endl;



    //[ integrate_stiff_system_alternative

    vector_type x2( 2 );
    // initial conditions
    for (int i=0; i<2; i++)
        x2[i] = 1.0; //(1.0 * rand()) / RAND_MAX;

    size_t num_of_steps2 = integrate_const( make_dense_output< runge_kutta_dopri5< vector_type > >( 1.0e-6 , 1.0e-6 ) ,
            vdp_stiff() , x2 , 0.0 , 1000.0 , 1.0
            , cout << phoenix::arg_names::arg2 << " " << phoenix::arg_names::arg1[0] << " " << phoenix::arg_names::arg1[1] << "\n"
            );
    //]
    clog << num_of_steps2 << endl;


    return 0;
}
