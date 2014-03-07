/* [auto_generated]
 boost/numeric/odeint/stepper/controlled_runge_kutta.hpp

 [begin_description]
 The default controlled stepper which can be used with all explicit Runge-Kutta error steppers.
 [end_description]

 Copyright 2009-2011 Karsten Ahnert
 Copyright 2009-2011 Mario Mulansky

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#ifndef BOOST_NUMERIC_ODEINT_STEPPER_CONTROLLED_RUNGE_KUTTA_HPP_INCLUDED
#define BOOST_NUMERIC_ODEINT_STEPPER_CONTROLLED_RUNGE_KUTTA_HPP_INCLUDED



#include <cmath>

#include <boost/config.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_same.hpp>

#include <boost/numeric/odeint/util/bind.hpp>
#include <boost/numeric/odeint/util/unwrap_reference.hpp>
#include <boost/numeric/odeint/util/copy.hpp>

#include <boost/numeric/odeint/util/state_wrapper.hpp>
#include <boost/numeric/odeint/util/is_resizeable.hpp>
#include <boost/numeric/odeint/util/resizer.hpp>

#include <boost/numeric/odeint/algebra/range_algebra.hpp>
#include <boost/numeric/odeint/algebra/default_operations.hpp>

#include <boost/numeric/odeint/stepper/controlled_step_result.hpp>
#include <boost/numeric/odeint/stepper/stepper_categories.hpp>

namespace boost {
namespace numeric {
namespace odeint {


template
<
class Value ,
class Algebra = range_algebra ,
class Operations = default_operations
>
class default_error_checker
{
public:

    typedef Value value_type;
    typedef Algebra algebra_type;
    typedef Operations operations_type;

    default_error_checker(
            value_type eps_abs = static_cast< value_type >( 1.0e-6 ) ,
            value_type eps_rel = static_cast< value_type >( 1.0e-6 ) ,
            value_type a_x = static_cast< value_type >( 1 ) ,
            value_type a_dxdt = static_cast< value_type >( 1 ) )
    : m_eps_abs( eps_abs ) , m_eps_rel( eps_rel ) , m_a_x( a_x ) , m_a_dxdt( a_dxdt )
    { }


    template< class State , class Deriv , class Err , class Time >
    value_type error( const State &x_old , const Deriv &dxdt_old , Err &x_err , Time dt ) const
    {
        return error( algebra_type() , x_old , dxdt_old , x_err , dt );
    }

    template< class State , class Deriv , class Err , class Time >
    value_type error( algebra_type &algebra , const State &x_old , const Deriv &dxdt_old , Err &x_err , Time dt ) const
    {
        // this overwrites x_err !
        algebra.for_each3( x_err , x_old , dxdt_old ,
                typename operations_type::template rel_error< value_type >( m_eps_abs , m_eps_rel , m_a_x , m_a_dxdt * get_unit_value( dt ) ) );

        value_type res = algebra.reduce( x_err ,
                typename operations_type::template maximum< value_type >() , static_cast< value_type >( 0 ) );
        return res;
    }

private:

    value_type m_eps_abs;
    value_type m_eps_rel;
    value_type m_a_x;
    value_type m_a_dxdt;

};








/*
 * error stepper category dispatcher
 */
template<
class ErrorStepper ,
class ErrorChecker = default_error_checker< typename ErrorStepper::value_type ,
typename ErrorStepper::algebra_type ,
typename ErrorStepper::operations_type > ,
class Resizer = typename ErrorStepper::resizer_type ,
class ErrorStepperCategory = typename ErrorStepper::stepper_category
>
class controlled_runge_kutta ;



/*
 * explicit stepper version
 *
 * this class introduces the following try_step overloads
    * try_step( sys , x , t , dt )
    * try_step( sys , x , dxdt , t , dt )
    * try_step( sys , in , t , out , dt )
    * try_step( sys , in , dxdt , t , out , dt )
 */
/**
 * \brief Implements step size control for Runge-Kutta steppers with error 
 * estimation.
 *
 * This class implements the step size control for standard Runge-Kutta 
 * steppers with error estimation.
 *
 * \tparam ErrorStepper The stepper type with error estimation, has to fulfill the ErrorStepper concept.
 * \tparam ErrorChecker The error checker
 * \tparam Resizer The resizer policy type.
 */
