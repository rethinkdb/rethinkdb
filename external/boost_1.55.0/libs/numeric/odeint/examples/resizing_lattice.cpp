/*
 * resizing_lattice.cpp
 *
 * Demonstrates the usage of resizing of the state type during integration.
 * Examplary system is a strongly nonlinear, disordered Hamiltonian lattice
 * where the spreading of energy is investigated
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

#include <boost/ref.hpp>
#include <boost/random.hpp>

using namespace std;
using namespace boost::numeric::odeint;

//[ resizing_lattice_system_class
typedef vector< double > coord_type;
typedef pair< coord_type , coord_type > state_type;

struct compacton_lattice
{
    const int m_max_N;
    const double m_beta;
    int m_pot_start_index;
    vector< double > m_pot;

    compacton_lattice( int max_N , double beta , int pot_start_index )
        : m_max_N( max_N ) , m_beta( beta ) , m_pot_start_index( pot_start_index ) , m_pot( max_N )
    {
        srand( time( NULL ) );
        // fill random potential with iid values from [0,1]
        boost::mt19937 rng;
        boost::uniform_real<> unif( 0.0 , 1.0 );
        boost::variate_generator< boost::mt19937&, boost::uniform_real<> > gen( rng , unif );
        generate( m_pot.begin() , m_pot.end() , gen );
    }

    void operator()( const coord_type &q , coord_type &dpdt )
    {
        // calculate dpdt = -dH/dq of this hamiltonian system
        // dp_i/dt = - V_i * q_i^3 - beta*(q_i - q_{i-1})^3 + beta*(q_{i+1} - q_i)^3
        const int N = q.size();
        double diff = q[0] - q[N-1];
        for( int i=0 ; i<N ; ++i )
        {
            dpdt[i] = - m_pot[m_pot_start_index+i] * q[i]*q[i]*q[i] -
                    m_beta * diff*diff*diff;
            diff = q[(i+1) % N] - q[i];
            dpdt[i] += m_beta * diff*diff*diff;
        }
    }

    void energy_distribution( const coord_type &q , const coord_type &p , coord_type &energies )
    {
        // computes the energy per lattice site normalized by total energy
        const size_t N = q.size();
        double en = 0.0;
        for( size_t i=0 ; i<N ; i++ )
        {
            const double diff = q[(i+1) % N] - q[i];
            energies[i] = p[i]*p[i]/2.0
                + m_pot[m_pot_start_index+i]*q[i]*q[i]*q[i]*q[i]/4.0
                + m_beta/4.0 * diff*diff*diff*diff;
            en += energies[i];
        }
        en = 1.0/en;
        for( size_t i=0 ; i<N ; i++ )
        {
            energies[i] *= en;
        }
    }

    double energy( const coord_type &q , const coord_type &p )
    {
        // calculates the total energy of the excitation
        const size_t N = q.size();
        double en = 0.0;
        for( size_t i=0 ; i<N ; i++ )
        {
            const double diff = q[(i+1) % N] - q[i];
            en += p[i]*p[i]/2.0
                + m_pot[m_pot_start_index+i]*q[i]*q[i]*q[i]*q[i] / 4.0
                + m_beta/4.0 * diff*diff*diff*diff;
        }
        return en;
    }

    void change_pot_start( const int delta )
    {
        m_pot_start_index += delta;
    }
};
//]

//[ resizing_lattice_resize_function
void do_resize( coord_type &q , coord_type &p , coord_type &distr , const int N )
{
    q.resize( N );
    p.resize( N );
    distr.resize( N );
}
//]

const int max_N = 1024;
const double beta = 1.0;

int main()
{
    //[ resizing_lattice_initialize
    //start with 60 sites
    const int N_start = 60;
    coord_type q( N_start , 0.0 );
    q.reserve( max_N );
    coord_type p( N_start , 0.0 );
    p.reserve( max_N );
    // start with uniform momentum distribution over 20 sites
    fill( p.begin()+20 , p.end()-20 , 1.0/sqrt(20.0) );

    coord_type distr( N_start , 0.0 );
    distr.reserve( max_N );

    // create the system
    compacton_lattice lattice( max_N , beta , (max_N-N_start)/2 );

    //create the stepper, note that we use an always_resizer because state size might change during steps
    typedef symplectic_rkn_sb3a_mclachlan< coord_type , coord_type , double , coord_type , coord_type , double ,
            range_algebra , default_operations , always_resizer > hamiltonian_stepper;
    hamiltonian_stepper stepper;
    hamiltonian_stepper::state_type state = make_pair( q , p );
    //]

    //[ resizing_lattice_steps_loop
    double t = 0.0;
    const double dt = 0.1;
    const int steps = 10000;
    for( int step = 0 ; step < steps ; ++step )
    {
        stepper.do_step( boost::ref(lattice) , state , t , dt );
        lattice.energy_distribution( state.first , state.second , distr );
        if( distr[10] > 1E-150 )
        {
            do_resize( state.first , state.second , distr , state.first.size()+20 );
            rotate( state.first.begin() , state.first.end()-20 , state.first.end() );
            rotate( state.second.begin() , state.second.end()-20 , state.second.end() );
            lattice.change_pot_start( -20 );
            cout << t << ": resized left to " << distr.size() << ", energy = " << lattice.energy( state.first , state.second ) << endl;
        }
        if( distr[distr.size()-10] > 1E-150 )
        {
            do_resize( state.first , state.second , distr , state.first.size()+20 );
            cout << t << ": resized right to " << distr.size() << ", energy = " << lattice.energy( state.first , state.second ) << endl;
        }
        t += dt;
    }
    //]

    cout << "final lattice size: " << distr.size() << ", final energy: " << lattice.energy( state.first , state.second ) << endl;
}
