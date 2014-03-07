/*
 * Solves many relaxation equations dxdt = - a * x in parallel and for different values of a.
 * The relaxation equations are completely uncoupled.
 */

#include <thrust/device_vector.h>

#include <boost/ref.hpp>

#include <boost/numeric/odeint.hpp>
#include <boost/numeric/odeint/external/thrust/thrust_algebra.hpp>
#include <boost/numeric/odeint/external/thrust/thrust_operations.hpp>
#include <boost/numeric/odeint/external/thrust/thrust_resize.hpp>


using namespace std;
using namespace boost::numeric::odeint;

// change to float if your GPU does not support doubles
typedef double value_type;
typedef thrust::device_vector< value_type > state_type;
typedef runge_kutta4< state_type , value_type , state_type , value_type , thrust_algebra , thrust_operations > stepper_type;

struct relaxation
{
    struct relaxation_functor
    {
        template< class T >
        __host__ __device__
        void operator()( T t ) const
        {
            // unpack the parameter we want to vary and the Lorenz variables
            value_type a = thrust::get< 1 >( t );
            value_type x = thrust::get< 0 >( t );
            thrust::get< 2 >( t ) = -a * x;
        }
    };

    relaxation( size_t N , const state_type &a )
    : m_N( N ) , m_a( a ) { }

    void operator()(  const state_type &x , state_type &dxdt , value_type t ) const
    {
        thrust::for_each(
            thrust::make_zip_iterator( thrust::make_tuple( x.begin() , m_a.begin() , dxdt.begin() ) ) ,
            thrust::make_zip_iterator( thrust::make_tuple( x.end() , m_a.end() , dxdt.end() ) ) ,
            relaxation_functor() );
    }

    size_t m_N;
    const state_type &m_a;
};

const size_t N = 1024 * 1024;
const value_type dt = 0.01;

int main( int arc , char* argv[] )
{
    // initialize the relaxation constants a
    vector< value_type > a_host( N );
    for( size_t i=0 ; i<N ; ++i ) a_host[i] = drand48();
    state_type a = a_host;

    // initialize the intial state x
    state_type x( N );
    thrust::fill( x.begin() , x.end() , 1.0 );

    // integrate
    relaxation relax( N , a );
    integrate_const( stepper_type() , boost::ref( relax ) , x , 0.0 , 10.0 , dt );

    return 0;
}