template<
class ErrorStepper ,
class ErrorChecker ,
class Resizer
>
class controlled_runge_kutta< ErrorStepper , ErrorChecker , Resizer , explicit_error_stepper_tag >
{

public:

    typedef ErrorStepper stepper_type;
    typedef typename stepper_type::state_type state_type;
    typedef typename stepper_type::value_type value_type;
    typedef typename stepper_type::deriv_type deriv_type;
    typedef typename stepper_type::time_type time_type;
    typedef typename stepper_type::algebra_type algebra_type;
    typedef typename stepper_type::operations_type operations_type;
    typedef Resizer resizer_type;
    typedef ErrorChecker error_checker_type;
    typedef explicit_controlled_stepper_tag stepper_category;

#ifndef DOXYGEN_SKIP
    typedef typename stepper_type::wrapped_state_type wrapped_state_type;
    typedef typename stepper_type::wrapped_deriv_type wrapped_deriv_type;

    typedef controlled_runge_kutta< ErrorStepper , ErrorChecker , Resizer , explicit_error_stepper_tag > controlled_stepper_type;
#endif //DOXYGEN_SKIP


    /**
     * \brief Constructs the controlled Runge-Kutta stepper.
     * \param error_checker An instance of the error checker.
     * \param stepper An instance of the underlying stepper.
     */
    controlled_runge_kutta(
            const error_checker_type &error_checker = error_checker_type( ) ,
            const stepper_type &stepper = stepper_type( )
    )
    : m_stepper( stepper ) , m_error_checker( error_checker )
    { }



    /*
     * Version 1 : try_step( sys , x , t , dt )
     *
     * The overloads are needed to solve the forwarding problem
     */
    /**
     * \brief Tries to perform one step.
     *
     * This method tries to do one step with step size dt. If the error estimate
     * is to large, the step is rejected and the method returns fail and the 
     * step size dt is reduced. If the error estimate is acceptably small, the
     * step is performed, success is returned and dt might be increased to make 
     * the steps as large as possible. This method also updates t if a step is
     * performed.
     *
     * \param system The system function to solve, hence the r.h.s. of the ODE. It must fulfill the
     *               Simple System concept.
     * \param x The state of the ODE which should be solved. Overwritten if 
     * the step is successful.
     * \param t The value of the time. Updated if the step is successful.
     * \param dt The step size. Updated.
     * \return success if the step was accepted, fail otherwise.
     */
    template< class System , class StateInOut >
    controlled_step_result try_step( System system , StateInOut &x , time_type &t , time_type &dt )
    {
        return try_step_v1( system , x , t, dt );
    }

    /**
     * \brief Tries to perform one step. Solves the forwarding problem and 
     * allows for using boost range as state_type.
     *
     * This method tries to do one step with step size dt. If the error estimate
     * is to large, the step is rejected and the method returns fail and the 
     * step size dt is reduced. If the error estimate is acceptably small, the
     * step is performed, success is returned and dt might be increased to make 
     * the steps as large as possible. This method also updates t if a step is
     * performed.
     *
     * \param system The system function to solve, hence the r.h.s. of the ODE. It must fulfill the
     *               Simple System concept.
     * \param x The state of the ODE which should be solved. Overwritten if 
     * the step is successful. Can be a boost range.
     * \param t The value of the time. Updated if the step is successful.
     * \param dt The step size. Updated.
     * \return success if the step was accepted, fail otherwise.
     */
    template< class System , class StateInOut >
    controlled_step_result try_step( System system , const StateInOut &x , time_type &t , time_type &dt )
    {
        return try_step_v1( system , x , t, dt );
    }



