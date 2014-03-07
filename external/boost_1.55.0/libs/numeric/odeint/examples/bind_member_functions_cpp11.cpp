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
#include <array>
#include <type_traits>

#include <boost/numeric/odeint.hpp>

namespace odeint = boost::numeric::odeint;



typedef std::array< double , 3 > state_type;

struct lorenz
{
    void ode( const state_type &x , state_type &dxdt , double t ) const
    {
        const double sigma = 10.0;
        const double R = 28.0;
        const double b = 8.0 / 3.0;

        dxdt[0] = sigma * ( x[1] - x[0] );
        dxdt[1] = R * x[0] - x[1] - x[0] * x[2];
        dxdt[2] = -b * x[2] + x[0] * x[1];
    }
};

int main( int argc , char *argv[] )
{
    using namespace boost::numeric::odeint;
    //[ bind_member_function_cpp11
    namespace pl = std::placeholders;

    state_type x = {{ 10.0 , 10.0 , 10.0 }};
    integrate_const( runge_kutta4< state_type >() ,
                     std::bind( &lorenz::ode , lorenz() , pl::_1 , pl::_2 , pl::_3 ) ,
                     x , 0.0 , 10.0 , 0.01  );
    //]
    return 0;
}

