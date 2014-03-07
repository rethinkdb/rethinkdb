// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2007-2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/** 
\file
    
\brief test_reduce_unit.cpp

\details
Test that reduce_unit works correctly by itself to try to isolate problems.

Output:
@verbatim
@endverbatim
**/

#include <boost/units/reduce_unit.hpp>
#include <boost/units/base_units/si/kelvin.hpp>

#include <boost/mpl/assert.hpp>
#include <boost/type_traits/is_same.hpp>

BOOST_MPL_ASSERT((boost::is_same<boost::units::reduce_unit<boost::units::si::kelvin_base_unit::unit_type>::type, boost::units::si::kelvin_base_unit::unit_type>));
