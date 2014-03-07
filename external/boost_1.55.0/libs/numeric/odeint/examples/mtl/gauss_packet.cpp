/*
 * gauss_packet.cpp
 *
 * Schroedinger equation with potential barrier and periodic boundary conditions
 * Initial Gauss packet moving to the right
 *
 * pipe output into gnuplot to see animation
 *
 * Implementation of Hamilton operator via MTL library
 *
 * Copyright 2009-2012 Karsten Ahnert
 * Copyright 2009-2012 Mario Mulansky
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or
 * copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#include <iostream>
#include <complex>

#include <boost/numeric/odeint.hpp>
#include <boost/numeric/odeint/external/mtl4/mtl4_resize.hpp>

#include <boost/numeric/mtl/mtl.hpp>


using namespace std;
using namespace boost::numeric::odeint;

typedef mtl::dense_vector< complex< double > > state_type;

struct hamiltonian {

    typedef mtl::compressed2D< complex< double > > matrix_type;
    matrix_type m_H;

    hamiltonian( const int N ) : m_H( N , N )
    {
        // constructor with zero potential
        m_H = 0.0;
        initialize_kinetic_term();
    }

    //template< mtl::compressed2D< double > >
    hamiltonian( mtl::compressed2D< double > &V ) : m_H( num_rows( V ) , num_cols( V ) )
    {
        // use potential V in hamiltonian
        m_H = complex<double>( 0.0 , -1.0 ) * V;
        initialize_kinetic_term();
    }

    void initialize_kinetic_term( )
    {
        const int N = num_rows( m_H );
        mtl::matrix::inserter< matrix_type , mtl::update_plus< complex<double> > > ins( m_H );
        const double z = 1.0;
        // fill diagonal and upper and lower diagonal
        for( int i = 0 ; i<N ; ++i )
        {
            ins[ i ][ (i+1) % N ] << complex< double >( 0.0 , -z );
            ins[ i ][ i ] << complex< double >( 0.0 , z );
            ins[ (i+1) % N ][ i ] << complex< double >( 0.0 , -z );
        }
    }

    void operator()( const state_type &psi , state_type &dpsidt , const double t )
    {
        dpsidt = m_H * psi;
    }

};

struct write_for_gnuplot
{
    size_t m_every , m_count;

    write_for_gnuplot( size_t every = 10 )
    : m_every( every ) , m_count( 0 ) { }

    void operator()( const state_type &x , double t )
    {
        if( ( m_count % m_every ) == 0 )
        {
            //clog << t << endl;
            cout << "p [0:" << mtl::size(x) << "][0:0.02] '-'" << endl;
            for( size_t i=0 ; i<mtl::size(x) ; ++i )
            {
                cout << i << "\t" << norm(x[i]) << "\n";
            }
            cout << "e" << endl;
        }

        ++m_count;
    }
};

static const int N = 1024;
static const int N0 = 256;
static const double sigma0 = 20;
static const double k0 = -1.0;

int main( int argc , char** argv )
{
    state_type x( N , 0.0 );

    // initialize gauss packet with nonzero velocity
    for( int i=0 ; i<N ; ++i )
    {
        x[i] = exp( -(i-N0)*(i-N0) / ( 4.0*sigma0*sigma0 ) ) * exp( complex< double >( 0.0 , k0*i ) );
        //x[i] += 2.0*exp( -(i+N0-N)*(i+N0-N) / ( 4.0*sigma0*sigma0 ) ) * exp( complex< double >( 0.0 , -k0*i ) );
    }
    x /= mtl::two_norm( x );

    typedef runge_kutta4< state_type , double , state_type , double , vector_space_algebra > stepper;

    // create potential barrier
    mtl::compressed2D< double > V( N , N );
    V = 0.0;
    {
        mtl::matrix::inserter< mtl::compressed2D< double > > ins( V );
        for( int i=0 ; i<N ; ++i )
        {
            //ins[i][i] << 1E-4*(i-N/2)*(i-N/2);

            if( i < N/2 )
                ins[ i ][ i ] << 0.0 ;
            else
                ins[ i ][ i ] << 1.0 ;

        }
    }

    // perform integration, output can be piped to gnuplot
    integrate_const( stepper() , hamiltonian( V ) , x , 0.0 , 1000.0 , 0.1 , write_for_gnuplot( 10 ) );

    clog << "Norm: " << mtl::two_norm( x ) << endl;

    return 0;
}
