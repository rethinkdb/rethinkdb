/*
  [auto_generated]
  libs/numeric/odeint/test/stepper_copying.cpp

  [begin_description]
  This file tests the copying of the steppers.
  [end_description]

  Copyright 2009-2012 Karsten Ahnert
  Copyright 2009-2012 Mario Mulansky

  Distributed under the Boost Software License, Version 1.0.
  (See accompanying file LICENSE_1_0.txt or
  copy at http://www.boost.org/LICENSE_1_0.txt)
*/

// disable checked iterator warning for msvc
#include <boost/config.hpp>
#ifdef BOOST_MSVC
#pragma warning(disable:4996)
#endif


#define BOOST_TEST_MODULE odeint_stepper_copying

#include <boost/test/unit_test.hpp>
#include <boost/type_traits/integral_constant.hpp>

//#include <boost/numeric/odeint/util/construct.hpp>
//#include <boost/numeric/odeint/util/destruct.hpp>
#include <boost/numeric/odeint/util/copy.hpp>

#include <boost/numeric/odeint/util/state_wrapper.hpp>

#include <boost/numeric/odeint/stepper/euler.hpp>
#include <boost/numeric/odeint/stepper/runge_kutta4_classic.hpp>
#include <boost/numeric/odeint/stepper/runge_kutta4.hpp>
#include <boost/numeric/odeint/stepper/runge_kutta_cash_karp54_classic.hpp>
#include <boost/numeric/odeint/stepper/runge_kutta_cash_karp54.hpp>
#include <boost/numeric/odeint/stepper/runge_kutta_dopri5.hpp>
#include <boost/numeric/odeint/stepper/controlled_runge_kutta.hpp>
#include <boost/numeric/odeint/stepper/dense_output_runge_kutta.hpp>

template< class T , size_t Dim >
class test_array
{
public:

    const static size_t dim = Dim;
    typedef T value_type;
    typedef value_type* iterator;
    typedef const value_type* const_iterator;

    value_type& operator[]( size_t i )
    {
        return m_data[i];
    }

    const value_type& operator[]( size_t i ) const
    {
        return m_data[i];
    }

    iterator begin( void )
    {
        return m_data;
    }

    iterator end( void )
    {
        return m_data + dim;
    }

    const_iterator begin( void ) const
    {
        return m_data;
    }

    const_iterator end( void ) const
    {
        return m_data + dim;
    }


private:

    value_type m_data[dim];
};

template< class T , size_t Dim >
class test_array2 : public test_array< T , Dim >
{
};



/*
 * Explicit testing if copying was successful is difficult,
 * hence we only test if the number of copy operations is right.
 *
 * Otherwise one has to prepare the states.
 */

size_t construct_count = 0;
size_t construct2_count = 0;
size_t destruct_count = 0;
size_t destruct2_count = 0;
size_t copy_count = 0;
size_t copy2_count = 0;

void reset_counter( void )
{
    construct_count = 0;
    construct2_count = 0;
    destruct_count = 0;
    destruct2_count = 0;
    copy_count = 0;
    copy2_count = 0;
}


namespace boost { namespace numeric { namespace odeint {

//provide the state_wrapper
            template< class T , size_t Dim >
            struct state_wrapper< test_array< T , Dim > >
            {
                typedef state_wrapper< test_array< T , Dim > > state_wrapper_type;
                typedef test_array< T , Dim > state_type;
                typedef T value_type;

                state_type m_v;

                state_wrapper() : m_v()
                {
                    construct_count++;
                }

                state_wrapper( const state_type &v ) : m_v( v )
                {
                    construct_count++;
                    copy_count++;
                }

                state_wrapper( const state_wrapper_type &x ) : m_v( x.m_v )
                {
                    construct_count++;
                    copy_count++;
                }

                state_wrapper_type& operator=( const state_wrapper_type &x )
                {
                    copy_count++;
                    return *this;
                }

                ~state_wrapper()
                {
                    destruct_count++;
                }
            };

//provide the state_wrapper
            template< class T , size_t Dim >
            struct state_wrapper< test_array2< T , Dim > >
            {
                typedef state_wrapper< test_array2< T , Dim > > state_wrapper_type;
                typedef test_array2< T , Dim > state_type;
                typedef T value_type;

                state_type m_v;

