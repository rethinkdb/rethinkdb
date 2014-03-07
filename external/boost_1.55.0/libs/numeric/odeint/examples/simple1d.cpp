/* Boost libs/numeric/odeint/examples/simple1d.cpp

 Copyright 2009-2012 Karsten Ahnert
 Copyright 2009-2012 Mario Mulansky

 example for a simple one-dimensional 1st order ODE

 Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#include <iostream>
#include <boost/numeric/odeint.hpp>

using namespace std;
using namespace boost::numeric::odeint;


/* we solve the simple ODE x' = 3/(2t^2) + x/(2t)
 * with initial condition x(1) = 0.
 * Analytic solution is x(t) = sqrt(t) - 1/t
 */

void rhs( const double x , double &dxdt , const double t )
{
    dxdt = 3.0/(2.0*t*t) + x/(2.0*t);
}

void write_cout( const double &x , const double t )
{
    cout << t << '\t' << x << endl;
}

// state_type = value_type = deriv_type = time_type = double
typedef runge_kutta_dopri5< double , double , double , double , vector_space_algebra , default_operations , never_resizer > stepper_type;

int main()
{
    double x = 0.0; //initial value x(1) = 0
    // use dopri5 with stepsize control and allowed errors 10^-12, integrate t=1...10
    integrate_adaptive( make_controlled( 1E-12 , 1E-12 , stepper_type() ) , rhs , x , 1.0 , 10.0 , 0.1 , write_cout );
}
