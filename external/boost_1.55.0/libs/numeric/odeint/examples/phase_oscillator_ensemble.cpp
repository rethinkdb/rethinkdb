/*
 * phase_oscillator_ensemble.cpp
 *
 * Demonstrates the phase transition from an unsynchronized to an synchronized state.
 *
 * Copyright 2009-2012 Karsten Ahnert and Mario Mulansky.
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 */

#include <iostream>
#include <utility>

#include <boost/numeric/odeint.hpp>

#ifndef M_PI //not there on windows
#define M_PI 3.141592653589793 //...
#endif

#include <boost/random.hpp>

using namespace std;
using namespace boost::numeric::odeint;

//[ phase_oscillator_ensemble_system_function
typedef vector< double > container_type;


pair< double , double > calc_mean_field( const container_type &x )
{
    size_t n = x.size();
    double cos_sum = 0.0 , sin_sum = 0.0;
    for( size_t i=0 ; i<n ; ++i )
    {
        cos_sum += cos( x[i] );
        sin_sum += sin( x[i] );
    }
    cos_sum /= double( n );
    sin_sum /= double( n );

    double K = sqrt( cos_sum * cos_sum + sin_sum * sin_sum );
    double Theta = atan2( sin_sum , cos_sum );

    return make_pair( K , Theta );
}


struct phase_ensemble
{
    container_type m_omega;
    double m_epsilon;

    phase_ensemble( const size_t n , double g = 1.0 , double epsilon = 1.0 )
    : m_omega( n , 0.0 ) , m_epsilon( epsilon )
    {
        create_frequencies( g );
    }

    void create_frequencies( double g )
    {
        boost::mt19937 rng;
        boost::cauchy_distribution<> cauchy( 0.0 , g );
        boost::variate_generator< boost::mt19937&, boost::cauchy_distribution<> > gen( rng , cauchy );
        generate( m_omega.begin() , m_omega.end() , gen );
    }

    void set_epsilon( double epsilon ) { m_epsilon = epsilon; }

    double get_epsilon( void ) const { return m_epsilon; }

    void operator()( const container_type &x , container_type &dxdt , double /* t */ ) const
    {
        pair< double , double > mean = calc_mean_field( x );
        for( size_t i=0 ; i<x.size() ; ++i )
            dxdt[i] = m_omega[i] + m_epsilon * mean.first * sin( mean.second - x[i] );
    }
};
//]



//[ phase_oscillator_ensemble_observer
struct statistics_observer
{
    double m_K_mean;
    size_t m_count;

    statistics_observer( void )
    : m_K_mean( 0.0 ) , m_count( 0 ) { }

    template< class State >
    void operator()( const State &x , double t )
    {
        pair< double , double > mean = calc_mean_field( x );
        m_K_mean += mean.first;
        ++m_count;
    }

    double get_K_mean( void ) const { return ( m_count != 0 ) ? m_K_mean / double( m_count ) : 0.0 ; }

    void reset( void ) { m_K_mean = 0.0; m_count = 0; }
};
//]








int main( int argc , char **argv )
{
    //[ phase_oscillator_ensemble_integration
    const size_t n = 16384;
    const double dt = 0.1;

    container_type x( n );

    boost::mt19937 rng;
    boost::uniform_real<> unif( 0.0 , 2.0 * M_PI );
    boost::variate_generator< boost::mt19937&, boost::uniform_real<> > gen( rng , unif );

    // gamma = 1, the phase transition occurs at epsilon = 2
    phase_ensemble ensemble( n , 1.0 );
    statistics_observer obs;

    for( double epsilon = 0.0 ; epsilon < 5.0 ; epsilon += 0.1 )
    {
        ensemble.set_epsilon( epsilon );
        obs.reset();

        // start with random initial conditions
        generate( x.begin() , x.end() , gen );

        // calculate some transients steps
        integrate_const( runge_kutta4< container_type >() , boost::ref( ensemble ) , x , 0.0 , 10.0 , dt );

        // integrate and compute the statistics
        integrate_const( runge_kutta4< container_type >() , boost::ref( ensemble ) , x , 0.0 , 100.0 , dt , boost::ref( obs ) );
        cout << epsilon << "\t" << obs.get_K_mean() << endl;
    }


    //]

    return 0;
}
