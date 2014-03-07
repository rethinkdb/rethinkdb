/*
 Copyright 2009-2012 Karsten Ahnert
 Copyright 2009-2012 Mario Mulansky

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


/* example showing how odeint can be used with std::list */

#include <iostream>
#include <cmath>
#include <list>

#include <boost/numeric/odeint.hpp>

//[ list_bindings
typedef std::list< double > state_type;

namespace boost { namespace numeric { namespace odeint {

template< >
struct is_resizeable< state_type >
{ // declare resizeability
    typedef boost::true_type type;
    const static bool value = type::value;
};

template< > 
struct same_size_impl< state_type , state_type >
{ // define how to check size
    static bool same_size( const state_type &v1 ,
                           const state_type &v2 )
    {
        return v1.size() == v2.size();
    }
};

template< >
struct resize_impl< state_type , state_type >
{ // define how to resize
    static void resize( state_type &v1 ,
                        const state_type &v2 )
    {
        v1.resize( v2.size() );
    }
};

} } } 
//]

void lattice( const state_type &x , state_type &dxdt , const double /* t */ )
{
    state_type::const_iterator x_begin = x.begin();
    state_type::const_iterator x_end = x.end();
    state_type::iterator dxdt_begin = dxdt.begin();
    
    x_end--; // stop one before last
    while( x_begin != x_end )
    {
        *(dxdt_begin++) = std::sin( *(x_begin) - *(x_begin++) );
    }
    *dxdt_begin = sin( *x_begin - *(x.begin()) ); // periodic boundary
}

using namespace boost::numeric::odeint;

int main()
{
    const int N = 32;
    state_type x;
    for( int i=0 ; i<N ; ++i )
        x.push_back( 1.0*i/N );

    integrate_const( runge_kutta4< state_type >() , lattice , x , 0.0 , 10.0 , 0.1 );
}