                state_wrapper() : m_v()
                {
                    construct2_count++;
                }

                state_wrapper( const state_type &v ) : m_v( v )
                {
                    construct2_count++;
                    copy2_count++;
                }

                state_wrapper( const state_wrapper_type &x ) : m_v( x.m_v )
                {
                    construct2_count++;
                    copy2_count++;
                }

                state_wrapper_type& operator=( const state_wrapper_type &x )
                {
                    copy2_count++;
                    return *this;
                }

                ~state_wrapper()
                {
                    destruct2_count++;
                }
            };


        } } }



typedef test_array< double , 3 > state_type;
typedef test_array2< double , 3 > deriv_type;
typedef boost::numeric::odeint::euler< state_type , double , deriv_type > euler_type;
typedef boost::numeric::odeint::runge_kutta4_classic< state_type , double , deriv_type > rk4_type;
typedef boost::numeric::odeint::runge_kutta4< state_type , double , deriv_type > rk4_generic_type;
typedef boost::numeric::odeint::runge_kutta_cash_karp54_classic< state_type , double , deriv_type > rk54_type;
typedef boost::numeric::odeint::runge_kutta_cash_karp54< state_type , double , deriv_type > rk54_generic_type;
typedef boost::numeric::odeint::runge_kutta_dopri5< state_type , double , deriv_type > dopri5_type;
typedef boost::numeric::odeint::controlled_runge_kutta< rk54_type > controlled_rk54_type;
typedef boost::numeric::odeint::controlled_runge_kutta< rk54_generic_type > controlled_rk54_generic_type;
typedef boost::numeric::odeint::controlled_runge_kutta< dopri5_type > controlled_dopri5_type;
typedef boost::numeric::odeint::dense_output_runge_kutta< euler_type > dense_output_euler_type;
typedef boost::numeric::odeint::dense_output_runge_kutta< controlled_dopri5_type > dense_output_dopri5_type;

#define CHECK_COUNTERS( c1 , c2 , c3 , c4 , c5 , c6 )           \
    BOOST_CHECK_EQUAL( construct_count , size_t( c1 ) );        \
    BOOST_CHECK_EQUAL( construct2_count , size_t( c2 ) );       \
    BOOST_CHECK_EQUAL( destruct_count , size_t( c3 ) );         \
    BOOST_CHECK_EQUAL( destruct2_count , size_t( c4) );         \
    BOOST_CHECK_EQUAL( copy_count , size_t( c5 ) ) ;            \
    BOOST_CHECK_EQUAL( copy2_count, size_t( c6 ) )

BOOST_AUTO_TEST_SUITE( stepper_copying )

/*
 * Construct + Destruct
 * 1 deriv_type in explicit_stepper_base
 */
BOOST_AUTO_TEST_CASE( explicit_euler_construct )
{
    reset_counter();
    {
        euler_type euler;
    }
    CHECK_COUNTERS( 0 , 1 , 0 , 1 , 0 , 0 );
}


/*
 * Construct + Destruct
 * 2 * 1 deriv_type in explicit_stepper_base
 *
 * Copying
 * 1 deriv_type in explicit_stepper_base
 */
BOOST_AUTO_TEST_CASE( explicit_euler_copy_construct )
{
    reset_counter();
    {
        euler_type euler;
        euler_type euler2( euler );
    }
    CHECK_COUNTERS( 0 , 1 + 1 , 0 , 1 + 1 , 0 , 1 );
}

/*
 * Construct + Destruct
 * 2 * 1 deriv_type in explicit_stepper_base
 *
 * Copying
 * 1 deriv_type in explicit_stepper_base
 */
BOOST_AUTO_TEST_CASE( explicit_euler_assign )
{
    reset_counter();
    {
        euler_type euler;
        euler_type euler2;
        euler2 = euler;
    }
    CHECK_COUNTERS( 0 , 2 , 0 , 2 , 0 , 1 );
}

/*
 * Construct + Destruct
 * 1 deriv_type in explicit_stepper_base
 * 3 deriv_type in explicit_rk4
 * 1 state_type in explicit_rk4
 */
BOOST_AUTO_TEST_CASE( explicit_rk4_construct )
{
    reset_counter();
    {
        rk4_type rk4;
    }
    CHECK_COUNTERS( 1 , 4 , 1 , 4 , 0 , 0 );
}

