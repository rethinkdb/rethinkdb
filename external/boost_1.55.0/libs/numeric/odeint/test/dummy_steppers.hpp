/*
 [auto_generated]
 libs/numeric/odeint/test/dummy_steppers.hpp

 [begin_description]
 Dummy steppers for several tests.
 [end_description]

 Copyright 2009-2011 Karsten Ahnert
 Copyright 2009-2011 Mario Mulansky

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#ifndef BOOST_LIBS_NUMERIC_ODEINT_TEST_DUMMY_STEPPER_HPP_INCLUDED
#define BOOST_LIBS_NUMERIC_ODEINT_TEST_DUMMY_STEPPER_HPP_INCLUDED

#include <boost/array.hpp>
#include <boost/numeric/odeint/stepper/stepper_categories.hpp>
#include <boost/numeric/odeint/stepper/controlled_step_result.hpp>

namespace boost {
namespace numeric {
namespace odeint {

struct dummy_stepper
{
    typedef double value_type;
    typedef value_type time_type;
    typedef boost::array< value_type , 1 > state_type;
    typedef state_type deriv_type;
    typedef unsigned short order_type;
    typedef stepper_tag stepper_category;

    order_type order( void ) const { return 1; }

    template< class System >
    void do_step( System sys , state_type &x , time_type t , time_type dt ) const
    {
        x[0] += 0.25;
    }
};

struct dummy_dense_output_stepper
{
    typedef double value_type;
    typedef value_type time_type;
    typedef boost::array< value_type , 1 > state_type;
    typedef state_type deriv_type;
    typedef dense_output_stepper_tag stepper_category;

    void initialize( const state_type &x0 , time_type t0 , time_type dt0 )
    {
        m_x = x0;
        m_t = t0;
        m_dt = dt0;
    }

    template< class System >
    std::pair< time_type , time_type > do_step( System sys )
    {
        m_x[0] += 0.25;
        m_t += m_dt;
        return std::make_pair( m_t - m_dt , m_t );
    }

    void calc_state( time_type t_inter , state_type &x ) const
    {
        value_type theta = ( m_t - t_inter ) / m_dt;
        x[0] = m_x[0] - 0.25 * theta;
        
    }

    const time_type& current_time( void ) const
    {
        return m_t;
    }

    const state_type& current_state( void ) const
    {
        return m_x;
    }

    state_type m_x;
    time_type m_t;
    time_type m_dt;
};



struct dummy_controlled_stepper
{
    typedef double value_type;
    typedef value_type time_type;
    typedef boost::array< value_type , 1 > state_type;
    typedef state_type deriv_type;
    typedef controlled_stepper_tag stepper_category;

    template< class Sys >
    controlled_step_result try_step( Sys sys , state_type &x , time_type &t , time_type &dt ) const
    {
        x[0] += 0.25;
        t += dt;
        return success;
    }
};


} // odeint
} // numeric
} // boost


#endif // BOOST_LIBS_NUMERIC_ODEINT_TEST_DUMMY_STEPPER_HPP_INCLUDED
