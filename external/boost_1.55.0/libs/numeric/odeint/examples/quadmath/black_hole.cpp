/*
 [auto_generated]
 libs/numeric/odeint/examples/black_hole.cpp

 [begin_description]
 This example shows how the __float128 from gcc libquadmath can be used with odeint.
 [end_description]

 Copyright 2012 Lee Hodgkinson
 Copyright 2012 Karsten Ahnert
 Copyright 2012 Mario Mulansky

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <cstdlib>
#include <cmath>
#include <iostream>
#include <iterator>
#include <utility>
#include <algorithm>
#include <cassert>
#include <vector>
#include <complex>

extern "C" {
#include <quadmath.h>
}

const __float128 zero =strtoflt128 ("0.0", NULL);

namespace std {

    inline __float128 abs( __float128 x )
    {
        return fabsq( x );
    }

    inline __float128 sqrt( __float128 x )
    {
        return sqrtq( x );
    }

    inline __float128 pow( __float128 x , __float128 y )
    {
        return powq( x , y );
    }

    inline __float128 abs( std::complex< __float128 > x )
    {
        return sqrtq( x.real() * x.real() + x.imag() * x.imag() );
    }

    inline std::complex< __float128 > pow( std::complex< __float128> x , __float128 y )
    {
        __float128 r = pow( abs(x) , y );
        __float128 phi = atanq( x.imag() / x.real() );
        return std::complex< __float128 >( r * cosq( y * phi ) , r * sinq( y * phi ) );
    }
}

inline std::ostream& operator<< (std::ostream& os, const __float128& f) {

    char* y = new char[1000];
    quadmath_snprintf(y, 1000, "%.30Qg", f) ;
    os.precision(30);
    os<<y;
    delete[] y;
    return os;
}


#include <boost/array.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/numeric.hpp>
#include <boost/numeric/odeint.hpp>



using namespace boost::numeric::odeint;
using namespace std;

typedef __float128 my_float;
typedef std::vector< std::complex < my_float > > state_type;

struct radMod
{
    my_float m_om;
    my_float m_l;

    radMod( my_float om , my_float l )
        : m_om( om ) , m_l( l ) { }

    void operator()( const state_type &x , state_type &dxdt , my_float r ) const
    {

        dxdt[0] = x[1];
        dxdt[1] = -(2*(r-1)/(r*(r-2)))*x[1]-((m_om*m_om*r*r/((r-2)*(r-2)))-(m_l*(m_l+1)/(r*(r-2))))*x[0];
    }
};







int main( int argc , char **argv )
{


    state_type x(2);

    my_float re0 = strtoflt128( "-0.00008944230755601224204687038354994353820468" , NULL );
    my_float im0 = strtoflt128( "0.00004472229441850588228136889483397204368247" , NULL );
    my_float re1 = strtoflt128( "-4.464175354293244250869336196695966076150E-6 " , NULL );
    my_float im1 = strtoflt128( "-8.950483248390306670770345406051469584488E-6" , NULL );

    x[0] = complex< my_float >( re0 ,im0 );
    x[1] = complex< my_float >( re1 ,im1 );

    const my_float dt =strtoflt128 ("-0.001", NULL);
    const my_float start =strtoflt128 ("10000.0", NULL);
    const my_float end =strtoflt128 ("9990.0", NULL);
    const my_float omega =strtoflt128 ("2.0", NULL);
    const my_float ell =strtoflt128 ("1.0", NULL);



    my_float abs_err = strtoflt128( "1.0E-15" , NULL ) , rel_err = strtoflt128( "1.0E-10" , NULL );
    my_float a_x = strtoflt128( "1.0" , NULL ) , a_dxdt = strtoflt128( "1.0" , NULL );

    typedef runge_kutta_dopri5< state_type, my_float > dopri5_type;
    typedef controlled_runge_kutta< dopri5_type > controlled_dopri5_type;
    typedef dense_output_runge_kutta< controlled_dopri5_type > dense_output_dopri5_type;
    
    dense_output_dopri5_type dopri5( controlled_dopri5_type( default_error_checker< my_float >( abs_err , rel_err , a_x , a_dxdt ) ) );

    std::for_each( make_adaptive_time_iterator_begin(dopri5 , radMod(omega , ell) , x , start , end , dt) ,
                   make_adaptive_time_iterator_end(dopri5 , radMod(omega , ell) , x ) ,
                   []( const std::pair< state_type&, my_float > &x ) {
                       std::cout << x.second << ", " << x.first[0].real() << "\n"; }
        );



    return 0;
}