    /*
     * Version 2 : try_step( sys , x , dxdt , t , dt )
     *
     * this version does not solve the forwarding problem, boost.range can not be used
     */
    /**
     * \brief Tries to perform one step.
     *
     * This method tries to do one step with step size dt. If the error estimate
     * is to large, the step is rejected and the method returns fail and the 
     * step size dt is reduced. If the error estimate is acceptably small, the
     * step is performed, success is returned and dt might be increased to make 
     * the steps as large as possible. This method also updates t if a step is
     * performed.
     *
     * \param system The system function to solve, hence the r.h.s. of the ODE. It must fulfill the
     *               Simple System concept.
     * \param x The state of the ODE which should be solved. Overwritten if 
     * the step is successful.
     * \param dxdt The derivative of state.
     * \param t The value of the time. Updated if the step is successful.
     * \param dt The step size. Updated.
     * \return success if the step was accepted, fail otherwise.
     */
    template< class System , class StateInOut , class DerivIn >
    controlled_step_result try_step( System system , StateInOut &x , const DerivIn &dxdt , time_type &t , time_type &dt )
    {
        m_xnew_resizer.adjust_size( x , detail::bind( &controlled_runge_kutta::template resize_m_xnew_impl< StateInOut > , detail::ref( *this ) , detail::_1 ) );
        controlled_step_result res = try_step( system , x , dxdt , t , m_xnew.m_v , dt );
        if( res == success )
        {
            boost::numeric::odeint::copy( m_xnew.m_v , x );
        }
        return res;
    }

    /*
     * Version 3 : try_step( sys , in , t , out , dt )
     *
     * this version does not solve the forwarding problem, boost.range can not be used
     *
     * the disable is needed to avoid ambiguous overloads if state_type = time_type
     */
    /**
     * \brief Tries to perform one step.
     *
     * \note This method is disabled if state_type=time_type to avoid ambiguity.
     *
     * This method tries to do one step with step size dt. If the error estimate
     * is to large, the step is rejected and the method returns fail and the 
     * step size dt is reduced. If the error estimate is acceptably small, the
     * step is performed, success is returned and dt might be increased to make 
     * the steps as large as possible. This method also updates t if a step is
     * performed.
     *
     * \param system The system function to solve, hence the r.h.s. of the ODE. It must fulfill the
     *               Simple System concept.
     * \param in The state of the ODE which should be solved.
     * \param t The value of the time. Updated if the step is successful.
     * \param out Used to store the result of the step.
     * \param dt The step size. Updated.
     * \return success if the step was accepted, fail otherwise.
     */
    template< class System , class StateIn , class StateOut >
    typename boost::disable_if< boost::is_same< StateIn , time_type > , controlled_step_result >::type
    try_step( System system , const StateIn &in , time_type &t , StateOut &out , time_type &dt )
    {
        typename odeint::unwrap_reference< System >::type &sys = system;
        m_dxdt_resizer.adjust_size( in , detail::bind( &controlled_runge_kutta::template resize_m_dxdt_impl< StateIn > , detail::ref( *this ) , detail::_1 ) );
        sys( in , m_dxdt.m_v , t );
        return try_step( system , in , m_dxdt.m_v , t , out , dt );
    }


    /*
     * Version 4 : try_step( sys , in , dxdt , t , out , dt )
     *
     * this version does not solve the forwarding problem, boost.range can not be used
     */
    /**
     * \brief Tries to perform one step.
     *
     * This method tries to do one step with step size dt. If the error estimate
     * is to large, the step is rejected and the method returns fail and the 
     * step size dt is reduced. If the error estimate is acceptably small, the
     * step is performed, success is returned and dt might be increased to make 
     * the steps as large as possible. This method also updates t if a step is
     * performed.
     *
     * \param system The system function to solve, hence the r.h.s. of the ODE. It must fulfill the
     *               Simple System concept.
     * \param in The state of the ODE which should be solved.
     * \param dxdt The derivative of state.
     * \param t The value of the time. Updated if the step is successful.
     * \param out Used to store the result of the step.
     * \param dt The step size. Updated.
     * \return success if the step was accepted, fail otherwise.
     */
    template< class System , class StateIn , class DerivIn , class StateOut >
    controlled_step_result try_step( System system , const StateIn &in , const DerivIn &dxdt , time_type &t , StateOut &out , time_type &dt )
    {
        BOOST_USING_STD_MIN();
        BOOST_USING_STD_MAX();
        using std::pow;

        m_xerr_resizer.adjust_size( in , detail::bind( &controlled_runge_kutta::template resize_m_xerr_impl< StateIn > , detail::ref( *this ) , detail::_1 ) );

        // do one step with error calculation
        m_stepper.do_step( system , in , dxdt , t , out , dt , m_xerr.m_v );

        m_max_rel_error = m_error_checker.error( m_stepper.algebra() , in , dxdt , m_xerr.m_v , dt );

        if( m_max_rel_error > 1.0 )
        {
            // error too large - decrease dt ,limit scaling factor to 0.2 and reset state
            dt *= max BOOST_PREVENT_MACRO_SUBSTITUTION ( static_cast<value_type>(9)/static_cast<value_type>(10) * pow( m_max_rel_error ,
                                                           static_cast<value_type>(-1) / ( m_stepper.error_order() - 1 ) ) ,
                       static_cast<value_type>(1)/static_cast<value_type> (5) );
            return fail;
        }
        else
        {
            if( m_max_rel_error < 0.5 )
            {
                // error should be > 0
                m_max_rel_error = max BOOST_PREVENT_MACRO_SUBSTITUTION ( pow( 5.0 , -m_stepper.stepper_order() ) , m_max_rel_error ); 
                //error too small - increase dt and keep the evolution and limit scaling factor to 5.0
                t += dt;
                dt *= static_cast<value_type>(9)/static_cast<value_type>(10) * pow( m_max_rel_error ,
                                                                                    static_cast<value_type>(-1) / m_stepper.stepper_order() );
                return success;
            }
            else
            {
                t += dt;
                return success;
            }
        }
    }

