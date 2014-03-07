 /*
 * phase_oscillator_ensemble.cu
 *
 * The example how the phase_oscillator ensemble can be implemented using CUDA and thrust
 *
 *  Created on: July 15, 2011
 *      Author: karsten
 */


#include <iostream>
#include <cmath>
#include <utility>


#include <thrust/device_vector.h>
#include <thrust/reduce.h>
#include <thrust/functional.h>

#include <boost/numeric/odeint.hpp>

#include <boost/numeric/odeint/external/thrust/thrust_algebra.hpp>
#include <boost/numeric/odeint/external/thrust/thrust_operations.hpp>
#include <boost/numeric/odeint/external/thrust/thrust_resize.hpp>

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/variate_generator.hpp>


using namespace std;
using namespace boost::numeric::odeint;

//change this to float if your device does not support double computation
typedef double value_type;

//change this to host_vector< ... > of you want to run on CPU
typedef thrust::device_vector< value_type > state_type;
typedef thrust::device_vector< size_t > index_vector_type;
// typedef thrust::host_vector< value_type > state_type;
// typedef thrust::host_vector< size_t > index_vector_type;


const value_type sigma = 10.0;
const value_type b = 8.0 / 3.0;


//[ thrust_lorenz_parameters_define_simple_system
struct lorenz_system
{
    struct lorenz_functor
    {
        template< class T >
        __host__ __device__
        void operator()( T t ) const
        {
            // unpack the parameter we want to vary and the Lorenz variables
            value_type R = thrust::get< 3 >( t );
            value_type x = thrust::get< 0 >( t );
            value_type y = thrust::get< 1 >( t );
            value_type z = thrust::get< 2 >( t );
            thrust::get< 4 >( t ) = sigma * ( y - x );
            thrust::get< 5 >( t ) = R * x - y - x * z;
            thrust::get< 6 >( t ) = -b * z + x * y ;

        }
    };

    lorenz_system( size_t N , const state_type &beta )
    : m_N( N ) , m_beta( beta ) { }

    template< class State , class Deriv >
    void operator()(  const State &x , Deriv &dxdt , value_type t ) const
    {
        thrust::for_each(
                thrust::make_zip_iterator( thrust::make_tuple(
                        boost::begin( x ) ,
                        boost::begin( x ) + m_N ,
                        boost::begin( x ) + 2 * m_N ,
                        m_beta.begin() ,
                        boost::begin( dxdt ) ,
                        boost::begin( dxdt ) + m_N ,
                        boost::begin( dxdt ) + 2 * m_N  ) ) ,
                thrust::make_zip_iterator( thrust::make_tuple(
                        boost::begin( x ) + m_N ,
                        boost::begin( x ) + 2 * m_N ,
                        boost::begin( x ) + 3 * m_N ,
                        m_beta.begin() ,
                        boost::begin( dxdt ) + m_N ,
                        boost::begin( dxdt ) + 2 * m_N ,
                        boost::begin( dxdt ) + 3 * m_N  ) ) ,
                lorenz_functor() );
    }
    size_t m_N;
    const state_type &m_beta;
};
//]

struct lorenz_perturbation_system
{
    struct lorenz_perturbation_functor
    {
        template< class T >
        __host__ __device__
        void operator()( T t ) const
        {
            value_type R = thrust::get< 1 >( t );
            value_type x = thrust::get< 0 >( thrust::get< 0 >( t ) );
            value_type y = thrust::get< 1 >( thrust::get< 0 >( t ) );
            value_type z = thrust::get< 2 >( thrust::get< 0 >( t ) );
            value_type dx = thrust::get< 3 >( thrust::get< 0 >( t ) );
            value_type dy = thrust::get< 4 >( thrust::get< 0 >( t ) );
            value_type dz = thrust::get< 5 >( thrust::get< 0 >( t ) );
            thrust::get< 0 >( thrust::get< 2 >( t ) ) = sigma * ( y - x );
            thrust::get< 1 >( thrust::get< 2 >( t ) ) = R * x - y - x * z;
            thrust::get< 2 >( thrust::get< 2 >( t ) ) = -b * z + x * y ;
            thrust::get< 3 >( thrust::get< 2 >( t ) ) = sigma * ( dy - dx );
            thrust::get< 4 >( thrust::get< 2 >( t ) ) = ( R - z ) * dx - dy - x * dz;
            thrust::get< 5 >( thrust::get< 2 >( t ) ) = y * dx + x * dy - b * dz;
        }
    };

    lorenz_perturbation_system( size_t N , const state_type &beta )
    : m_N( N ) , m_beta( beta ) { }