/*
 * Construct + Destruct
 * 2 * 1 deriv_type in explicit_stepper_base
 * 2 * 3 deriv_type in explicit_rk4
 * 2 * 1 state_type in explicit_rk4
 *
 * Copying
 * 1 deriv_type in explicit_stepper_base
 * 3 deriv_type in explicit_stepper_base
 * 1 state_type in explicit_stepper_base
 */
BOOST_AUTO_TEST_CASE( explicit_rk4_copy_construct )
{
    reset_counter();
    {
        rk4_type rk4;
        rk4_type rk4_2( rk4 );
    }
    CHECK_COUNTERS( 2 , 8 , 2 , 8 , 1 , 4 );
}

/*
 * Construct + Destruct
 * 2 * 1 deriv_type in explicit_stepper_base
 * 2 * 3 deriv_type in explicit_rk4
 * 2 * 1 state_type in explicit_rk4
 *
 * Copying
 * 1 deriv_type in explicit_stepper_base
 * 3 deriv_type in explicit_stepper_base
 * 1 state_type in explicit_stepper_base
 */
BOOST_AUTO_TEST_CASE( explicit_rk4_assign )
{
    reset_counter();
    {
        rk4_type rk4;
        rk4_type rk4_2;
        rk4 = rk4_2;
    }
    CHECK_COUNTERS( 2 , 8 , 2 , 8 , 1 , 4 );
}


/*
 * Construct + Destruct
 * 1 deriv_type in explicit_stepper_base
 * 3 deriv_type in explicit_rk4
 * 1 state_type in explicit_rk4
 */
BOOST_AUTO_TEST_CASE( explicit_rk4_generic_construct )
{
    reset_counter();
    {
        rk4_generic_type rk4;
    }
    CHECK_COUNTERS( 1 , 4 , 1 , 4 , 0 , 0 );
}

/*
 * Construct + Destruct
 * 2 * 1 deriv_type in explicit_stepper_base
 * 2 * 3 deriv_type in explicit_rk4
 * 2 * 1 state_type in explicit_rk4
 *
 * Copying
 * 1 deriv_type in explicit_stepper_base
 * 3 deriv_type in explicit_stepper_base
 * 1 state_type in explicit_stepper_base
 */
BOOST_AUTO_TEST_CASE( explicit_rk4_generic_copy_construct )
{
    reset_counter();
    {
        rk4_generic_type rk4;
        rk4_generic_type rk4_2( rk4 );
    }
    CHECK_COUNTERS( 2 , 8 , 2 , 8 , 1 , 4 );
}

/*
 * Construct + Destruct
 * 2 * 1 deriv_type in explicit_stepper_base
 * 2 * 3 deriv_type in explicit_rk4
 * 2 * 1 state_type in explicit_rk4
 *
 * Copying
 * 1 deriv_type in explicit_stepper_base
 * 3 deriv_type in explicit_stepper_base
 * 1 state_type in explicit_stepper_base
 */
BOOST_AUTO_TEST_CASE( explicit_rk4_generic_assign )
{
    reset_counter();
    {
        rk4_generic_type rk4;
        rk4_generic_type rk4_2;
        rk4 = rk4_2;
    }
    CHECK_COUNTERS( 2 , 8 , 2 , 8 , 1 , 4 );
}

/*
 * Construct + Destruct
 * 2 explicit_rk54_ck:
 * 2 * 1 deriv_type in explicit_error_stepper_base
 * 2 * 5 deriv_type in explicit_error_rk54_ck
 * 2 * 1 state_type in explicit_error_rk4
 * 1 controlled_stepper:
 * 1 deriv_type
 * 2 state_type
 *
 * Copying
 * 1 copy process of explicit_rk54_ck:
 * 1 deriv_type from explicit_error_stepper_base
 * 5 deriv_type from explicit_error_rk54_ck
 * 1 state_type from explicit_error_rk54_ck
 */
BOOST_AUTO_TEST_CASE( controlled_rk54_construct )
{
    reset_counter();
    {
        controlled_rk54_type stepper;
    }
    CHECK_COUNTERS( 4 , 13 , 4 , 13 , 1 , 6 );
}


