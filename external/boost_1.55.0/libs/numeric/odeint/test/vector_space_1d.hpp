/*
 [auto_generated]
 libs/numeric/odeint/test/trivial_state.cpp

 [begin_description]
 This file defines a vector_space 1d class with the appropriate operators.
 [end_description]

 Copyright 2009-2012 Karsten Ahnert
 Copyright 2009-2012 Mario Mulansky

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef VECTOR_SPACE_1D_HPP_INCLUDED
#define VECTOR_SPACE_1D_HPP_INCLUDED

#include <boost/config.hpp>
#include <boost/operators.hpp>

#include <boost/numeric/odeint/algebra/vector_space_algebra.hpp>

template< class T >
struct vector_space_1d :
    boost::additive1< vector_space_1d< T > ,
    boost::additive2< vector_space_1d< T > , T ,
    boost::multiplicative1< vector_space_1d< T > ,
    boost::multiplicative2< vector_space_1d< T > , T
    > > > >
{
    typedef T value_type;

    value_type m_x;

    vector_space_1d( void ) : m_x( 0.0 ) {}

    vector_space_1d& operator+=( const vector_space_1d& p )
    {
        m_x += p.m_x;
        return *this;
    }

    vector_space_1d& operator-=( const vector_space_1d& p )
    {
        m_x -= p.m_x;
        return *this;
    }

    vector_space_1d& operator*=( const vector_space_1d& p )
    {
        m_x *= p.m_x;
        return *this;
    }

    vector_space_1d& operator/=( const vector_space_1d& p )
    {
        m_x /= p.m_x;
        return *this;
    }

    vector_space_1d& operator+=( const value_type& val )
    {
        m_x += val;
        return *this;
    }

    vector_space_1d& operator-=( const value_type& val )
    {
        m_x -= val;
        return *this;
    }

    vector_space_1d& operator*=( const value_type &val )
    {
        m_x *= val;
        return *this;
    }

    vector_space_1d& operator/=( const value_type &val )
    {
        m_x /= val;
        return *this;
    }
};


template< class T >
vector_space_1d< T > abs( const vector_space_1d< T > &v)
{
    vector_space_1d< T > tmp;
    tmp.m_x = std::abs( v.m_x );
    return tmp;
}


template< class T >
T max BOOST_PREVENT_MACRO_SUBSTITUTION ( const vector_space_1d< T > &v )
{
    return v.m_x;
}

namespace boost {
    namespace numeric {
        namespace odeint {

            template< class T >
            struct vector_space_reduce< vector_space_1d< T > >
            {
                template< class Op >
                T operator()( const vector_space_1d< T > &v , Op op , T value )
                {
                    return v.m_x;
                }
            };

} // odeint
} // numeric
} // boost

#endif // VECTOR_SPACE_1D_HPP_INCLUDED
