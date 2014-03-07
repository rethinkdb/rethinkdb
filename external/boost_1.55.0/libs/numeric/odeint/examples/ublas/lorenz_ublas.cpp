/*
 * Copyright 2009-2012 Karsten Ahnert
 * Copyright 2009-2012 Mario Mulansky
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or
 * copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#include <iostream>

#include <boost/numeric/odeint.hpp>
#include <boost/numeric/ublas/vector.hpp>

/* define ublas::vector<double> as resizeable 
 * this is not neccessarily required because this definition already 
 * exists in util/ublas_wrapper.hpp.
 * However, for completeness and educational purpose it is repeated here.
 */

//[ ublas_resizeable
typedef boost::numeric::ublas::vector< double > state_type;

namespace boost { namespace numeric { namespace odeint {

template<>
struct is_resizeable< state_type >
{
    typedef boost::true_type type;
    const static bool value = type::value;
};

} } }
//]

void lorenz( const state_type &x , state_type &dxdt , const double t )
{
    const double sigma( 10.0 );
    const double R( 28.0 );
    const double b( 8.0 / 3.0 );
    
    dxdt[0] = sigma * ( x[1] - x[0] );
    dxdt[1] = R * x[0] - x[1] - x[0] * x[2];
    dxdt[2] = -b * x[2] + x[0] * x[1];
}

using namespace boost::numeric::odeint;

//[ublas_main
int main()
{
    state_type x(3);
    x[0] = 10.0; x[1] = 5.0 ; x[2] = 0.0;
    typedef runge_kutta4< state_type , double , state_type , double , vector_space_algebra > stepper;
    integrate_const( stepper() , lorenz , x ,
                     0.0 , 10.0 , 0.1 );
}
//]
