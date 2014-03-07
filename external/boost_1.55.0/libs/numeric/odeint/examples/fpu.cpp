/*
 * fpu.cpp
 *
 * This example demonstrates how one can use odeint to solve the Fermi-Pasta-Ulam system.

 *  Created on: July 13, 2011
 *
 * Copyright 2009 Karsten Ahnert and Mario Mulansky.
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 */

#include <iostream>
#include <numeric>
#include <cmath>
#include <vector>

#include <boost/numeric/odeint.hpp>

#ifndef M_PI //not there on windows
#define M_PI 3.1415927 //...
#endif

using namespace std;
using namespace boost::numeric::odeint;

//[ fpu_system_function
typedef vector< double > container_type;

struct fpu
{
    const double m_beta;

    fpu( const double beta = 1.0 ) : m_beta( beta ) { }

    // system function defining the ODE
    void operator()( const container_type &q , container_type &dpdt ) const
    {
        size_t n = q.size();
        double tmp = q[0] - 0.0;
        double tmp2 = tmp + m_beta * tmp * tmp * tmp;
        dpdt[0] = -tmp2;
        for( size_t i=0 ; i<n-1 ; ++i )
        {
            tmp = q[i+1] - q[i];
            tmp2 = tmp + m_beta * tmp * tmp * tmp;
            dpdt[i] += tmp2;
            dpdt[i+1] = -tmp2;
        }
        tmp = - q[n-1];
        tmp2 = tmp + m_beta * tmp * tmp * tmp;
        dpdt[n-1] += tmp2;
    }

    // calculates the energy of the system
    double energy( const container_type &q , const container_type &p ) const
    {
        // ...
        //<-
        double energy = 0.0;
        size_t n = q.size();

        double tmp = q[0];
        energy += 0.5 * tmp * tmp + 0.25 * m_beta * tmp * tmp * tmp * tmp;
        for( size_t i=0 ; i<n-1 ; ++i )
        {
            tmp = q[i+1] - q[i];
            energy += 0.5 * ( p[i] * p[i] + tmp * tmp ) + 0.25 * m_beta * tmp * tmp * tmp * tmp;
        }
        energy += 0.5 * p[n-1] * p[n-1];
        tmp = q[n-1];
        energy += 0.5 * tmp * tmp + 0.25 * m_beta * tmp * tmp * tmp * tmp;

        return energy;
        //->
    }

    // calculates the local energy of the system
    void local_energy( const container_type &q , const container_type &p , container_type &e ) const
    {
        // ...
        //<-
        size_t n = q.size();
        double tmp = q[0];
        double tmp2 = 0.5 * tmp * tmp + 0.25 * m_beta * tmp * tmp * tmp * tmp;
        e[0] = tmp2;
        for( size_t i=0 ; i<n-1 ; ++i )
        {
            tmp = q[i+1] - q[i];
            tmp2 = 0.25 * tmp * tmp + 0.125 * m_beta * tmp * tmp * tmp * tmp;
            e[i] += 0.5 * p[i] * p[i] + tmp2 ;
            e[i+1] = tmp2;
        }
        tmp = q[n-1];
        tmp2 = 0.5 * tmp * tmp + 0.25 * m_beta * tmp * tmp * tmp * tmp;
        e[n-1] += 0.5 * p[n-1] * p[n-1] + tmp2;
        //->
    }
};
//]



//[ fpu_observer
struct streaming_observer
{
    std::ostream& m_out;
    const fpu &m_fpu;
    size_t m_write_every;
    size_t m_count;

    streaming_observer( std::ostream &out , const fpu &f , size_t write_every = 100 )
    : m_out( out ) , m_fpu( f ) , m_write_every( write_every ) , m_count( 0 ) { }

    template< class State >
    void operator()( const State &x , double t )
    {
        if( ( m_count % m_write_every ) == 0 )
        {
            container_type &q = x.first;
            container_type &p = x.second;
            container_type energy( q.size() );
            m_fpu.local_energy( q , p , energy );
            for( size_t i=0 ; i<q.size() ; ++i )
            {
                m_out << t << "\t" << i << "\t" << q[i] << "\t" << p[i] << "\t" << energy[i] << "\n";
            }
            m_out << "\n";
            clog << t << "\t" << accumulate( energy.begin() , energy.end() , 0.0 ) << "\n";
        }
        ++m_count;
    }
};
//]








int main( int argc , char **argv )
{
    //[ fpu_integration
    const size_t n = 64;
    container_type q( n , 0.0 ) , p( n , 0.0 );

    for( size_t i=0 ; i<n ; ++i )
    {
        p[i] = 0.0;
        q[i] = 32.0 * sin( double( i + 1 ) / double( n + 1 ) * M_PI );
    }


    const double dt = 0.1;

    typedef symplectic_rkn_sb3a_mclachlan< container_type > stepper_type;
    fpu fpu_instance( 8.0 );

    integrate_const( stepper_type() , fpu_instance ,
            make_pair( boost::ref( q ) , boost::ref( p ) ) ,
            0.0 , 1000.0 , dt , streaming_observer( cout , fpu_instance , 10 ) );
    //]

    return 0;
}
