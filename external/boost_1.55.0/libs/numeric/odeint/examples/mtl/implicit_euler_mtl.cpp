/*
 * Copyright 2009-2012 Karsten Ahnert
 * Copyright 2009-2012 Mario Mulansky
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or
 * copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#include <iostream>
#include <fstream>
#include <utility>
#include "time.h"

#include <boost/numeric/odeint.hpp>
#include <boost/phoenix/phoenix.hpp>
#include <boost/numeric/mtl/mtl.hpp>

#include <boost/numeric/odeint/external/mtl4/implicit_euler_mtl4.hpp>

using namespace std;
using namespace boost::numeric::odeint;

namespace phoenix = boost::phoenix;



typedef mtl::dense_vector< double > vec_mtl4;
typedef mtl::compressed2D< double > mat_mtl4;

typedef boost::numeric::ublas::vector< double > vec_ublas;
typedef boost::numeric::ublas::matrix< double > mat_ublas;


// two systems defined 1 & 2 both are mostly sparse with the number of element variable
struct system1_mtl4
{

    void operator()( const vec_mtl4 &x , vec_mtl4 &dxdt , double t )
    {
        int size = mtl::size(x);

        dxdt[ 0 ] = -0.06*x[0];

        for (int i =1; i< size ; ++i){

            dxdt[ i ] = 4.2*x[i-1]-2.2*x[i]*x[i];
        }

    }
};

struct jacobi1_mtl4
{
    void operator()( const vec_mtl4 &x , mat_mtl4 &J , const double &t )
    {
        int size = mtl::size(x);
        mtl::matrix::inserter<mat_mtl4> ins(J);

        ins[0][0]=-0.06;

        for (int i =1; i< size ; ++i)
        {
            ins[i][i-1] = + 4.2;
            ins[i][i] = -4.2*x[i];
        }
    }
};



struct system1_ublas
{

    void operator()( const vec_ublas &x , vec_ublas &dxdt , double t )
    {
        int size = x.size();

        dxdt[ 0 ] = -0.06*x[0];

        for (int i =1; i< size ; ++i){

            dxdt[ i ] = 4.2*x[i-1]-2.2*x[i]*x[i];
        }

    }
};

struct jacobi1_ublas
{
    void operator()( const vec_ublas &x , mat_ublas &J , const double &t )
    {
        int size = x.size();
// mtl::matrix::inserter<mat_mtl4> ins(J);

        J(0,0)=-0.06;

        for (int i =1; i< size ; ++i){
//ins[i][0]=120.0*x[i];
            J(i,i-1) = + 4.2;
            J(i,i) = -4.2*x[i];

        }
    }
};

struct system2_mtl4
{

    void operator()( const vec_mtl4 &x , vec_mtl4 &dxdt , double t )
    {
        int size = mtl::size(x);


        for (int i =0; i< size/5 ; i+=5){

            dxdt[ i ] = -0.5*x[i];
            dxdt[i+1]= +25*x[i+1]*x[i+2]-740*x[i+3]*x[i+3]+4.2e-2*x[i];
            dxdt[i+2]= +25*x[i]*x[i]-740*x[i+3]*x[i+3];
            dxdt[i+3]= -25*x[i+1]*x[i+2]+740*x[i+3]*x[i+3];
            dxdt[i+4] = 0.250*x[i]*x[i+1]-44.5*x[i+3];

        }

    }
};

struct jacobi2_mtl4
{
    void operator()( const vec_mtl4 &x , mat_mtl4 &J , const double &t )
    {
        int size = mtl::size(x);
        mtl::matrix::inserter<mat_mtl4> ins(J);

        for (int i =0; i< size/5 ; i+=5){

            ins[ i ][i] = -0.5;
            ins[i+1][i+1]=25*x[i+2];
            ins[i+1][i+2] = 25*x[i+1];
            ins[i+1][i+3] = -740*2*x[i+3];
            ins[i+1][i] =+4.2e-2;

            ins[i+2][i]= 50*x[i];
            ins[i+2][i+3]= -740*2*x[i+3];
            ins[i+3][i+1] = -25*x[i+2];
            ins[i+3][i+2] = -25*x[i+1];
            ins[i+3][i+3] = +740*2*x[i+3];
            ins[i+4][i] = 0.25*x[i+1];
            ins[i+4][i+1] =0.25*x[i];
            ins[i+4][i+3]=-44.5;



        }
    }
};



struct system2_ublas
{

    void operator()( const vec_ublas &x , vec_ublas &dxdt , double t )
    {
        int size = x.size();
        for (int i =0; i< size/5 ; i+=5){

            dxdt[ i ] = -4.2e-2*x[i];
            dxdt[i+1]= +25*x[i+1]*x[i+2]-740*x[i+3]*x[i+3]+4.2e-2*x[i];
            dxdt[i+2]= +25*x[i]*x[i]-740*x[i+3]*x[i+3];
            dxdt[i+3]= -25*x[i+1]*x[i+2]+740*x[i+3]*x[i+3];
            dxdt[i+4] = 0.250*x[i]*x[i+1]-44.5*x[i+3];

        }

    }
};

struct jacobi2_ublas
{
    void operator()( const vec_ublas &x , mat_ublas &J , const double &t )
    {
        int size = x.size();

        for (int i =0; i< size/5 ; i+=5){

            J(i ,i) = -4.2e-2;
            J(i+1,i+1)=25*x[i+2];
            J(i+1,i+2) = 25*x[i+1];
            J(i+1,i+3) = -740*2*x[i+3];
            J(i+1,i) =+4.2e-2;

            J(i+2,i)= 50*x[i];
            J(i+2,i+3)= -740*2*x[i+3];
            J(i+3,i+1) = -25*x[i+2];
            J(i+3,i+2) = -25*x[i+1];
            J(i+3,i+3) = +740*2*x[i+3];
            J(i+4,i) = 0.25*x[i+1];
            J(i+4,i+1) =0.25*x[i];
            J(i+4,i+3)=-44.5;



        }


    }
};




void testRidiculouslyMassiveArray( int size )
{
    typedef boost::numeric::odeint::implicit_euler_mtl4 < double > mtl4stepper;
    typedef boost::numeric::odeint::implicit_euler< double > booststepper;

    vec_mtl4 x(size , 0.0);
    x[0]=1;


    double dt = 0.02;
    double endtime = 10.0;

    clock_t tstart_mtl4 = clock();
    size_t num_of_steps_mtl4 = integrate_const(
        mtl4stepper() ,
        make_pair( system1_mtl4() , jacobi1_mtl4() ) ,
        x , 0.0 , endtime , dt  );
    clock_t tend_mtl4 = clock() ;

    clog << x[0] << endl;
    clog << num_of_steps_mtl4 << " time elapsed: " << (double)(tend_mtl4-tstart_mtl4 )/CLOCKS_PER_SEC << endl;

    vec_ublas x_ublas(size , 0.0);
    x_ublas[0]=1;

    clock_t tstart_boost = clock();
    size_t num_of_steps_ublas = integrate_const(
        booststepper() ,
        make_pair( system1_ublas() , jacobi1_ublas() ) ,
        x_ublas , 0.0 , endtime , dt  );
    clock_t tend_boost = clock() ;
    
    clog << x_ublas[0] << endl;
    clog << num_of_steps_ublas << " time elapsed: " << (double)(tend_boost-tstart_boost)/CLOCKS_PER_SEC<< endl;

    clog << "dt_ublas/dt_mtl4 = " << (double)( tend_boost-tstart_boost )/( tend_mtl4-tstart_mtl4 ) << endl << endl;
    return ;
}



void testRidiculouslyMassiveArray2( int size )
{
    typedef boost::numeric::odeint::implicit_euler_mtl4 < double > mtl4stepper;
    typedef boost::numeric::odeint::implicit_euler< double > booststepper;


    vec_mtl4 x(size , 0.0);
    x[0]=100;


    double dt = 0.01;
    double endtime = 10.0;

    clock_t tstart_mtl4 = clock();
    size_t num_of_steps_mtl4 = integrate_const(
        mtl4stepper() ,
        make_pair( system1_mtl4() , jacobi1_mtl4() ) ,
        x , 0.0 , endtime , dt );


    clock_t tend_mtl4 = clock() ;
    
    clog << x[0] << endl;
    clog << num_of_steps_mtl4 << " time elapsed: " << (double)(tend_mtl4-tstart_mtl4 )/CLOCKS_PER_SEC << endl;

    vec_ublas x_ublas(size , 0.0);
    x_ublas[0]=100;

    clock_t tstart_boost = clock();
    size_t num_of_steps_ublas = integrate_const(
        booststepper() ,
        make_pair( system1_ublas() , jacobi1_ublas() ) ,
        x_ublas , 0.0 , endtime , dt  );


    clock_t tend_boost = clock() ;
    
    clog << x_ublas[0] << endl;
    clog << num_of_steps_ublas << " time elapsed: " << (double)(tend_boost-tstart_boost)/CLOCKS_PER_SEC<< endl;

    clog << "dt_ublas/dt_mtl4 = " << (double)( tend_boost-tstart_boost )/( tend_mtl4-tstart_mtl4 ) << endl << endl;
    return ;
}



 
int main( int argc , char **argv )
{
    std::vector< size_t > length;
    length.push_back( 8 );
    length.push_back( 16 );
    length.push_back( 32 );
    length.push_back( 64 );
    length.push_back( 128 );
    length.push_back( 256 );

    for( size_t i=0 ; i<length.size() ; ++i )
    {
        clog << "Testing with size " << length[i] << endl;
        testRidiculouslyMassiveArray( length[i] );
    }
    clog << endl << endl;

    for( size_t i=0 ; i<length.size() ; ++i )
    {
        clog << "Testing with size " << length[i] << endl;
        testRidiculouslyMassiveArray2( length[i] );
    }
}
