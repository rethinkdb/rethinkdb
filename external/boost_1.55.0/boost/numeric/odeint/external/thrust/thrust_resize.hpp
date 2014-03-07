/*
 [auto_generated]
 boost/numeric/odeint/external/thrust/thrust_resize.hpp

 [begin_description]
 Enable resizing for thrusts device and host_vector.
 [end_description]

 Copyright 2009-2011 Karsten Ahnert
 Copyright 2009-2011 Mario Mulansky

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#ifndef BOOST_NUMERIC_ODEINT_EXTERNAL_THRUST_THRUST_RESIZE_HPP_INCLUDED
#define BOOST_NUMERIC_ODEINT_EXTERNAL_THRUST_THRUST_RESIZE_HPP_INCLUDED


#include <thrust/device_vector.h>
#include <thrust/host_vector.h>

#include <boost/numeric/odeint/util/copy.hpp>

namespace boost {
namespace numeric {
namespace odeint {

template< class T >
struct is_resizeable< thrust::device_vector< T > >
{
    struct type : public boost::true_type { };
    const static bool value = type::value;
};

template< class T >
struct same_size_impl< thrust::device_vector< T > , thrust::device_vector< T > >
{
    static bool same_size( const thrust::device_vector< T > &x , const thrust::device_vector< T > &y )
    {
        return x.size() == y.size();
    }
};

template< class T >
struct resize_impl< thrust::device_vector< T > , thrust::device_vector< T > >
{
    static void resize( thrust::device_vector< T > &x , const thrust::device_vector< T > &y )
    {
        x.resize( y.size() );
    }
};


template< class T >
struct is_resizeable< thrust::host_vector< T > >
{
    struct type : public boost::true_type { };
    const static bool value = type::value;
};

template< class T >
struct same_size_impl< thrust::host_vector< T > , thrust::host_vector< T > >
{
    static bool same_size( const thrust::host_vector< T > &x , const thrust::host_vector< T > &y )
    {
        return x.size() == y.size();
    }
};

template< class T >
struct resize_impl< thrust::host_vector< T > , thrust::host_vector< T > >
{
    static void resize( thrust::host_vector< T > &x , const thrust::host_vector< T > &y )
    {
        x.resize( y.size() );
    }
};



template< class Container1, class Value >
struct copy_impl< Container1 , thrust::device_vector< Value > >
{
    static void copy( const Container1 &from , thrust::device_vector< Value > &to )
    {
        thrust::copy( boost::begin( from ) , boost::end( from ) , boost::begin( to ) );
    }
};

template< class Value , class Container2 >
struct copy_impl< thrust::device_vector< Value > , Container2 >
{
    static void copy( const thrust::device_vector< Value > &from , Container2 &to )
    {
        thrust::copy( boost::begin( from ) , boost::end( from ) , boost::begin( to ) );
    }
};

template< class Value >
struct copy_impl< thrust::device_vector< Value > , thrust::device_vector< Value > >
{
    static void copy( const thrust::device_vector< Value > &from , thrust::device_vector< Value > &to )
    {
        thrust::copy( boost::begin( from ) , boost::end( from ) , boost::begin( to ) );
    }
};




} // odeint
} // numeric
} // boost


#endif // BOOST_NUMERIC_ODEINT_EXTERNAL_THRUST_THRUST_RESIZE_HPP_INCLUDED