    /**
     * \brief Returns the error of the last step.
     *
     * returns The last error of the step.
     */
    value_type last_error( void ) const
    {
        return m_max_rel_error;
    }


    /**
     * \brief Adjust the size of all temporaries in the stepper manually.
     * \param x A state from which the size of the temporaries to be resized is deduced.
     */
    template< class StateType >
    void adjust_size( const StateType &x )
    {
        resize_m_xerr_impl( x );
        resize_m_dxdt_impl( x );
        resize_m_xnew_impl( x );
        m_stepper.adjust_size( x );
    }

    /**
     * \brief Returns the instance of the underlying stepper.
     * \returns The instance of the underlying stepper.
     */
    stepper_type& stepper( void )
    {
        return m_stepper;
    }

    /**
     * \brief Returns the instance of the underlying stepper.
     * \returns The instance of the underlying stepper.
     */
    const stepper_type& stepper( void ) const
    {
        return m_stepper;
    }

private:


    template< class System , class StateInOut >
    controlled_step_result try_step_v1( System system , StateInOut &x , time_type &t , time_type &dt )
    {
        typename odeint::unwrap_reference< System >::type &sys = system;
        m_dxdt_resizer.adjust_size( x , detail::bind( &controlled_runge_kutta::template resize_m_dxdt_impl< StateInOut > , detail::ref( *this ) , detail::_1 ) );
        sys( x , m_dxdt.m_v ,t );
        return try_step( system , x , m_dxdt.m_v , t , dt );
    }

    template< class StateIn >
    bool resize_m_xerr_impl( const StateIn &x )
    {
        return adjust_size_by_resizeability( m_xerr , x , typename is_resizeable<state_type>::type() );
    }

    template< class StateIn >
    bool resize_m_dxdt_impl( const StateIn &x )
    {
        return adjust_size_by_resizeability( m_dxdt , x , typename is_resizeable<deriv_type>::type() );
    }

    template< class StateIn >
    bool resize_m_xnew_impl( const StateIn &x )
    {
        return adjust_size_by_resizeability( m_xnew , x , typename is_resizeable<state_type>::type() );
    }



    stepper_type m_stepper;
    error_checker_type m_error_checker;

    resizer_type m_dxdt_resizer;
    resizer_type m_xerr_resizer;
    resizer_type m_xnew_resizer;

    wrapped_deriv_type m_dxdt;
    wrapped_state_type m_xerr;
    wrapped_state_type m_xnew;
    value_type m_max_rel_error;
};










/*
 * explicit stepper fsal version
 *
 * the class introduces the following try_step overloads
    * try_step( sys , x , t , dt ) 
    * try_step( sys , in , t , out , dt )
    * try_step( sys , x , dxdt , t , dt )
    * try_step( sys , in , dxdt_in , t , out , dxdt_out , dt )
 */
