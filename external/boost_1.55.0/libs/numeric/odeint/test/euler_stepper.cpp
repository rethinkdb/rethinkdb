/*
 [auto_generated]
 libs/numeric/odeint/test/euler_stepper.cpp

 [begin_description]
 This file tests explicit Euler stepper.
 [end_description]

 Copyright 2009-2012 Karsten Ahnert
 Copyright 2009-2012 Mario Mulansky

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#define BOOST_TEST_MODULE odeint_explicit_euler

#include <boost/test/unit_test.hpp>

#include <utility>
#include <iostream>
#include <vector>

#include <boost/numeric/odeint/stepper/euler.hpp>
#include <boost/numeric/odeint/algebra/range_algebra.hpp>

using namespace boost::unit_test;
using namespace boost::numeric::odeint;

// test with own vector implementation

class my_vec : public std::vector< double > {

public:

    my_vec() : std::vector< double >()
        { }

    my_vec( const my_vec &x ) : std::vector< double >( x )
        { }


    my_vec( size_t dim )
        : std::vector< double >( dim )
    { }

};

namespace boost {
namespace numeric {
namespace odeint {

template<>
struct is_resizeable< my_vec >
{
    //struct type : public boost::true_type { };
    typedef boost::true_type type;
    const static bool value = type::value;
};
} } }

typedef double value_type;
//typedef std::vector< value_type > state_type;
typedef my_vec state_type;

/* use functors, because functions don't work with msvc 10, I guess this is a bug */
struct sys
{
    void operator()( const state_type &x , state_type &dxdt , const value_type t ) const
    {
        std::cout << "sys start " << dxdt.size() << std::endl;
        dxdt[0] = x[0] + 2 * x[1];
        dxdt[1] = x[1];
        std::cout << "sys done" << std::endl;
    }
};


BOOST_AUTO_TEST_SUITE( explicit_euler_test )

BOOST_AUTO_TEST_CASE( test_euler )
{
    range_algebra algebra;
    euler< state_type > stepper( algebra );
    state_type x( 2 );
    x[0] = 0.0; x[1] = 1.0;

    std::cout << "initialized" << std::endl;

    const value_type eps = 1E-12;
    const value_type dt = 0.1;

    stepper.do_step( sys() , x , 0.0 , dt );

    using std::abs;

    // compare with analytic solution of above system
    BOOST_CHECK_MESSAGE( abs( x[0] - 2.0*1.0*dt ) < eps , x[0] - 2.0*1.0*dt );
    BOOST_CHECK_MESSAGE( abs( x[1] - (1.0 + dt) ) < eps , x[1] - (1.0+dt) );

}

BOOST_AUTO_TEST_SUITE_END()