/*
 * Construct + Destruct
 * 3 explicit_rk54_ck:
 * 3 * 1 deriv_type in explicit_error_stepper_base
 * 3 * 5 deriv_type in explicit_error_rk54_ck
 * 3 * 1 state_type in explicit_error_rk4
 * 2 controlled_stepper:
 * 2 * 1 deriv_type
 * 2 * 2 state_type
 *
 * Copying
 * 1 copy process of explicit_rk54_ck:
 * 1 deriv_type from explicit_error_stepper_base
 * 5 deriv_type from explicit_error_rk54_ck
 * 1 state_type from explicit_error_rk54_ck
 *
 * 1 process of copying controlled_error_stepper
 * 1 deriv_type from explicit_error_stepper_base
 * 5 deriv_type from explicit_error_rk54_ck
 * 1 state_type from explicit_error_rk54_ck
 * 1 deriv_type from controlled_error_stepper
 * 2 state_type from controlled_error_stepper
 */
BOOST_AUTO_TEST_CASE( controlled_rk54_copy_construct )
{
    reset_counter();
    {
        controlled_rk54_type stepper;
        controlled_rk54_type stepper2( stepper );
    }
    CHECK_COUNTERS( 7 , 20 , 7 , 20 , 4 , 13 );
}

/*
 * Construct + Destruct
 * 4 explicit_rk54_ck:
 * 4 * 1 deriv_type in explicit_error_stepper_base
 * 4 * 5 deriv_type in explicit_error_rk54_ck
 * 4 * 1 state_type in explicit_error_rk4
 * 2 controlled_stepper:
 * 2 * 1 deriv_type
 * 2 * 2 state_type
 *
 * Copying
 * 2 copy process of explicit_rk54_ck:
 * 2 * 1 deriv_type from explicit_error_stepper_base
 * 2 * 5 deriv_type from explicit_error_rk54_ck
 * 2 * 1 state_type from explicit_error_rk54_ck
 *
 * 1 process of copying controlled_error_stepper
 * 1 deriv_type from explicit_error_stepper_base
 * 5 deriv_type from explicit_error_rk54_ck
 * 1 state_type from explicit_error_rk54_ck
 * 1 deriv_type from controlled_error_stepper
 * 2 state_type from controlled_error_stepper
 */
BOOST_AUTO_TEST_CASE( controlled_rk54_assign )
{
    reset_counter();
    {
        controlled_rk54_type stepper;
        controlled_rk54_type stepper2;
        stepper2 = stepper;
    }
    CHECK_COUNTERS( 8 , 26 , 8 , 26 , 5 , 19 );
}



/*
 * Construct + Destruct
 * 2 explicit_rk54_ck_generic:
 * 2 * 1 deriv_type in explicit_error_stepper_base
 * 2 * 5 deriv_type in explicit_error_rk54_ck_generic
 * 2 * 1 state_type in explicit_error_rk54_ck_generic
 * 1 controlled_stepper:
 * 1 deriv_type
 * 2 state_type
 *
 * Copying
 * 1 copy process of explicit_rk54_ck_generic:
 * 1 deriv_type from explicit_error_stepper_base
 * 5 deriv_type from explicit_error_rk54_ck_generic
 * 1 state_type from explicit_error_rk54_ck_generic
 */
BOOST_AUTO_TEST_CASE( controlled_rk54_generic_construct )
{
    reset_counter();
    {
        controlled_rk54_generic_type stepper;
    }
    CHECK_COUNTERS( 4 , 13 , 4 , 13 , 1 , 6 );
}


/*
 * Construct + Destruct
 * 3 explicit_rk54_ck_generic:
 * 3 * 1 deriv_type in explicit_error_stepper_base
 * 3 * 5 deriv_type in explicit_error_rk54_ck_generic
 * 3 * 1 state_type in explicit_error_rk4_generic
 * 2 controlled_stepper:
 * 2 * 1 deriv_type
 * 2 * 2 state_type
 *
 * Copying
 * 1 copy process of explicit_rk54_ck_generic:
 * 1 deriv_type from explicit_error_stepper_base
 * 5 deriv_type from explicit_error_rk54_ck_generic
 * 1 state_type from explicit_error_rk54_ck_generic
 *
 * 1 process of copying controlled_error_stepper
 * 1 deriv_type from explicit_error_stepper_base
 * 5 deriv_type from explicit_error_rk54_ck_generic
 * 1 state_type from explicit_error_rk54_ck_generic
 * 1 deriv_type from controlled_error_stepper
 * 2 state_type from controlled_error_stepper
 */