/**
 * \brief Implements step size control for Runge-Kutta FSAL steppers with 
 * error estimation.
 *
 * This class implements the step size control for FSAL Runge-Kutta 
 * steppers with error estimation.
 *
 * \tparam ErrorStepper The stepper type with error estimation, has to fulfill the ErrorStepper concept.
 * \tparam ErrorChecker The error checker
 * \tparam Resizer The resizer policy type.
 */
template<
class ErrorStepper ,
class ErrorChecker ,
class Resizer
>
class controlled_runge_kutta< ErrorStepper , ErrorChecker , Resizer , explicit_error_stepper_fsal_tag >
{

public:

    typedef ErrorStepper stepper_type;
    typedef typename stepper_type::state_type state_type;
    typedef typename stepper_type::value_type value_type;
    typedef typename stepper_type::deriv_type deriv_type;
    typedef typename stepper_type::time_type time_type;
    typedef typename stepper_type::algebra_type algebra_type;
    typedef typename stepper_type::operations_type operations_type;
    typedef Resizer resizer_type;
    typedef ErrorChecker error_checker_type;
    typedef explicit_controlled_stepper_fsal_tag stepper_category;

#ifndef DOXYGEN_SKIP
    typedef typename stepper_type::wrapped_state_type wrapped_state_type;
    typedef typename stepper_type::wrapped_deriv_type wrapped_deriv_type;

    typedef controlled_runge_kutta< ErrorStepper , ErrorChecker , Resizer , explicit_error_stepper_tag > controlled_stepper_type;
#endif // DOXYGEN_SKIP

    /**
     * \brief Constructs the controlled Runge-Kutta stepper.
     * \param error_checker An instance of the error checker.
     * \param stepper An instance of the underlying stepper.
     */
    controlled_runge_kutta(
            const error_checker_type &error_checker = error_checker_type() ,
            const stepper_type &stepper = stepper_type()
    )
    : m_stepper( stepper ) , m_error_checker( error_checker ) ,
      m_first_call( true )
    { }

    /*
     * Version 1 : try_step( sys , x , t , dt )
     *
     * The two overloads are needed in order to solve the forwarding problem
     */
    /**
     * \brief Tries to perform one step.
     *
     * This method tries to do one step with step size dt. If the error estimate
     * is to large, the step is rejected and the method returns fail and the 
     * step size dt is reduced. If the error estimate is acceptably small, the
     * step is performed, success is returned and dt might be increased to make 
     * the steps as large as possible. This method also updates t if a step is
     * performed.
     *
     * \param system The system function to solve, hence the r.h.s. of the ODE. It must fulfill the
     *               Simple System concept.
     * \param x The state of the ODE which should be solved. Overwritten if 
     * the step is successful.
     * \param t The value of the time. Updated if the step is successful.
     * \param dt The step size. Updated.
     * \return success if the step was accepted, fail otherwise.
     */
    template< class System , class StateInOut >
    controlled_step_result try_step( System system , StateInOut &x , time_type &t , time_type &dt )
    {
        return try_step_v1( system , x , t , dt );
    }


    /**
     * \brief Tries to perform one step. Solves the forwarding problem and 
     * allows for using boost range as state_type.
     *
     * This method tries to do one step with step size dt. If the error estimate
     * is to large, the step is rejected and the method returns fail and the 
     * step size dt is reduced. If the error estimate is acceptably small, the
     * step is performed, success is returned and dt might be increased to make 
     * the steps as large as possible. This method also updates t if a step is
     * performed.
     *
     * \param system The system function to solve, hence the r.h.s. of the ODE. It must fulfill the
     *               Simple System concept.
     * \param x The state of the ODE which should be solved. Overwritten if 
     * the step is successful. Can be a boost range.
     * \param t The value of the time. Updated if the step is successful.
     * \param dt The step size. Updated.
     * \return success if the step was accepted, fail otherwise.
     */
    template< class System , class StateInOut >
    controlled_step_result try_step( System system , const StateInOut &x , time_type &t , time_type &dt )
    {
        return try_step_v1( system , x , t , dt );
    }



