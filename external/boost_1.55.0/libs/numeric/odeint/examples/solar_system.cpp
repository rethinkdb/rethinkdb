/* Boost libs/numeric/odeint/examples/solar_system.cpp

 Copyright 2009-2012 Karsten Ahnert
 Copyright 2009-2012 Mario Mulansky

 Solar system example for Hamiltonian stepper

 Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#include <iostream>
#include <boost/array.hpp>

#include <boost/numeric/odeint.hpp>

#include "point_type.hpp"

//[ container_type_definition
// we simulate 5 planets and the sun
const size_t n = 6;

typedef point< double , 3 > point_type;
typedef boost::array< point_type , n > container_type;
typedef boost::array< double , n > mass_type;
//]







//[ coordinate_function
const double gravitational_constant = 2.95912208286e-4;

struct solar_system_coor
{
    const mass_type &m_masses;

    solar_system_coor( const mass_type &masses ) : m_masses( masses ) { }

    void operator()( const container_type &p , container_type &dqdt ) const
    {
        for( size_t i=0 ; i<n ; ++i )
            dqdt[i] = p[i] / m_masses[i];
    }
};
//]


//[ momentum_function
struct solar_system_momentum
{
    const mass_type &m_masses;

    solar_system_momentum( const mass_type &masses ) : m_masses( masses ) { }

    void operator()( const container_type &q , container_type &dpdt ) const
    {
        const size_t n = q.size();
        for( size_t i=0 ; i<n ; ++i )
        {
            dpdt[i] = 0.0;
            for( size_t j=0 ; j<i ; ++j )
            {
                point_type diff = q[j] - q[i];
                double d = abs( diff );
                diff *= ( gravitational_constant * m_masses[i] * m_masses[j] / d / d / d );
                dpdt[i] += diff;
                dpdt[j] -= diff;

            }
        }
    }
};
//]







//[ some_helpers
point_type center_of_mass( const container_type &x , const mass_type &m )
{
    double overall_mass = 0.0;
    point_type mean( 0.0 );
    for( size_t i=0 ; i<x.size() ; ++i )
    {
        overall_mass += m[i];
        mean += m[i] * x[i];
    }
    if( !x.empty() ) mean /= overall_mass;
    return mean;
}


double energy( const container_type &q , const container_type &p , const mass_type &masses )
{
    const size_t n = q.size();
    double en = 0.0;
    for( size_t i=0 ; i<n ; ++i )
    {
        en += 0.5 * norm( p[i] ) / masses[i];
        for( size_t j=0 ; j<i ; ++j )
        {
            double diff = abs( q[i] - q[j] );
            en -= gravitational_constant * masses[j] * masses[i] / diff;
        }
    }
    return en;
}
//]


//[ streaming_observer
struct streaming_observer
{
    std::ostream& m_out;

    streaming_observer( std::ostream &out ) : m_out( out ) { }

    template< class State >
    void operator()( const State &x , double t ) const
    {
        container_type &q = x.first;
        m_out << t;
        for( size_t i=0 ; i<q.size() ; ++i ) m_out << "\t" << q[i];
        m_out << "\n";
    }
};
//]


int main( int argc , char **argv )
{

    using namespace std;
    using namespace boost::numeric::odeint;

    mass_type masses = {{
            1.00000597682 ,      // sun
            0.000954786104043 ,  // jupiter
            0.000285583733151 ,  // saturn
            0.0000437273164546 , // uranus
            0.0000517759138449 , // neptune
            1.0 / ( 1.3e8 )      // pluto
    }};

    container_type q = {{
            point_type( 0.0 , 0.0 , 0.0 ) ,                        // sun
            point_type( -3.5023653 , -3.8169847 , -1.5507963 ) ,   // jupiter
            point_type( 9.0755314 , -3.0458353 , -1.6483708 ) ,    // saturn
            point_type( 8.3101420 , -16.2901086 , -7.2521278 ) ,   // uranus
            point_type( 11.4707666 , -25.7294829 , -10.8169456 ) , // neptune
            point_type( -15.5387357 , -25.2225594 , -3.1902382 )   // pluto
    }};

    container_type p = {{
            point_type( 0.0 , 0.0 , 0.0 ) ,                        // sun
            point_type( 0.00565429 , -0.00412490 , -0.00190589 ) , // jupiter
            point_type( 0.00168318 , 0.00483525 , 0.00192462 ) ,   // saturn
            point_type( 0.00354178 , 0.00137102 , 0.00055029 ) ,   // uranus
            point_type( 0.00288930 , 0.00114527 , 0.00039677 ) ,   // neptune
            point_type( 0.00276725 , -0.00170702 , -0.00136504 )   // pluto
    }};

    point_type qmean = center_of_mass( q , masses );
    point_type pmean = center_of_mass( p , masses );
    for( size_t i=0 ; i<n ; ++i )
    {
        q[i] -= qmean ;
        p[i] -= pmean;
    }

    for( size_t i=0 ; i<n ; ++i ) p[i] *= masses[i];

    //[ integration_solar_system
    typedef symplectic_rkn_sb3a_mclachlan< container_type > stepper_type;
    const double dt = 100.0;

    integrate_const(
            stepper_type() ,
            make_pair( solar_system_coor( masses ) , solar_system_momentum( masses ) ) ,
            make_pair( boost::ref( q ) , boost::ref( p ) ) ,
            0.0 , 200000.0 , dt , streaming_observer( cout ) );
    //]


    return 0;
}


/*
Plot with gnuplot:
p "solar_system.dat" u 2:4 w l,"solar_system.dat" u 5:7 w l,"solar_system.dat" u 8:10 w l,"solar_system.dat" u 11:13 w l,"solar_system.dat" u 14:16 w l,"solar_system.dat" u 17:19 w l
 */