    template< class State , class Deriv >
    void operator()(  const State &x , Deriv &dxdt , value_type t ) const
    {
        thrust::for_each(
                thrust::make_zip_iterator( thrust::make_tuple(
                        thrust::make_zip_iterator( thrust::make_tuple(
                                boost::begin( x ) ,
                                boost::begin( x ) + m_N ,
                                boost::begin( x ) + 2 * m_N ,
                                boost::begin( x ) + 3 * m_N ,
                                boost::begin( x ) + 4 * m_N ,
                                boost::begin( x ) + 5 * m_N ) ) ,
                        m_beta.begin() ,
                        thrust::make_zip_iterator( thrust::make_tuple(
                                boost::begin( dxdt ) ,
                                boost::begin( dxdt ) + m_N ,
                                boost::begin( dxdt ) + 2 * m_N ,
                                boost::begin( dxdt ) + 3 * m_N ,
                                boost::begin( dxdt ) + 4 * m_N ,
                                boost::begin( dxdt ) + 5 * m_N ) )
                ) ) ,
                thrust::make_zip_iterator( thrust::make_tuple(
                        thrust::make_zip_iterator( thrust::make_tuple(
                                boost::begin( x ) + m_N ,
                                boost::begin( x ) + 2 * m_N ,
                                boost::begin( x ) + 3 * m_N ,
                                boost::begin( x ) + 4 * m_N ,
                                boost::begin( x ) + 5 * m_N ,
                                boost::begin( x ) + 6 * m_N ) ) ,
                        m_beta.begin() ,
                        thrust::make_zip_iterator( thrust::make_tuple(
                                boost::begin( dxdt ) + m_N ,
                                boost::begin( dxdt ) + 2 * m_N ,
                                boost::begin( dxdt ) + 3 * m_N ,
                                boost::begin( dxdt ) + 4 * m_N ,
                                boost::begin( dxdt ) + 5 * m_N ,
                                boost::begin( dxdt ) + 6 * m_N  ) )
                ) ) ,
                lorenz_perturbation_functor() );
    }

    size_t m_N;
    const state_type &m_beta;
};

struct lyap_observer
{
    //[thrust_lorenz_parameters_observer_functor
    struct lyap_functor
    {
        template< class T >
        __host__ __device__
        void operator()( T t ) const
        {
            value_type &dx = thrust::get< 0 >( t );
            value_type &dy = thrust::get< 1 >( t );
            value_type &dz = thrust::get< 2 >( t );
            value_type norm = sqrt( dx * dx + dy * dy + dz * dz );
            dx /= norm;
            dy /= norm;
            dz /= norm;
            thrust::get< 3 >( t ) += log( norm );
        }
    };
    //]

    lyap_observer( size_t N , size_t every = 100 )
    : m_N( N ) , m_lyap( N ) , m_every( every ) , m_count( 0 )
    {
        thrust::fill( m_lyap.begin() , m_lyap.end() , 0.0 );
    }

    template< class Lyap >
    void fill_lyap( Lyap &lyap )
    {
        thrust::copy( m_lyap.begin() , m_lyap.end() , lyap.begin() );
        for( size_t i=0 ; i<lyap.size() ; ++i )
            lyap[i] /= m_t_overall;
    }


    template< class State >
    void operator()( State &x , value_type t )
    {
        if( ( m_count != 0 ) && ( ( m_count % m_every ) == 0 ) )
        {
            thrust::for_each(
                    thrust::make_zip_iterator( thrust::make_tuple(
                            boost::begin( x ) + 3 * m_N ,
                            boost::begin( x ) + 4 * m_N ,
                            boost::begin( x ) + 5 * m_N ,
                            m_lyap.begin() ) ) ,
                    thrust::make_zip_iterator( thrust::make_tuple(
                            boost::begin( x ) + 4 * m_N ,
                            boost::begin( x ) + 5 * m_N ,
                            boost::begin( x ) + 6 * m_N ,
                            m_lyap.end() ) ) ,
                    lyap_functor() );
            clog << t << "\n";
        }
        ++m_count;
        m_t_overall = t;
    }

    size_t m_N;
    state_type m_lyap;
    size_t m_every;
    size_t m_count;
    value_type m_t_overall;
};

const size_t N = 1024*2;
const value_type dt = 0.01;


int main( int arc , char* argv[] )
{
    int driver_version , runtime_version;
    cudaDriverGetVersion( &driver_version );
    cudaRuntimeGetVersion ( &runtime_version );
    cout << driver_version << "\t" << runtime_version << endl;


    //[ thrust_lorenz_parameters_define_beta
    vector< value_type > beta_host( N );
    const value_type beta_min = 0.0 , beta_max = 56.0;
    for( size_t i=0 ; i<N ; ++i )
        beta_host[i] = beta_min + value_type( i ) * ( beta_max - beta_min ) / value_type( N - 1 );

    state_type beta = beta_host;
    //]

    //[ thrust_lorenz_parameters_integration
    state_type x( 6 * N );

    // initialize x,y,z
    thrust::fill( x.begin() , x.begin() + 3 * N , 10.0 );

    // initial dx
    thrust::fill( x.begin() + 3 * N , x.begin() + 4 * N , 1.0 );

    // initialize dy,dz
    thrust::fill( x.begin() + 4 * N , x.end() , 0.0 );


    // create error stepper, can be used with make_controlled or make_dense_output
    typedef runge_kutta_dopri5< state_type , value_type , state_type , value_type , thrust_algebra , thrust_operations > stepper_type;


    lorenz_system lorenz( N , beta );
    lorenz_perturbation_system lorenz_perturbation( N , beta );
    lyap_observer obs( N , 1 );

    // calculate transients
    integrate_adaptive( make_controlled( 1.0e-6 , 1.0e-6 , stepper_type() ) , lorenz , std::make_pair( x.begin() , x.begin() + 3 * N ) , 0.0 , 10.0 , dt );

    // calculate the Lyapunov exponents -- the main loop
    double t = 0.0;
    while( t < 10000.0 )
    {
        integrate_adaptive( make_controlled( 1.0e-6 , 1.0e-6 , stepper_type() ) , lorenz_perturbation , x , t , t + 1.0 , 0.1 );
        t += 1.0;
        obs( x , t );
    }

    vector< value_type > lyap( N );
    obs.fill_lyap( lyap );

    for( size_t i=0 ; i<N ; ++i )
        cout << beta_host[i] << "\t" << lyap[i] << "\n";
    //]

    return 0;
}
