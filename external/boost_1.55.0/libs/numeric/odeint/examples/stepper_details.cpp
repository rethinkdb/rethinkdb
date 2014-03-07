/*
 * stepper_details.cpp
 *
 * This example demonstrates some details about the steppers in odeint.
 *
 *
 * Copyright 2009-2012 Karsten Ahnert
 * Copyright 2009-2012 Mario Mulansky
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or
 * copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <iostream>
#include <boost/array.hpp>

#include <boost/numeric/odeint.hpp>

using namespace std;
using namespace boost::numeric::odeint;

const size_t N = 3;

typedef boost::array< double , N > state_type;

//[ system_function_structure
void sys( const state_type & /*x*/ , state_type & /*dxdt*/ , const double /*t*/ )
{
    // ...
}
//]

void sys1( const state_type &/*x*/ , state_type &/*dxdt*/ , const double /*t*/ )
{
}

void sys2( const state_type &/*x*/ , state_type &/*dxdt*/ , const double /*t*/ )
{
}


//[ symplectic_stepper_detail_system_function
typedef boost::array< double , 1 > vector_type;


struct harm_osc_f1
{
    void operator()( const vector_type &p , vector_type &dqdt )
    {
        dqdt[0] = p[0];
    }
};

struct harm_osc_f2
{
    void operator()( const vector_type &q , vector_type &dpdt )
    {
        dpdt[0] = -q[0];
    }
};
//]

//[ symplectic_stepper_detail_system_class
struct harm_osc
{
    void f1( const vector_type &p , vector_type &dqdt ) const
    {
        dqdt[0] = p[0];
    }

    void f2( const vector_type &q , vector_type &dpdt ) const
    {
        dpdt[0] = -q[0];
    }
};
//]

int main( int argc , char **argv )
{
    using namespace std;

    // Explicit stepper example
    {
        double t( 0.0 ) , dt( 0.1 );
        state_type in , out , dxdtin , inout;
        //[ explicit_stepper_detail_example
        runge_kutta4< state_type > rk;
        rk.do_step( sys1 , inout , t , dt );               // In-place transformation of inout
        rk.do_step( sys2 , inout , t , dt );               // call with different system: Ok
        rk.do_step( sys1 , in , t , out , dt );            // Out-of-place transformation
        rk.do_step( sys1 , inout , dxdtin , t , dt );      // In-place tranformation of inout
        rk.do_step( sys1 , in , dxdtin , t , out , dt );   // Out-of-place transformation
        //]
    }



    // FSAL stepper example
    {
        double t( 0.0 ) , dt( 0.1 );
        state_type in , in2 , in3 , out , dxdtin , dxdtout , inout , dxdtinout;
        //[ fsal_stepper_detail_example
        runge_kutta_dopri5< state_type > rk;
        rk.do_step( sys1 , in , t , out , dt );
        rk.do_step( sys2 , in , t , out , dt );         // DONT do this, sys1 is assumed

        rk.do_step( sys2 , in2 , t , out , dt );
        rk.do_step( sys2 , in3 , t , out , dt );        // DONT do this, in2 is assumed

        rk.do_step( sys1 , inout , dxdtinout , t , dt );
        rk.do_step( sys2 , inout , dxdtinout , t , dt );           // Ok, internal derivative is not used, dxdtinout is updated

        rk.do_step( sys1 , in , dxdtin , t , out , dxdtout , dt );
        rk.do_step( sys2 , in , dxdtin , t , out , dxdtout , dt ); // Ok, internal derivative is not used
        //]
    }


    // Symplectic harmonic oscillator example
    {
        double t( 0.0 ) , dt( 0.1 );
        //[ symplectic_stepper_detail_example
        pair< vector_type , vector_type > x;
        x.first[0] = 1.0; x.second[0] = 0.0;
        symplectic_rkn_sb3a_mclachlan< vector_type > rkn;
        rkn.do_step( make_pair( harm_osc_f1() , harm_osc_f2() ) , x , t , dt );
        //]

        //[ symplectic_stepper_detail_system_class_example
        harm_osc h;
        rkn.do_step( make_pair( boost::bind( &harm_osc::f1 , h , _1 , _2 ) , boost::bind( &harm_osc::f2 , h , _1 , _2 ) ) ,
                x , t , dt );
        //]
    }

    // Simplified harmonic oscillator example
    {
        double t = 0.0, dt = 0.1;
        //[ simplified_symplectic_stepper_example
        pair< vector_type , vector_type > x;
        x.first[0] = 1.0; x.second[0] = 0.0;
        symplectic_rkn_sb3a_mclachlan< vector_type > rkn;
        rkn.do_step( harm_osc_f1() , x , t , dt );
        //]

        vector_type q = {{ 1.0 }} , p = {{ 0.0 }};
        //[ symplectic_stepper_detail_ref_usage
        rkn.do_step( harm_osc_f1() , make_pair( boost::ref( q ) , boost::ref( p ) ) , t , dt );
        rkn.do_step( harm_osc_f1() , q , p , t , dt );
        rkn.do_step( make_pair( harm_osc_f1() , harm_osc_f2() ) , q , p , t , dt );
        //]
    }
    
    // adams_bashforth_moulton stepper example
    {
        double t = 0.0 , dt = 0.1;
        state_type inout;
        //[ multistep_detail_example
        adams_bashforth_moulton< 5 , state_type > abm;
        abm.initialize( sys , inout , t , dt );
        abm.do_step( sys , inout , t , dt );
        //]
        
        //[ multistep_detail_own_stepper_initialization
        abm.initialize( runge_kutta_fehlberg78< state_type >() , sys , inout , t , dt );
        //]
    }



    // dense output stepper examples
    {
        double t = 0.0 , dt = 0.1;
        state_type in;
        //[ dense_output_detail_example
        dense_output_runge_kutta< controlled_runge_kutta< runge_kutta_dopri5< state_type > > > dense;
        dense.initialize( in , t , dt );
        pair< double , double > times = dense.do_step( sys );
        //]

        state_type inout;
        double t_start = 0.0 , t_end = 1.0;
        //[ dense_output_detail_generation1
        typedef boost::numeric::odeint::result_of::make_dense_output<
            runge_kutta_dopri5< state_type > >::type dense_stepper_type;
        dense_stepper_type dense2 = make_dense_output( 1.0e-6 , 1.0e-6 , runge_kutta_dopri5< state_type >() );
        //]

        //[ dense_output_detail_generation2
        integrate_const( make_dense_output( 1.0e-6 , 1.0e-6 , runge_kutta_dopri5< state_type >() ) , sys , inout , t_start , t_end , dt );
        //]
    }


    return 0;
}
