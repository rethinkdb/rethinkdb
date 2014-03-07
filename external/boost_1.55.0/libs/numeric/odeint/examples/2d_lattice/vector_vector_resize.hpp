/*
 Copyright 2009-2012 Karsten Ahnert
 Copyright 2009-2012 Mario Mulansky

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


/* reserved vector */

#ifndef VECTOR_VECTOR_RESIZE_HPP
#define VECTOR_VECTOR_RESIZE_HPP

#include <vector>

#include <boost/range.hpp>

namespace boost { namespace numeric { namespace odeint {

template<>
struct is_resizeable< std::vector< std::vector< double > > >
{
    typedef boost::true_type type;
    const static bool value = type::value;
};

template<>
struct same_size_impl< std::vector< std::vector< double > > , std::vector< std::vector< double > > >
{
    typedef std::vector< std::vector< double > > state_type;

    static bool same_size( const state_type &x1 ,
                           const state_type &x2 )
    {
        bool same = ( boost::size( x1 ) == boost::size( x2 ) );
        if( !same )
            return false;
        typename state_type::const_iterator begin1 = boost::begin( x1 );
        typename state_type::const_iterator begin2 = boost::begin( x2 );
        while( begin1 != boost::end( x1 ) )
            same &= ( boost::size( *begin1++ ) == boost::size( *begin2++ ) );
        return same;
    }
};

template<>
struct resize_impl< std::vector< std::vector< double > > , std::vector< std::vector< double > > >
{
    typedef std::vector< std::vector< double > > state_type;

    static void resize( state_type &x1 , const state_type &x2 )
    {
        x1.resize( boost::size( x2 ) );
        typename state_type::iterator begin1 = boost::begin( x1 );
        typename state_type::const_iterator begin2 = boost::begin( x2 );
        while( begin1 != boost::end( x1 ) )
            (*begin1++).resize( boost::size( *begin2++ ) );
    }
};

template<>
struct state_wrapper< std::vector< std::vector< double > > >
{
    typedef std::vector< std::vector< double > > state_type;
    typedef state_wrapper< state_type > state_wrapper_type;
    typedef boost::true_type is_resizeable;

    state_type m_v;

    template< class State >
    bool same_size( const State &x )
    {
        bool same = ( boost::size( m_v ) == boost::size( x ) );
        if( !same )
            return false;
        typename state_type::iterator begin1 = boost::begin( m_v );
        typename State::const_iterator begin2 = boost::begin( x );
        while( begin1 != boost::end( m_v ) )
            same &= ( boost::size( *begin1++ ) == boost::size( *begin2++ ) );
        return same;
    }

    template< class State >
    bool resize( const State &x )
    {
        if( !same_size( x ) )
        {
            m_v.resize( boost::size( x ) );
            typename state_type::iterator begin1 = boost::begin( m_v );
            typename State::const_iterator begin2 = boost::begin( x );
            while( begin1 != boost::end( m_v ) )
                (*begin1++).resize( boost::size( *begin2++ ) );

            return true;
        } else
            return false;
    }
 
};

} } }

#endif
