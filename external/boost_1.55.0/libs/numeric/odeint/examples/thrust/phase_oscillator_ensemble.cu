/*
 * phase_oscillator_ensemble.cu
 *
 * The example how the phase_oscillator ensemble can be implemented using CUDA and thrust
 *
 *  Created on: July 15, 2011
 *      Author: karsten
 */


#include <iostream>
#include <fstream>
#include <cmath>
#include <utility>

#include <thrust/device_vector.h>
#include <thrust/reduce.h>
#include <thrust/functional.h>

#include <boost/numeric/odeint.hpp>
#include <boost/numeric/odeint/external/thrust/thrust_algebra.hpp>
#include <boost/numeric/odeint/external/thrust/thrust_operations.hpp>
#include <boost/numeric/odeint/external/thrust/thrust_resize.hpp>

#include <boost/timer.hpp>
#include <boost/random/cauchy_distribution.hpp>

using namespace std;

using namespace boost::numeric::odeint;

/*
 * Sorry for that dirty hack, but nvcc has large problems with boost::random.
 *
 * Nevertheless we need the cauchy distribution from boost::random, and therefore
 * we need a generator. Here it is:
 */
struct drand48_generator
{
    typedef double result_type;
    result_type operator()( void ) const { return drand48(); }
    result_type min( void ) const { return 0.0; }
    result_type max( void ) const { return 1.0; }
};

//[ thrust_phase_ensemble_state_type
//change this to float if your device does not support double computation
typedef double value_type;

//change this to host_vector< ... > of you want to run on CPU
typedef thrust::device_vector< value_type > state_type;
// typedef thrust::host_vector< value_type > state_type;
//]


//[ thrust_phase_ensemble_mean_field_calculator
struct mean_field_calculator
{
    struct sin_functor : public thrust::unary_function< value_type , value_type >
    {
        __host__ __device__
        value_type operator()( value_type x) const
        {
            return sin( x );
        }
    };

    struct cos_functor : public thrust::unary_function< value_type , value_type >
    {
        __host__ __device__
        value_type operator()( value_type x) const
        {
            return cos( x );
        }
    };

    static std::pair< value_type , value_type > get_mean( const state_type &x )
    {
        //[ thrust_phase_ensemble_sin_sum
        value_type sin_sum = thrust::reduce(
                thrust::make_transform_iterator( x.begin() , sin_functor() ) ,
                thrust::make_transform_iterator( x.end() , sin_functor() ) );
        //]
        value_type cos_sum = thrust::reduce(
                thrust::make_transform_iterator( x.begin() , cos_functor() ) ,
                thrust::make_transform_iterator( x.end() , cos_functor() ) );

        cos_sum /= value_type( x.size() );
        sin_sum /= value_type( x.size() );

        value_type K = sqrt( cos_sum * cos_sum + sin_sum * sin_sum );
        value_type Theta = atan2( sin_sum , cos_sum );

        return std::make_pair( K , Theta );
    }
};
//]



//[ thrust_phase_ensemble_sys_function
class phase_oscillator_ensemble
{

public:

    struct sys_functor
    {
        value_type m_K , m_Theta , m_epsilon;

        sys_functor( value_type K , value_type Theta , value_type epsilon )
        : m_K( K ) , m_Theta( Theta ) , m_epsilon( epsilon ) { }

        template< class Tuple >
        __host__ __device__
        void operator()( Tuple t )
        {
            thrust::get<2>(t) = thrust::get<1>(t) + m_epsilon * m_K * sin( m_Theta - thrust::get<0>(t) );
        }
    };

    // ...
    //<-
    phase_oscillator_ensemble( size_t N , value_type g = 1.0 , value_type epsilon = 1.0 )
        : m_omega() , m_N( N ) , m_epsilon( epsilon )
    {
        create_frequencies( g );
    }

    void create_frequencies( value_type g )
    {
        boost::cauchy_distribution< value_type > cauchy( 0.0 , g );
//        boost::variate_generator< boost::mt19937&, boost::cauchy_distribution< value_type > > gen( rng , cauchy );
        drand48_generator d48;
        vector< value_type > omega( m_N );
        for( size_t i=0 ; i<m_N ; ++i )
            omega[i] = cauchy( d48 );
//        generate( omega.begin() , omega.end() , gen );
        m_omega = omega;
    }

    void set_epsilon( value_type epsilon ) { m_epsilon = epsilon; }

    value_type get_epsilon( void ) const { return m_epsilon; }
    //->

