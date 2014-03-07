/*
 libs/numeric/odeint/examples/stochastic_euler.hpp

 Copyright 2009 Karsten Ahnert
 Copyright 2009 Mario Mulansky

 Stochastic euler stepper example and Ornstein-Uhlenbeck process

 Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <boost/array.hpp>

#include <boost/numeric/odeint.hpp>

typedef boost::array< double , 1 > state_type;

using namespace boost::numeric::odeint;


//[ generation_functions_own_steppers
class custom_stepper
{
public:

    typedef double value_type;
    // ...
};

class custom_controller
{
    // ...
};

class custom_dense_output
{
    // ...
};
//]


//[ generation_functions_get_controller
namespace boost { namespace numeric { namespace odeint {

template<>
struct get_controller< custom_stepper >
{
    typedef custom_controller type;
};

} } }
//]

//[ generation_functions_controller_factory
namespace boost { namespace numeric { namespace odeint {

template<>
struct controller_factory< custom_stepper , custom_controller >
{
    custom_controller operator()( double abs_tol , double rel_tol , const custom_stepper & ) const
    {
        return custom_controller();
    }
};

} } }
//]

int main( int argc , char **argv )
{
    {
        typedef runge_kutta_dopri5< state_type > stepper_type;

        /*
        //[ generation_functions_syntax_auto
        auto stepper1 = make_controlled( 1.0e-6 , 1.0e-6 , stepper_type() );
        auto stepper2 = make_dense_output( 1.0e-6 , 1.0e-6 , stepper_type() );
        //]
        */

        //[ generation_functions_syntax_result_of
        boost::numeric::odeint::result_of::make_controlled< stepper_type >::type stepper3 = make_controlled( 1.0e-6 , 1.0e-6 , stepper_type() );
        boost::numeric::odeint::result_of::make_dense_output< stepper_type >::type stepper4 = make_dense_output( 1.0e-6 , 1.0e-6 , stepper_type() );
        //]
    }

    {
        /*
        //[ generation_functions_example_custom_controller
        auto stepper5 = make_controlled( 1.0e-6 , 1.0e-6 , custom_stepper() );
        //]
        */

        boost::numeric::odeint::result_of::make_controlled< custom_stepper >::type stepper5 = make_controlled( 1.0e-6 , 1.0e-6 , custom_stepper() );
    }
    return 0;
}
