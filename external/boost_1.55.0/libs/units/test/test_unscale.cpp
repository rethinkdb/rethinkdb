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
    
\brief test_unscale.cpp

\details
Test that unscale works in an attempt to isolate the sun problems.

Output:
@verbatim
@endverbatim
**/

#include <boost/units/reduce_unit.hpp>
#include <boost/units/detail/unscale.hpp>
#include <boost/units/base_units/temperature/fahrenheit.hpp>

#include <boost/mpl/assert.hpp>
#include <boost/type_traits/is_same.hpp>

BOOST_MPL_ASSERT((boost::is_same<boost::units::unscale<boost::units::temperature::fahrenheit_base_unit::unit_type::system_type>::type,
                  boost::units::temperature::fahrenheit_base_unit::unit_type::system_type>));

BOOST_MPL_ASSERT((boost::is_same<boost::units::unscale<boost::units::temperature::fahrenheit_base_unit::unit_type>::type,
                 boost::units::temperature::fahrenheit_base_unit::unit_type>));

BOOST_MPL_ASSERT((boost::is_same<boost::units::unscale<boost::units::reduce_unit<boost::units::temperature::fahrenheit_base_unit::unit_type>::type>::type,
                 boost::units::temperature::fahrenheit_base_unit::unit_type>));

BOOST_MPL_ASSERT((boost::is_same<
                    boost::units::temperature::fahrenheit_base_unit::unit_type,
                    boost::units::unit<
                        boost::units::temperature_dimension,
                        boost::units::heterogeneous_system<
                            boost::units::heterogeneous_system_impl<
                                boost::units::list<
                                    boost::units::heterogeneous_system_dim<
                                        boost::units::temperature::fahrenheit_base_unit,
                                        boost::units::static_rational<1>
                                    >,
                                    boost::units::dimensionless_type
                                >,
                                boost::units::temperature_dimension,
                                boost::units::dimensionless_type
                            >
                        >
                    >
>));
