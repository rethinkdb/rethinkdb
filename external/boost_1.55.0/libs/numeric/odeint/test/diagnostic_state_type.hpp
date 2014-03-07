/*
  [auto_generated]
  libs/numeric/odeint/test/diagnostic_state_type.hpp

  [begin_description]
  tba.
  [end_description]

  Copyright 2009-2012 Karsten Ahnert
  Copyright 2009-2012 Mario Mulansky

  Distributed under the Boost Software License, Version 1.0.
  (See accompanying file LICENSE_1_0.txt or
  copy at http://www.boost.org/LICENSE_1_0.txt)
*/


#ifndef LIBS_NUMERIC_ODEINT_TEST_DIAGNOSTIC_STATE_TYPE_HPP_DEFINED
#define LIBS_NUMERIC_ODEINT_TEST_DIAGNOSTIC_STATE_TYPE_HPP_DEFINED

#include <boost/array.hpp>
#include <boost/numeric/odeint/util/is_resizeable.hpp>
#include <boost/numeric/odeint/util/same_size.hpp>
#include <boost/numeric/odeint/util/resize.hpp>
#include <boost/numeric/odeint/util/state_wrapper.hpp>

template< size_t N >
struct counter
{
    static size_t& adjust_size_count( void )
    {
        static size_t m_adjust_size_count;
        return m_adjust_size_count;
    }

    static size_t& construct_count( void )
    {
        static size_t m_construct_count;
        return m_construct_count;
    }

    static size_t& copy_count( void )
    {
        static size_t m_copy_count;
        return m_copy_count;
    }

    static size_t& destroy_count( void )
    {
        static size_t m_destroy_count;
        return m_destroy_count;
    }


    static void init_counter( void )
    {
        counter< N >::adjust_size_count() = 0;
        counter< N >::construct_count() = 0;
        counter< N >::copy_count() = 0;
        counter< N >::destroy_count() = 0;
    }
};

template< size_t N >
class diagnostic_type : public boost::array< double , 1 > { };


typedef diagnostic_type< 0 > diagnostic_state_type;
typedef diagnostic_type< 1 > diagnostic_deriv_type;
typedef diagnostic_type< 2 > diagnostic_state_type2;
typedef diagnostic_type< 3 > diagnostic_deriv_type2;
    
typedef counter< 0 > counter_state;
typedef counter< 1 > counter_deriv;
typedef counter< 2 > counter_state2;
typedef counter< 3 > counter_deriv2;



namespace boost {
namespace numeric {
namespace odeint {

    template< size_t N >
    struct is_resizeable< diagnostic_type< N > >
    {
        typedef boost::true_type type;
        const static bool value = type::value;
    };

    template< size_t N , size_t M  >
    struct same_size_impl< diagnostic_type< N > , diagnostic_type< M > >
    {
        static bool same_size( const diagnostic_type< N > &x1 , const diagnostic_type< M > &x2 )
        {
            return false;
        }
    };

    template< size_t N , class State1  >
    struct same_size_impl< diagnostic_type< N > , State1 >
    {
        static bool same_size( const diagnostic_type< N > &x1 , const State1 &x2 )
        {
            return false;
        }
    };

    template< class State1 , size_t N >
    struct same_size_impl< State1 , diagnostic_type< N > >
    {
        static bool same_size( const State1 &x1 , const diagnostic_type< N > &x2 )
        {
            return false;
        }
    };



    template< size_t N , class StateIn >
    struct resize_impl< diagnostic_type< N > , StateIn >
    {
        static void resize( diagnostic_type< N > &x1 , const StateIn &x2 )
        {
            counter< N >::adjust_size_count()++;
        }
    };

    template< size_t N >
    struct state_wrapper< diagnostic_type< N > >
    {
        typedef state_wrapper< diagnostic_type< N > > state_wrapper_type;
        typedef diagnostic_type< N > state_type;
        typedef double value_type;
        
        state_type m_v;

        state_wrapper() : m_v()
        {
            counter< N >::construct_count()++;
        }

        state_wrapper( const state_type &v ) : m_v( v )
        {
            counter< N >::construct_count()++;
            counter< N >::copy_count()++;
        }

        state_wrapper( const state_wrapper_type &x ) : m_v( x.m_v )
        {
            counter< N >::construct_count()++;
            counter< N >::copy_count()++;
        }

        state_wrapper_type& operator=( const state_wrapper_type &x )
        {
            counter< N >::copy_count()++;
            return *this;
        }

        ~state_wrapper()
        {
            counter< N >::destroy_count()++;
        }
    };
    

} // namespace odeint
} // namespace numeric
} // namespace boost

#define TEST_COUNTERS( c , s1 , s2 , s3 ,s4 ) \
    BOOST_CHECK_EQUAL( c::adjust_size_count() , size_t( s1 ) ); \
    BOOST_CHECK_EQUAL( c::construct_count() , size_t( s2 ) ); \
    BOOST_CHECK_EQUAL( c::copy_count() , size_t( s3 ) ); \
    BOOST_CHECK_EQUAL( c::destroy_count() , size_t( s4 ) );

#define TEST_COUNTERS_MSG( c , s1 , s2 , s3 ,s4 , msg )              \
    BOOST_CHECK_EQUAL( c::adjust_size_count() , size_t( s1 ) ); \
    BOOST_CHECK_EQUAL( c::construct_count() , size_t( s2 ) ); \
    BOOST_CHECK_EQUAL( c::copy_count() , size_t( s3 ) ); \
    BOOST_CHECK_EQUAL( c::destroy_count() , size_t( s4 ) );


#endif // LIBS_NUMERIC_ODEINT_TEST_DIAGNOSTIC_STATE_TYPE_HPP_DEFINED
