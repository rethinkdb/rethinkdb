/*
 * fusion_algebra.hpp
 *
 * Copyright 2009-2012 Karsten Ahnert
 * Copyright 2009-2012 Mario Mulansky
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or
 * copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#ifndef FUSION_ALGEBRA_HPP_
#define FUSION_ALGEBRA_HPP_

#include <boost/array.hpp>

#include <iostream>


template< size_t n >
struct fusion_algebra
{
    template< typename T , size_t dim >
    inline static void foreach( boost::array< T , dim > &x_tmp , const boost::array< T , dim > &x ,
            const boost::array< double , n > &a ,
            const boost::array< T , dim > k_vector[n] , const double dt )
    {
        for( size_t i=0 ; i<dim ; ++i )
        {
            x_tmp[i] = x[i];// + a[0]*dt*k_vector[0][i];
            for( size_t j = 0 ; j<n ; ++j )
                x_tmp[i] += a[j]*dt*k_vector[j][i];
        }
    }

    template< typename T , size_t dim >
    inline static void foreach( boost::array< T , dim > &x_tmp ,
                const boost::array< double , n > &a ,
                const boost::array< T , dim > k_vector[n] , const double dt )
    {
        for( size_t i=0 ; i<dim ; ++i )
        {
            x_tmp[i] = a[0]*dt*k_vector[0][i];
            for( size_t j = 1 ; j<n ; ++j )
                x_tmp[i] += a[j]*dt*k_vector[j][i];
         }
    }

};




/** hand-wise implementation for performance improvement for n = 1..4 **/

/* !!!!!!!   Actually, this is factor 3 slower with intel compiler, so we don'y use it !!!!!
 * Update: It increases performance on msvc 9.0 by about 30%, so it is activated for MSVC
 */

//#ifdef BOOST_MSVC

template<>
struct fusion_algebra< 1 >
{
    template< typename T , size_t dim >
    inline static void foreach( boost::array< T , dim > &x_tmp , const boost::array< T , dim > &x ,
            const boost::array< double , 1 > &a ,
            const boost::array< T , dim > *k_vector , const double dt )
    {
        for( size_t i=0 ; i<dim ; ++i )
        {
            x_tmp[i] = x[i]
                + a[0]*dt*k_vector[0][i];
        }
    }

};


template<>
struct fusion_algebra< 2 >
{

    template< typename T , size_t dim >
    inline static void foreach( boost::array< T , dim > &x_tmp , const boost::array< T , dim > &x ,
            const boost::array< double , 2 > &a ,
            const boost::array< T , dim > *k_vector , const double dt )
    {
        for( size_t i=0 ; i<dim ; ++i )
        {
            x_tmp[i] = x[i]
             + a[0]*dt*k_vector[0][i]
             + a[1]*dt*k_vector[1][i];
        }
    }

};


template<>
struct fusion_algebra< 3 >
{

    template< typename T , size_t dim >
    inline static void foreach( boost::array< T , dim > &x_tmp , const boost::array< T , dim > &x ,
            const boost::array< double , 3 > &a ,
            const boost::array< T , dim > *k_vector , const double dt )
    {
        for( size_t i=0 ; i<dim ; ++i )
        {
            x_tmp[i] = x[i]
             + a[0]*dt*k_vector[0][i]
             + a[1]*dt*k_vector[1][i]
             + a[2]*dt*k_vector[2][i];
        }
    }

};

template<>
struct fusion_algebra< 4 >
{

    template< typename T , size_t dim >
    inline static void foreach( boost::array< T , dim > &x_tmp , const boost::array< T , dim > &x ,
            const boost::array< double , 4 > &a ,
            const boost::array< T , dim > *k_vector , const double dt )
    {
        for( size_t i=0 ; i<dim ; ++i )
        {
            x_tmp[i] = x[i]
             + a[0]*dt*k_vector[0][i]
             + a[1]*dt*k_vector[1][i]
             + a[2]*dt*k_vector[2][i]
             + a[3]*dt*k_vector[3][i];
        }
    }

};

template<>
struct fusion_algebra< 5 >
{

    template< typename T , size_t dim >
    inline static void foreach( boost::array< T , dim > &x_tmp , const boost::array< T , dim > &x ,
            const boost::array< double , 5 > &a ,
            const boost::array< T , dim > *k_vector , const double dt )
    {
        for( size_t i=0 ; i<dim ; ++i )
        {
            x_tmp[i] = x[i]
             + a[0]*dt*k_vector[0][i]
             + a[1]*dt*k_vector[1][i]
             + a[2]*dt*k_vector[2][i]
             + a[3]*dt*k_vector[3][i]
             + a[4]*dt*k_vector[4][i];
        }
    }

};

template<>
struct fusion_algebra< 6 >
{

    template< typename T , size_t dim >
    inline static void foreach( boost::array< T , dim > &x_tmp , const boost::array< T , dim > &x ,
            const boost::array< double , 6 > &a ,
            const boost::array< T , dim > *k_vector , const double dt )
    {
        for( size_t i=0 ; i<dim ; ++i )
        {
            x_tmp[i] = x[i]
             + a[0]*dt*k_vector[0][i]
             + a[1]*dt*k_vector[1][i]
             + a[2]*dt*k_vector[2][i]
             + a[3]*dt*k_vector[3][i]
             + a[4]*dt*k_vector[4][i]
             + a[5]*dt*k_vector[5][i];
        }
    }

    template< typename T , size_t dim >
    inline static void foreach(boost::array<T , dim> &x_tmp ,
            const boost::array<double , 6> &a ,
            const boost::array<T , dim> *k_vector , const double dt)
    {
        for (size_t i = 0 ; i < dim ; ++i)
        {
            x_tmp[i] = a[0] * dt * k_vector[0][i] + a[1] * dt * k_vector[1][i]
                    + a[2] * dt * k_vector[2][i] + a[3] * dt * k_vector[3][i]
                    + a[4] * dt * k_vector[4][i] + a[5] * dt * k_vector[5][i];
        }
    }

};

//#endif /* BOOST_MSVC */

#endif /* FUSION_ALGEBRA_HPP_ */
