/*
 Copyright 2009-2012 Karsten Ahnert
 Copyright 2009-2012 Mario Mulansky

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


/*
 * Example of a 2D simulation of nonlinearly coupled oscillators.
 * Program just prints final energy which should be close to the initial energy (1.0).
 * No parallelization is employed here.
 * Run time on a 2.3GHz Intel Core-i5: about 10 seconds for 100 steps.
 * Compile simply via bjam or directly:
 * g++ -O3 -I${BOOST_ROOT} -I../../../../.. spreading.cpp
 */


#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>
#include <sys/time.h>

#include <boost/ref.hpp>
#include <boost/numeric/odeint/stepper/symplectic_rkn_sb3a_mclachlan.hpp>

// we use a vector< vector< double > > as state type,
// for that some functionality has to be added for odeint to work
#include "nested_range_algebra.hpp"
#include "vector_vector_resize.hpp"

// defines the rhs of our dynamical equation
#include "lattice2d.hpp"
/* dynamical equations (Hamiltonian structure):
dqdt_{i,j} = p_{i,j}
dpdt_{i,j} = - omega_{i,j}*q_{i,j} - \beta*[ (q_{i,j} - q_{i,j-1})^3
                                            +(q_{i,j} - q_{i,j+1})^3
                                            +(q_{i,j} - q_{i-1,j})^3
                                            +(q_{i,j} - q_{i+1,j})^3 ]
*/


using namespace std;

static const int MAX_N = 1024;//2048;

static const size_t KAPPA = 2;
static const size_t LAMBDA = 4;
static const double W = 1.0;
static const double gap = 0.0;
static const size_t steps = 100;
static const double dt = 0.1;

double initial_e = 1.0;
double beta = 1.0;
int realization_index = 0;

//the state type
typedef vector< vector< double > > state_type;

//the stepper, choose a symplectic one to account for hamiltonian structure
//use nested_range_algebra for calculations on vector< vector< ... > >
typedef boost::numeric::odeint::symplectic_rkn_sb3a_mclachlan< 
    state_type , state_type , double , state_type , state_type , double , 
    nested_range_algebra< boost::numeric::odeint::range_algebra > ,
    boost::numeric::odeint::default_operations > stepper_type;

double time_diff_in_ms( timeval &t1 , timeval &t2 )
{ return (t2.tv_sec - t1.tv_sec)*1000.0 + (t2.tv_usec - t1.tv_usec)/1000.0 + 0.5; }


int main( int argc, const char* argv[] ) {

    srand( time(NULL) );

    lattice2d< KAPPA , LAMBDA > lattice( beta );


    lattice.generate_pot( W , gap , MAX_N );

    state_type q( MAX_N , vector< double >( MAX_N , 0.0 ) );

    state_type p( q );

    state_type energy( q );

    p[MAX_N/2][MAX_N/2] = sqrt( 0.5*initial_e );
    p[MAX_N/2+1][MAX_N/2] = sqrt( 0.5*initial_e );
    p[MAX_N/2][MAX_N/2+1] = sqrt( 0.5*initial_e );
    p[MAX_N/2+1][MAX_N/2+1] = sqrt( 0.5*initial_e );

    cout.precision(10);

    lattice.local_energy( q , p , energy );
    double e=0.0;
    for( size_t i=0 ; i<energy.size() ; ++i )
        for( size_t j=0 ; j<energy[i].size() ; ++j )
        {
            e += energy[i][j];
        }

    cout << "initial energy: " << lattice.energy( q , p ) << endl;

    timeval elapsed_time_start , elapsed_time_end;
    gettimeofday(&elapsed_time_start , NULL);

    stepper_type stepper;

    for( size_t step=0 ; step<=steps ; ++step )
    {
        stepper.do_step( boost::ref( lattice ) , 
                         make_pair( boost::ref( q ) , boost::ref( p ) ) , 
                         0.0 , 0.1 );
    }

    gettimeofday(&elapsed_time_end , NULL);
    double elapsed_time = time_diff_in_ms( elapsed_time_start , elapsed_time_end );
    cout << steps << " steps in " << elapsed_time/1000 << " s (energy: " << lattice.energy( q , p ) << ")" << endl;
}