    /*
     * Version 2 : try_step( sys , in , t , out , dt );
     *
     * This version does not solve the forwarding problem, boost::range can not be used.
     * 
     * The disabler is needed to solve ambiguous overloads
     */
    /**
     * \brief Tries to perform one step.
     *
     * \note This method is disabled if state_type=time_type to avoid ambiguity.
     *
     * This method tries to do one step with step size dt. If the error estimate
     * is to large, the step is rejected and the method returns fail and the 
     * step size dt is reduced. If the error estimate is acceptably small, the
     * step is performed, success is returned and dt might be increased to make 
     * the steps as large as possible. This method also updates t if a step is
     * performed.
     *
     * \param system The system function to solve, hence the r.h.s. of the ODE. It must fulfill the
     *               Simple System concept.
     * \param in The state of the ODE which should be solved.
     * \param t The value of the time. Updated if the step is successful.
     * \param out Used to store the result of the step.
     * \param dt The step size. Updated.
     * \return success if the step was accepted, fail otherwise.
     */
    template< class System , class StateIn , class StateOut >
    typename boost::disable_if< boost::is_same< StateIn , time_type > , controlled_step_result >::type
    try_step( System system , const StateIn &in , time_type &t , StateOut &out , time_type &dt )
    {
        if( m_dxdt_resizer.adjust_size( in , detail::bind( &controlled_runge_kutta::template resize_m_dxdt_impl< StateIn > , detail::ref( *this ) , detail::_1 ) ) || m_first_call )
        {
            initialize( system , in , t );
        }
        return try_step( system , in , m_dxdt.m_v , t , out , dt );
    }


    /*
     * Version 3 : try_step( sys , x , dxdt , t , dt )
     *
     * This version does not solve the forwarding problem, boost::range can not be used.
     */
    /**
     * \brief Tries to perform one step.
     *
     * This method tries to do one step with step size dt. If the error estimate
     * is to large, the step is rejected and the method returns fail and the 
     * step size dt is reduced. If the error estimate is acceptably small, the
     * step is performed, success is returned and dt might be increased to make 
     * the steps as large as possible. This method also updates t if a step is
     * performed.
     *
     * \param system The system function to solve, hence the r.h.s. of the ODE. It must fulfill the
     *               Simple System concept.
     * \param x The state of the ODE which should be solved. Overwritten if 
     * the step is successful.
     * \param dxdt The derivative of state.
     * \param t The value of the time. Updated if the step is successful.
     * \param dt The step size. Updated.
     * \return success if the step was accepted, fail otherwise.
     */
    template< class System , class StateInOut , class DerivInOut >
    controlled_step_result try_step( System system , StateInOut &x , DerivInOut &dxdt , time_type &t , time_type &dt )
    {
        m_xnew_resizer.adjust_size( x , detail::bind( &controlled_runge_kutta::template resize_m_xnew_impl< StateInOut > , detail::ref( *this ) , detail::_1 ) );
        m_dxdt_new_resizer.adjust_size( x , detail::bind( &controlled_runge_kutta::template resize_m_dxdt_new_impl< StateInOut > , detail::ref( *this ) , detail::_1 ) );
        controlled_step_result res = try_step( system , x , dxdt , t , m_xnew.m_v , m_dxdtnew.m_v , dt );
        if( res == success )
        {
            boost::numeric::odeint::copy( m_xnew.m_v , x );
            boost::numeric::odeint::copy( m_dxdtnew.m_v , dxdt );
        }
        return res;
    }


