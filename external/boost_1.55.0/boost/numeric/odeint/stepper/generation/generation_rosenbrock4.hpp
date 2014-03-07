/*
 [auto_generated]
 boost/numeric/odeint/stepper/generation/generation_rosenbrock4.hpp

 [begin_description]
 Enable the factory functions for the controller and the dense output of the Rosenbrock4 method.
 [end_description]

 Copyright 2009-2011 Karsten Ahnert
 Copyright 2009-2011 Mario Mulansky

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#ifndef BOOST_NUMERIC_ODEINT_STEPPER_GENERATION_GENERATION_ROSENBROCK4_HPP_INCLUDED
#define BOOST_NUMERIC_ODEINT_STEPPER_GENERATION_GENERATION_ROSENBROCK4_HPP_INCLUDED

#include <boost/numeric/odeint/stepper/rosenbrock4.hpp>
#include <boost/numeric/odeint/stepper/rosenbrock4_controller.hpp>
#include <boost/numeric/odeint/stepper/rosenbrock4_dense_output.hpp>


namespace boost {
namespace numeric {
namespace odeint {


template< class Value , class Coefficients , class Resize >
struct get_controller< rosenbrock4< Value , Coefficients , Resize > >
{
    typedef rosenbrock4< Value , Coefficients , Resize > stepper_type;
    typedef rosenbrock4_controller< stepper_type > type;
};



template< class Value , class Coefficients , class Resize >
struct get_dense_output< rosenbrock4< Value , Coefficients , Resize > >
{
    typedef rosenbrock4< Value , Coefficients , Resize > stepper_type;
    typedef rosenbrock4_controller< stepper_type > controller_type;
    typedef rosenbrock4_dense_output< controller_type > type;
};



// controller factory for controlled_runge_kutta
template< class Stepper >
struct dense_output_factory< Stepper , rosenbrock4_dense_output< rosenbrock4_controller< Stepper > > >
{
    typedef Stepper stepper_type;
    typedef rosenbrock4_controller< stepper_type > controller_type;
    typedef typename stepper_type::value_type value_type;
    typedef rosenbrock4_dense_output< controller_type > dense_output_type;

    dense_output_type operator()( value_type abs_error , value_type rel_error , const stepper_type &stepper )
    {
        return dense_output_type( controller_type( abs_error , rel_error , stepper ) );
    }
};



} // odeint
} // numeric
} // boost


#endif // BOOST_NUMERIC_ODEINT_STEPPER_GENERATION_GENERATION_ROSENBROCK4_HPP_INCLUDED
