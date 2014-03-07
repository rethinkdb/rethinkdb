/*
 [auto_generated]
 libs/numeric/odeint/test/prepare_stepper_testing.hpp

 [begin_description]
 This file defines some helper functions for the stepper tests.
 [end_description]

 Copyright 2009-2012 Karsten Ahnert
 Copyright 2009-2012 Mario Mulansky

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef PREPARE_STEPPER_TESTING_HPP_
#define PREPARE_STEPPER_TESTING_HPP_

#include <boost/array.hpp>
#include <vector>
#include <boost/fusion/sequence.hpp>

#include <boost/mpl/vector.hpp>

#include <boost/numeric/odeint/algebra/vector_space_algebra.hpp>
#include <boost/numeric/odeint/algebra/array_algebra.hpp>

#include "vector_space_1d.hpp"

namespace mpl = boost::mpl;
namespace fusion = boost::fusion;

using namespace boost::numeric::odeint;

/* the state types that will be tested */
typedef std::vector< double > vector_type;
typedef vector_space_1d< double > vector_space_type;
typedef boost::array< double , 1 > array_type;

typedef mpl::vector< vector_type , vector_space_type , array_type >::type container_types;

/* choose the right algebra */
template< class State > struct algebra_dispatcher { typedef range_algebra type; };
template<> struct algebra_dispatcher< array_type > { typedef array_algebra type; };
template<> struct algebra_dispatcher< vector_space_type > { typedef vector_space_algebra type; };
template<> struct algebra_dispatcher< double > { typedef vector_space_algebra type; };



/*
typedef mpl::vector
<
    mpl::vector< float , boost::array< float , 1 > , boost::array< float , 1 > , constant_system_standard , stepper_type > ,
    mpl::vector< float , boost::array< float , 1 > , std::vector< float > , constant_system_standard , stepper_type >
    > types_and_systems_matrix;


struct check_stepper
{
    template< class Vector >
    void operator()( Vector )
    {
        typedef typename mpl::at_c< Vector , 0 >::type value_type;
        typedef typename mpl::at_c< Vector , 1 >::type deriv_type;
        typedef typename mpl::at_c< Vector , 2 >::type state_type;
        typedef typename mpl::at_c< Vector , 3 >::type system_type;
    }
};
*/

#endif /* PREPARE_STEPPER_TESTING_HPP_ */