BOOST_AUTO_TEST_CASE( controlled_rk54_generic_copy_construct )
{
    reset_counter();
    {
        controlled_rk54_generic_type stepper;
        controlled_rk54_generic_type stepper2( stepper );
    }
    CHECK_COUNTERS( 7 , 20 , 7 , 20 , 4 , 13 );
}

/*
 * Construct + Destruct
 * 4 explicit_rk54_ck_generic:
 * 4 * 1 deriv_type in explicit_error_stepper_base
 * 4 * 5 deriv_type in explicit_error_rk54_ck_generic
 * 4 * 1 state_type in explicit_error_rk4_generic
 * 2 controlled_stepper:
 * 2 * 1 deriv_type
 * 2 * 2 state_type
 *
 * Copying
 * 2 copy process of explicit_rk54_ck_generic:
 * 2 * 1 deriv_type from explicit_error_stepper_base
 * 2 * 5 deriv_type from explicit_error_rk54_ck_generic
 * 2 * 1 state_type from explicit_error_rk54_ck_generic
 *
 * 1 process of copying controlled_error_stepper
 * 1 deriv_type from explicit_error_stepper_base
 * 5 deriv_type from explicit_error_rk54_ck_generic
 * 1 state_type from explicit_error_rk54_ck_generic
 * 1 deriv_type from controlled_error_stepper
 * 2 state_type from controlled_error_stepper
 */
BOOST_AUTO_TEST_CASE( controlled_rk54_generic_assign )
{
    reset_counter();
    {
        controlled_rk54_generic_type stepper;
        controlled_rk54_generic_type stepper2;
        stepper2 = stepper;
    }
    CHECK_COUNTERS( 8 , 26 , 8 , 26 , 5 , 19 );
}


/*
 * Construct + Destruct
 * 2 explicit_error_dopri5:
 * 2 * 1 deriv_type in explicit_error_stepper_base_fsal
 * 2 * 6 deriv_type in explicit_error_dopri5
 * 2 * 1 state_type in explicit_error_dopri5
 * 1 controlled_error_stepper (fsal):
 * 2 deriv_type
 * 2 state_type
 *
 * Copying
 * 1 copy process of explicit_dopri5:
 * 1 deriv_type from explicit_error_stepper_base_fsal
 * 6 deriv_type from explicit_error_dopri5
 * 1 state_type from explicit_error_dopri5
 */

BOOST_AUTO_TEST_CASE( controlled_dopri5_construct )
{
    reset_counter();
    {
        controlled_dopri5_type dopri5;
    }
    CHECK_COUNTERS( 2 * 1 + 2 , 2 * (6+1) + 2 , 2 * 1 + 2 , 2 * (6+1) + 2 , 1 , 1 + 6 );
}


/*
 * Construct + Destruct
 * 3 explicit_error_dopri5:
 * 3 * 1 deriv_type in explicit_error_stepper_base_fsal
 * 3 * 6 deriv_type in explicit_error_dopri5
 * 3 * 1 state_type in explicit_error_dopri5
 * 2 controlled_error_stepper (fsal):
 * 2 * 2 deriv_type
 * 2 * 2 state_type
 *
 * Copying
 * 1 copy process of explicit_error_dopri5:
 * 1 deriv_type from explicit_error_stepper_base_fsal
 * 6 deriv_type from explicit_error_error_dopri5
 * 1 state_type from explicit_error_error_dopri5
 *
 * 1 process of copying controlled_error_stepper
 * 1 deriv_type from explicit_error_stepper_base_fsal
 * 6 deriv_type from explicit_error_dopri5
 * 1 state_type from explicit_error_dopri5
 * 2 deriv_type from controlled_error_stepper (fsal)
 * 2 state_type from controlled_error_stepper (fsal)
 */
BOOST_AUTO_TEST_CASE( controlled_dopri5_copy_construct )
{
    reset_counter();
    {
        controlled_dopri5_type dopri5;
        controlled_dopri5_type dopri5_2( dopri5 );
    }
    CHECK_COUNTERS( 3 * 1 + 2 * 2 , 3 * (6+1) + 2 * 2 ,  3 * 1 + 2 * 2 , 3 * (6+1) + 2 * 2 , 1 + 1 + 2 , 1 + 6 + 1 + 6 + 2 );
}

