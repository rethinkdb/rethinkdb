/* Boost libs/numeric/odeint/examples/point_type.hpp

 Copyright 2009-2012 Karsten Ahnert
 Copyright 2009-2012 Mario Mulansky

 solar system example for Hamiltonian stepper

 Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef POINT_TYPE_HPP_INCLUDED
#define POINT_TYPE_HPP_INCLUDED


#include <boost/operators.hpp>
#include <ostream>


//[ point_type
/*the point type */
template< class T , size_t Dim >
class point :
    boost::additive1< point< T , Dim > ,
    boost::additive2< point< T , Dim  > , T ,
    boost::multiplicative2< point< T , Dim > , T
    > > >
    {
    public:

        const static size_t dim = Dim;
        typedef T value_type;
        typedef point< value_type , dim > point_type;

        // ...
        // constructors
        //<-
        point( void )
        {
            for( size_t i=0 ; i<dim ; ++i ) m_val[i] = 0.0;
        }

        point( value_type val )
        {
            for( size_t i=0 ; i<dim ; ++i ) m_val[i] = val;
        }

        point( value_type x , value_type y , value_type z = 0.0 )
        {
            if( dim > 0 ) m_val[0] = x;
            if( dim > 1 ) m_val[1] = y;
            if( dim > 2 ) m_val[2] = z;
        }
        //->

        // ...
        // operators
        //<-
        T operator[]( size_t i ) const { return m_val[i]; }
        T& operator[]( size_t i ) { return m_val[i]; }

        point_type& operator+=( const point_type& p )
        {
            for( size_t i=0 ; i<dim ; ++i )
                m_val[i] += p[i];
            return *this;
        }

        point_type& operator-=( const point_type& p )
        {
            for( size_t i=0 ; i<dim ; ++i )
                m_val[i] -= p[i];
            return *this;
        }

        point_type& operator+=( const value_type& val )
        {
            for( size_t i=0 ; i<dim ; ++i )
                m_val[i] += val;
            return *this;
        }

        point_type& operator-=( const value_type& val )
        {
            for( size_t i=0 ; i<dim ; ++i )
                m_val[i] -= val;
            return *this;
        }

        point_type& operator*=( const value_type &val )
        {
            for( size_t i=0 ; i<dim ; ++i )
                m_val[i] *= val;
            return *this;
        }

        point_type& operator/=( const value_type &val )
        {
            for( size_t i=0 ; i<dim ; ++i )
                m_val[i] /= val;
            return *this;
        }

        //->

    private:

        T m_val[dim];
    };

    //...
    // more operators
    //]

    //
    // the - operator
    //
    template< class T , size_t Dim >
    point< T , Dim > operator-( const point< T , Dim > &p )
    {
        point< T , Dim > tmp;
        for( size_t i=0 ; i<Dim ; ++i ) tmp[i] = -p[i];
        return tmp;
    }

    //
    // scalar product
    //
    template< class T , size_t Dim >
    T scalar_prod( const point< T , Dim > &p1 , const point< T , Dim > &p2 )
    {
        T tmp = 0.0;
        for( size_t i=0 ; i<Dim ; ++i ) tmp += p1[i] * p2[i];
        return tmp;
    }



    //
    // norm
    //
    template< class T , size_t Dim >
    T norm( const point< T , Dim > &p1 )
    {
        return scalar_prod( p1 , p1 );
    }




    //
    // absolute value
    //
    template< class T , size_t Dim >
    T abs( const point< T , Dim > &p1 )
    {
        return sqrt( norm( p1 ) );
    }




    //
    // output operator
    //
    template< class T , size_t Dim >
    std::ostream& operator<<( std::ostream &out , const point< T , Dim > &p )
    {
        if( Dim > 0 ) out << p[0];
        for( size_t i=1 ; i<Dim ; ++i ) out << " " << p[i];
        return out;
    }



#endif //POINT_TYPE_HPP_INCLUDED