    /*
     * Version 4 : try_step( sys , in , dxdt_in , t , out , dxdt_out , dt )
     *
     * This version does not solve the forwarding problem, boost::range can not be used.
     */
    /**
     * \brief Tries to perform one step.
     *
     * This method tries to do one step with step size dt. If the error estimate
     * is to large, the step is rejected and the method returns fail and the 
     * step size dt is reduced. If the error estimate is acceptably small, the
     * step is performed, success is returned and dt might be increased to make 
     * the steps as large as possible. This method also updates t if a step is
     * performed.
     *
     * \param system The system function to solve, hence the r.h.s. of the ODE. It must fulfill the
     *               Simple System concept.
     * \param in The state of the ODE which should be solved.
     * \param dxdt The derivative of state.
     * \param t The value of the time. Updated if the step is successful.
     * \param out Used to store the result of the step.
     * \param dt The step size. Updated.
     * \return success if the step was accepted, fail otherwise.
     */
    template< class System , class StateIn , class DerivIn , class StateOut , class DerivOut >
    controlled_step_result try_step( System system , const StateIn &in , const DerivIn &dxdt_in , time_type &t ,
            StateOut &out , DerivOut &dxdt_out , time_type &dt )
    {
        BOOST_USING_STD_MIN();
        BOOST_USING_STD_MAX();

        using std::pow;

        m_xerr_resizer.adjust_size( in , detail::bind( &controlled_runge_kutta::template resize_m_xerr_impl< StateIn > , detail::ref( *this ) , detail::_1 ) );

        //fsal: m_stepper.get_dxdt( dxdt );
        //fsal: m_stepper.do_step( sys , x , dxdt , t , dt , m_x_err );
        m_stepper.do_step( system , in , dxdt_in , t , out , dxdt_out , dt , m_xerr.m_v );

        // this potentially overwrites m_x_err! (standard_error_checker does, at least)
        value_type max_rel_err = m_error_checker.error( m_stepper.algebra() , in , dxdt_in , m_xerr.m_v , dt );

        if( max_rel_err > 1.0 )
        {
            // error too large - decrease dt ,limit scaling factor to 0.2 and reset state
            dt *= max BOOST_PREVENT_MACRO_SUBSTITUTION ( static_cast<value_type>( static_cast<value_type>(9)/static_cast<value_type>(10) * pow( max_rel_err , static_cast<value_type>(-1) / ( m_stepper.error_order() - 1 ) ) ) , static_cast<value_type>( static_cast<value_type>(1)/static_cast<value_type> (5)) );
            return fail;
        }
        else
        {
            if( max_rel_err < 0.5 )
            {                //error too small - increase dt and keep the evolution and limit scaling factor to 5.0
                // error should be > 0
                max_rel_err = max BOOST_PREVENT_MACRO_SUBSTITUTION ( pow( 5.0 , -m_stepper.stepper_order() ) , max_rel_err ); 
                t += dt;
                dt *= static_cast<value_type>( static_cast<value_type>(9)/static_cast<value_type>(10) * pow( max_rel_err , static_cast<value_type>(-1) / m_stepper.stepper_order() ) );
                return success;
            }
            else
            {
                t += dt;
                return success;
            }
        }
    }


    /**
     * \brief Resets the internal state of the underlying FSAL stepper.
     */
    void reset( void )
    {
        m_first_call = true;
    }

    /**
     * \brief Initializes the internal state storing an internal copy of the derivative.
     *
     * \param deriv The initial derivative of the ODE.
     */
    template< class DerivIn >
    void initialize( const DerivIn &deriv )
    {
        boost::numeric::odeint::copy( deriv , m_dxdt.m_v );
        m_first_call = false;
    }

    /**
     * \brief Initializes the internal state storing an internal copy of the derivative.
     *
     * \param system The system function to solve, hence the r.h.s. of the ODE. It must fulfill the
     *               Simple System concept.
     * \param x The initial state of the ODE which should be solved.
     * \param t The initial time.
     */
    template< class System , class StateIn >
    void initialize( System system , const StateIn &x , time_type t )
    {
        typename odeint::unwrap_reference< System >::type &sys = system;
        sys( x , m_dxdt.m_v , t );
        m_first_call = false;
    }

    /**
     * \brief Returns true if the stepper has been initialized, false otherwise.
     *
     * \return true, if the stepper has been initialized, false otherwise.
     */
    bool is_initialized( void ) const
    {
        return ! m_first_call;
    }


    /**
     * \brief Adjust the size of all temporaries in the stepper manually.
     * \param x A state from which the size of the temporaries to be resized is deduced.
     */
    template< class StateType >
    void adjust_size( const StateType &x )
    {
        resize_m_xerr_impl( x );
        resize_m_dxdt_impl( x );
        resize_m_dxdt_new_impl( x );
        resize_m_xnew_impl( x );
    }


    /**
     * \brief Returns the instance of the underlying stepper.
     * \returns The instance of the underlying stepper.
     */
    stepper_type& stepper( void )
    {
        return m_stepper;
    }