/*
 * Construct + Destruct
 * 4 explicit_error_dopri5:
 * 4 * 1 deriv_type in explicit_error_stepper_base_fsal
 * 4 * 6 deriv_type in explicit_error_dopri5
 * 4 * 1 state_type in explicit_error_dopri5
 * 2 controlled_error_stepper (fsal):
 * 2 * 2 deriv_type
 * 2 * 2 state_type
 *
 * Copying
 * 2 copy process of explicit_error_dopri5:
 * 2 * 1 deriv_type from explicit_error_stepper_base_fsal
 * 2 * 6 deriv_type from explicit_error_dopri5
 * 2 * 1 state_type from explicit_error_dopri5
 *
 * 1 process of copying controlled_error_stepper
 * 1 deriv_type from explicit_error_stepper_base
 * 6 deriv_type from explicit_error_dopri5
 * 1 state_type from explicit_error_dopri5
 * 2 deriv_type from controlled_error_stepper (fsal)
 * 2 state_type from controlled_error_stepper (fsal)
 */
BOOST_AUTO_TEST_CASE( controlled_dopri5_assign )
{
    reset_counter();
    {
        controlled_dopri5_type dopri5;
        controlled_dopri5_type dopri5_2;
        dopri5_2 = dopri5;
    }
    CHECK_COUNTERS( 4 * 1 + 2 * 2 , 4 * (1+6) + 2 * 2 , 4 * 1 + 2 * 2 , 4 * (1+6) + 2 * 2 , 2 * 1 + 1 + 2 , 2 * (6+1) + 1 + 6 + 2 );
}


/*
 * Construct + Destruct
 * 2 explicit_euler:
 * 2 * 1 deriv_type in explicit_stepper_base
 * 1 dense_output_explicit:
 * 2 state_type
 *
 * Copying
 * 1 copy process of explicit_euler:
 * 1 deriv_type from explicit_stepper_base
 */
BOOST_AUTO_TEST_CASE( dense_output_euler_construct )
{
    reset_counter();
    {
        dense_output_euler_type euler;
    }
    CHECK_COUNTERS( 2 , 2 * 1 , 2 , 2 * 1 , 0 , 1 );
}

/*
 * Construct + Destruct
 * 3 explicit_euler:
 * 3 * 1 deriv_type in explicit_stepper_base
 * 2 dense_output_explicit:
 * 2 * 2 state_type
 *
 * Copying
 * 1 copy process of explicit_euler:
 * 1 deriv_type from explicit_stepper_base
 *
 * 1 process of copying 
 * 1 deriv_type from explicit_stepper_base
 * 2 state_type from dense_output_explicit
 */
BOOST_AUTO_TEST_CASE( dense_output_euler_copy_construct )
{
    reset_counter();
    {
        dense_output_euler_type euler;
        dense_output_euler_type euler2( euler );
    }
    CHECK_COUNTERS( 2 * 2 , 3 * 1 , 2 * 2 , 3 * 1 , 2 , 1 + 1 );
}

/*
 * Construct + Destruct
 * 4 explicit_euler:
 * 4 * 1 deriv_type in explicit_stepper_base
 * 2 dense_output_explicit:
 * 2 * 2 state_type
 *
 * Copying
 * 2 copy process of explicit_euler:
 * 2 * 1 deriv_type from explicit_stepper_base
 *
 * 1 process of copying dense_ouput_explicit
 * 1 deriv_type from explicit_stepper_base
 * 2 state_type from dense_output_explicit
 */
BOOST_AUTO_TEST_CASE( dense_output_euler_assign )
{
    reset_counter();
    {
        dense_output_euler_type euler;
        dense_output_euler_type euler2;
        euler2 = euler;
    }
    CHECK_COUNTERS( 2 * 2 , 4 * 1 , 2 * 2 , 4 * 1 , 2 , 2 * 1 + 1 );
}

