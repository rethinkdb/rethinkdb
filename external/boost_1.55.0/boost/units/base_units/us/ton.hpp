// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2007-2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNIT_BASE_UNITS_US_TON_HPP_INCLUDED
#define BOOST_UNIT_BASE_UNITS_US_TON_HPP_INCLUDED

#include <boost/units/scaled_base_unit.hpp>
#include <boost/units/static_rational.hpp>
#include <boost/units/scale.hpp>
#include <boost/units/base_units/us/pound.hpp>

namespace boost {

namespace units {

namespace us {

typedef scaled_base_unit<pound_base_unit, scale<2000, static_rational<1> > > ton_base_unit;

} // namespace us

template<>
struct base_unit_info<us::ton_base_unit> {
    static const char* name()   { return("short ton"); }
    static const char* symbol() { return("t"); }
};

} // namespace units

} // namespace boost

#endif // BOOST_UNIT_BASE_UNITS_US_TON_HPP_INCLUDED
