/*
 [auto_generated]
 boost/numeric/odeint/integrate/detail/integrate_adaptive.hpp

 [begin_description]
 Default Integrate adaptive implementation.
 [end_description]

 Copyright 2009-2011 Karsten Ahnert
 Copyright 2009-2011 Mario Mulansky

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#ifndef BOOST_NUMERIC_ODEINT_INTEGRATE_DETAIL_INTEGRATE_ADAPTIVE_HPP_INCLUDED
#define BOOST_NUMERIC_ODEINT_INTEGRATE_DETAIL_INTEGRATE_ADAPTIVE_HPP_INCLUDED

#include <stdexcept>

#include <boost/numeric/odeint/stepper/stepper_categories.hpp>
#include <boost/numeric/odeint/stepper/controlled_step_result.hpp>
#include <boost/numeric/odeint/integrate/detail/integrate_n_steps.hpp>
#include <boost/numeric/odeint/util/bind.hpp>
#include <boost/numeric/odeint/util/unwrap_reference.hpp>
#include <boost/numeric/odeint/util/copy.hpp>

#include <boost/numeric/odeint/util/detail/less_with_sign.hpp>


#include <iostream>

namespace boost {
namespace numeric {
namespace odeint {
namespace detail {

// forward declaration
template< class Stepper , class System , class State , class Time , class Observer>
Time integrate_n_steps(
        Stepper stepper , System system , State &start_state ,
        Time start_time , Time dt , size_t num_of_steps ,
        Observer observer , stepper_tag );

/*
 * integrate_adaptive for simple stepper is basically an integrate_const + some last step
 */
template< class Stepper , class System , class State , class Time , class Observer >
size_t integrate_adaptive(
        Stepper stepper , System system , State &start_state ,
        Time start_time , Time end_time , Time dt ,
        Observer observer , stepper_tag
)
{
    size_t steps = static_cast< size_t >( (end_time-start_time)/dt );
    Time end = detail::integrate_n_steps( stepper , system , start_state , start_time ,
                                          dt , steps , observer , stepper_tag() );
    if( less_with_sign( end , end_time , dt ) )
    {   //make a last step to end exactly at end_time
        stepper.do_step( system , start_state , end , end_time - end );
        steps++;
        typename odeint::unwrap_reference< Observer >::type &obs = observer;
        obs( start_state , end_time );
    }
    return steps;
}


/*
 * classical integrate adaptive
 */
template< class Stepper , class System , class State , class Time , class Observer >
size_t integrate_adaptive(
        Stepper stepper , System system , State &start_state ,
        Time &start_time , Time end_time , Time &dt ,
        Observer observer , controlled_stepper_tag
)
{
    typename odeint::unwrap_reference< Observer >::type &obs = observer;

    const size_t max_attempts = 1000;
    const char *error_string = "Integrate adaptive : Maximal number of iterations reached. A step size could not be found.";
    size_t count = 0;
    while( less_with_sign( start_time , end_time , dt ) )
    {
        obs( start_state , start_time );
        if( less_with_sign( end_time , start_time + dt , dt ) )
        {
            dt = end_time - start_time;
        }

        size_t trials = 0;
        controlled_step_result res = success;
        do
        {
            res = stepper.try_step( system , start_state , start_time , dt );
            ++trials;
        }
        while( ( res == fail ) && ( trials < max_attempts ) );
        if( trials == max_attempts ) throw std::overflow_error( error_string );

        ++count;
    }
    obs( start_state , start_time );
    return count;
}


/*
 * integrate adaptive for dense output steppers
 *
 * step size control is used if the stepper supports it
 */
template< class Stepper , class System , class State , class Time , class Observer >
size_t integrate_adaptive(
        Stepper stepper , System system , State &start_state ,
        Time start_time , Time end_time , Time dt ,
        Observer observer , dense_output_stepper_tag )
{
    typename odeint::unwrap_reference< Observer >::type &obs = observer;

    size_t count = 0;
    stepper.initialize( start_state , start_time , dt );

    while( less_with_sign( stepper.current_time() , end_time , stepper.current_time_step() ) )
    {
        while( less_eq_with_sign( stepper.current_time() + stepper.current_time_step() ,
               end_time ,
               stepper.current_time_step() ) )
        {   //make sure we don't go beyond the end_time
            obs( stepper.current_state() , stepper.current_time() );
            stepper.do_step( system );
            ++count;
        }
        stepper.initialize( stepper.current_state() , stepper.current_time() , end_time - stepper.current_time() );
    }
    obs( stepper.current_state() , stepper.current_time() );
    // overwrite start_state with the final point
    boost::numeric::odeint::copy( stepper.current_state() , start_state );
    return count;
}




} // namespace detail
} // namespace odeint
} // namespace numeric
} // namespace boost


#endif // BOOST_NUMERIC_ODEINT_INTEGRATE_DETAIL_INTEGRATE_ADAPTIVE_HPP_INCLUDED