    void operator() ( const state_type &x , state_type &dxdt , const value_type dt ) const
    {
        std::pair< value_type , value_type > mean_field = mean_field_calculator::get_mean( x );

        thrust::for_each(
                thrust::make_zip_iterator( thrust::make_tuple( x.begin() , m_omega.begin() , dxdt.begin() ) ),
                thrust::make_zip_iterator( thrust::make_tuple( x.end() , m_omega.end() , dxdt.end()) ) ,
                sys_functor( mean_field.first , mean_field.second , m_epsilon )
                );
    }

    // ...
    //<-
private:

    state_type m_omega;
    const size_t m_N;
    value_type m_epsilon;
    //->
};
//]


//[ thrust_phase_ensemble_observer
struct statistics_observer
{
    value_type m_K_mean;
    size_t m_count;

    statistics_observer( void )
    : m_K_mean( 0.0 ) , m_count( 0 ) { }

    template< class State >
    void operator()( const State &x , value_type t )
    {
        std::pair< value_type , value_type > mean = mean_field_calculator::get_mean( x );
        m_K_mean += mean.first;
        ++m_count;
    }

    value_type get_K_mean( void ) const { return ( m_count != 0 ) ? m_K_mean / value_type( m_count ) : 0.0 ; }

    void reset( void ) { m_K_mean = 0.0; m_count = 0; }
};
//]



// const size_t N = 16384 * 128;
const size_t N = 16384;
const value_type pi = 3.1415926535897932384626433832795029;
const value_type dt = 0.1;
const value_type d_epsilon = 0.1;
const value_type epsilon_min = 0.0;
const value_type epsilon_max = 5.0;
const value_type t_transients = 10.0;
const value_type t_max = 100.0;

int main( int arc , char* argv[] )
{
    // initial conditions on host
    vector< value_type > x_host( N );
    for( size_t i=0 ; i<N ; ++i ) x_host[i] = 2.0 * pi * drand48();

    //[ thrust_phase_ensemble_system_instance
    phase_oscillator_ensemble ensemble( N , 1.0 );
    //]



    boost::timer timer;
    boost::timer timer_local;
    double dopri5_time = 0.0 , rk4_time = 0.0;
    {
        //[thrust_phase_ensemble_define_dopri5
        typedef runge_kutta_dopri5< state_type , value_type , state_type , value_type , thrust_algebra , thrust_operations > stepper_type;
        //]

        ofstream fout( "phase_ensemble_dopri5.dat" );
        timer.restart();
        for( value_type epsilon = epsilon_min ; epsilon < epsilon_max ; epsilon += d_epsilon )
        {
            ensemble.set_epsilon( epsilon );
            statistics_observer obs;
            state_type x = x_host;

            timer_local.restart();

            // calculate some transients steps
            //[ thrust_phase_ensemble_integration
            size_t steps1 = integrate_const( make_controlled( 1.0e-6 , 1.0e-6 , stepper_type() ) , boost::ref( ensemble ) , x , 0.0 , t_transients , dt );
            //]

            // integrate and compute the statistics
            size_t steps2 = integrate_const( make_dense_output( 1.0e-6 , 1.0e-6 , stepper_type() ) , boost::ref( ensemble ) , x , 0.0 , t_max , dt , boost::ref( obs ) );

            fout << epsilon << "\t" << obs.get_K_mean() << endl;
            cout << "Dopri5 : " << epsilon << "\t" << obs.get_K_mean() << "\t" << timer_local.elapsed() << "\t" << steps1 << "\t" << steps2 << endl;
        }
        dopri5_time = timer.elapsed();
    }



    {
        //[ thrust_phase_ensemble_define_rk4
        typedef runge_kutta4< state_type , value_type , state_type , value_type , thrust_algebra , thrust_operations > stepper_type;
        //]

        ofstream fout( "phase_ensemble_rk4.dat" );
        timer.restart();
        for( value_type epsilon = epsilon_min ; epsilon < epsilon_max ; epsilon += d_epsilon )
        {
            ensemble.set_epsilon( epsilon );
            statistics_observer obs;
            state_type x = x_host;

            timer_local.restart();

            // calculate some transients steps
            size_t steps1 = integrate_const( stepper_type() , boost::ref( ensemble ) , x , 0.0 , t_transients , dt );

            // integrate and compute the statistics
            size_t steps2 = integrate_const( stepper_type() , boost::ref( ensemble ) , x , 0.0 , t_max , dt , boost::ref( obs ) );
            fout << epsilon << "\t" << obs.get_K_mean() << endl;
            cout << "RK4     : " << epsilon << "\t" << obs.get_K_mean() << "\t" << timer_local.elapsed() << "\t" << steps1 << "\t" << steps2 << endl;
        }
        rk4_time = timer.elapsed();
    }

    cout << "Dopri 5 : " << dopri5_time << " s\n";
    cout << "RK4     : " << rk4_time << "\n";

    return 0;
}