    /**
     * \brief Returns the instance of the underlying stepper.
     * \returns The instance of the underlying stepper.
     */
    const stepper_type& stepper( void ) const
    {
        return m_stepper;
    }



private:


    template< class StateIn >
    bool resize_m_xerr_impl( const StateIn &x )
    {
        return adjust_size_by_resizeability( m_xerr , x , typename is_resizeable<state_type>::type() );
    }

    template< class StateIn >
    bool resize_m_dxdt_impl( const StateIn &x )
    {
        return adjust_size_by_resizeability( m_dxdt , x , typename is_resizeable<deriv_type>::type() );
    }

    template< class StateIn >
    bool resize_m_dxdt_new_impl( const StateIn &x )
    {
        return adjust_size_by_resizeability( m_dxdtnew , x , typename is_resizeable<deriv_type>::type() );
    }

    template< class StateIn >
    bool resize_m_xnew_impl( const StateIn &x )
    {
        return adjust_size_by_resizeability( m_xnew , x , typename is_resizeable<state_type>::type() );
    }


    template< class System , class StateInOut >
    controlled_step_result try_step_v1( System system , StateInOut &x , time_type &t , time_type &dt )
    {
        if( m_dxdt_resizer.adjust_size( x , detail::bind( &controlled_runge_kutta::template resize_m_dxdt_impl< StateInOut > , detail::ref( *this ) , detail::_1 ) ) || m_first_call )
        {
            initialize( system , x , t );
        }
        return try_step( system , x , m_dxdt.m_v , t , dt );
    }


    stepper_type m_stepper;
    error_checker_type m_error_checker;

    resizer_type m_dxdt_resizer;
    resizer_type m_xerr_resizer;
    resizer_type m_xnew_resizer;
    resizer_type m_dxdt_new_resizer;

    wrapped_deriv_type m_dxdt;
    wrapped_state_type m_xerr;
    wrapped_state_type m_xnew;
    wrapped_deriv_type m_dxdtnew;
    bool m_first_call;
};


/********** DOXYGEN **********/

/**** DEFAULT ERROR CHECKER ****/

/**
 * \class default_error_checker
 * \brief The default error checker to be used with Runge-Kutta error steppers
 *
 * This class provides the default mechanism to compare the error estimates 
 * reported by Runge-Kutta error steppers with user defined error bounds.
 * It is used by the controlled_runge_kutta steppers.
 *
 * \tparam Value The value type.
 * \tparam Algebra The algebra type.
 * \tparam Operations The operations type.
 */

    /**
     * \fn default_error_checker( value_type eps_abs , value_type eps_rel , value_type a_x , value_type a_dxdt )
     * \brief Constructs the error checker.
     *
     * The error is calculated as follows: ???? 
     *
     * \param eps_abs Absolute tolerance level.
     * \param eps_rel Relative tolerance level.
     * \param a_x Factor for the weight of the state.
     * \param a_dxdt Factor for the weight of the derivative.
     */
    
    /**
     * \fn error( const State &x_old , const Deriv &dxdt_old , Err &x_err , Time dt ) const
     * \brief Calculates the error level.
     *
     * If the returned error level is greater than 1, the estimated error was
     * larger than the permitted error bounds and the step should be repeated
     * with a smaller step size.
     *
     * \param x_old State at the beginning of the step.
     * \param dxdt_old Derivative at the beginning of the step.
     * \param x_err Error estimate.
     * \param dt Time step.
     * \return error
     */

    /**
     * \fn error( algebra_type &algebra , const State &x_old , const Deriv &dxdt_old , Err &x_err , Time dt ) const
     * \brief Calculates the error level using a given algebra.
     *
     * If the returned error level is greater than 1, the estimated error was
     * larger than the permitted error bounds and the step should be repeated
     * with a smaller step size.
     *
     * \param algebra The algebra used for calculation of the error.
     * \param x_old State at the beginning of the step.
     * \param dxdt_old Derivative at the beginning of the step.
     * \param x_err Error estimate.
     * \param dt Time step.
     * \return error
     */


} // odeint
} // numeric
} // boost


#endif // BOOST_NUMERIC_ODEINT_STEPPER_CONTROLLED_RUNGE_KUTTA_HPP_INCLUDED