/*
 * Construct + Destruct
 * 3 dense_output_dopri5:
 * 3 * 1 deriv_type in explicit_error_stepper_base_fsal
 * 3 * 6 deriv_type in explicit_error_dopri5
 * 3 * 1 state_type in explicit_error_dopri5
 * 2 controlled_error_stepper (fsal):
 * 2 * 2 state_type
 * 2 * 2 deriv_type
 * 1 dense_output_controlled_explicit:
 * 2 state_type
 * 2 deriv_type
 *
 * Copying
 * 2 copy process of explicit_error_dopri5:
 * 2 * 1 deriv_type from explicit_erro_stepper_base_fsal
 * 2 * 6 deriv_type in explicit_error_dopri5
 * 2 * 1 state_type in explicit_error_dopri5
 * 1 copy process of dense_output_controlled (fsal)
 * 2 state_type
 * 2 deriv_type
 */
BOOST_AUTO_TEST_CASE( dense_output_dopri5_construct )
{
    reset_counter();
    {
        dense_output_dopri5_type dopri5;
    }
    CHECK_COUNTERS( 3*1 + 2*2 + 2 , 3*(1+6) + 2*2 + 2 , 3*1 + 2*2 + 2 , 3*(1+6) + 2*2 + 2 , 2*1 + 2 , 2*(1+6) + 2 );
}

/*
 * Construct + Destruct
 * 4 dense_output_dopri5:
 * 4 * 1 deriv_type in explicit_error_stepper_base_fsal
 * 4 * 5 deriv_type in explicit_error_dopri5
 * 4 * 1 state_type in explicit_error_dopri5
 * 3 controlled_error_stepper (fsal):
 * 3 * 2 state_type
 * 3 * 2 deriv_type
 * 2 dense_output_controlled_explicit:
 * 2 * 2 state_type
 * 2 * 2 deriv_type
 *
 * Copying
 * 3 copy process of explicit_error_dopri5:
 * 3 * 1 deriv_type from explicit_erro_stepper_base_fsal
 * 3 * 6 deriv_type in explicit_error_dopri5
 * 3 * 1 state_type in explicit_error_dopri5
 * 2 copy process of controlled_error_stepper (fsal):
 * 2 * 2 state_type
 * 2 * 2 deriv_type
 * 1 copy process of dense_output_controlled_explicit:
 * 2 state_type
 * 2 deriv_type
 */
BOOST_AUTO_TEST_CASE( dense_output_dopri5_copy_construct )
{
    reset_counter();
    {
        dense_output_dopri5_type dopri5;
        dense_output_dopri5_type dopri5_2( dopri5 );
    }
    CHECK_COUNTERS( 4*1 + 3*2 + 2*2 , 4*(1+6) + 3*2 + 2*2 , 4*1 + 3*2 + 2*2 , 4*(1+6) + 3*2 + 2*2 , 3*1 + 2*2 + 1*2 , 3*(6+1) + 2*2 + 2 );
}

/*
 * Construct + Destruct
 * 6 dense_output_dopri5:
 * 6 * 1 deriv_type in explicit_error_stepper_base_fsal
 * 6 * 6 deriv_type in explicit_error_dopri5
 * 6 * 1 state_type in explicit_error_dopri5
 * 4 controlled_error_stepper (fsal):
 * 4 * 2 state_type
 * 4 * 2 deriv_type
 * 2 dense_output_controlled_explicit:
 * 2 * 2 state_type
 * 2 * 2 deriv_type
 *
 * Copying
 * 5 copy process of explicit_error_dopri5:
 * 5 * 1 deriv_type from explicit_erro_stepper_base_fsal
 * 5 * 6 deriv_type in explicit_error_dopri5
 * 5 * 1 state_type in explicit_error_dopri5
 * 3 copy process of controlled_error_stepper (fsal):
 * 3 * 2 state_type
 * 3 * 2 deriv_type
 * 1 copy process of dense_output_controlled_explicit:
 * 2 state_type
 * 2 deriv_type
 */
BOOST_AUTO_TEST_CASE( dense_output_dopri5_assign )
{
    reset_counter();
    {
        dense_output_dopri5_type dopri5;
        dense_output_dopri5_type dopri5_2;
        dopri5_2 = dopri5;
    }
    CHECK_COUNTERS( 6*1 + 4*2 + 2*2 , 6*(6+1) + 4*2 + 2*2 , 6*1 + 4*2 + 2*2 , 6*(6+1) + 4*2 + 2*2 , 5*1 + 3*2 + 2 , 5*(6+1) + 3*2 + 2 );
}


BOOST_AUTO_TEST_SUITE_END()

