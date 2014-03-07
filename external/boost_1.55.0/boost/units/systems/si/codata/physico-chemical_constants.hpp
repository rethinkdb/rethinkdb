// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_CODATA_PHYSICO_CHEMICAL_CONSTANTS_HPP
#define BOOST_UNITS_CODATA_PHYSICO_CHEMICAL_CONSTANTS_HPP

#include <boost/units/pow.hpp>
#include <boost/units/static_constant.hpp>

#include <boost/units/systems/detail/constants.hpp>
#include <boost/units/systems/si/amount.hpp>
#include <boost/units/systems/si/area.hpp>
#include <boost/units/systems/si/electric_charge.hpp>
#include <boost/units/systems/si/energy.hpp>
#include <boost/units/systems/si/frequency.hpp>
#include <boost/units/systems/si/mass.hpp>
#include <boost/units/systems/si/power.hpp>
#include <boost/units/systems/si/solid_angle.hpp>
#include <boost/units/systems/si/temperature.hpp>

#include <boost/units/systems/si/codata/typedefs.hpp>

/// \file
/// CODATA recommended values of fundamental physico-chemical constants
/// CODATA 2006 values as of 2007/03/30

namespace boost {

namespace units { 

namespace si {
                            
namespace constants {

namespace codata {

// PHYSICO-CHEMICAL
/// Avogadro constant
BOOST_UNITS_PHYSICAL_CONSTANT(N_A,quantity<inverse_amount>,6.02214179e23/mole,3.0e16/mole);
/// atomic mass constant
BOOST_UNITS_PHYSICAL_CONSTANT(m_u,quantity<mass>,1.660538782e-27*kilograms,8.3e-35*kilograms);
/// Faraday constant
BOOST_UNITS_PHYSICAL_CONSTANT(F,quantity<electric_charge_over_amount>,96485.3399*coulombs/mole,2.4e-3*coulombs/mole);
/// molar gas constant
BOOST_UNITS_PHYSICAL_CONSTANT(R,quantity<energy_over_temperature_amount>,8.314472*joules/kelvin/mole,1.5e-5*joules/kelvin/mole);
/// Boltzmann constant
BOOST_UNITS_PHYSICAL_CONSTANT(k_B,quantity<energy_over_temperature>,1.3806504e-23*joules/kelvin,2.4e-29*joules/kelvin);
/// Stefan-Boltzmann constant
BOOST_UNITS_PHYSICAL_CONSTANT(sigma_SB,quantity<power_over_area_temperature_4>,5.670400e-8*watts/square_meter/pow<4>(kelvin),4.0e-13*watts/square_meter/pow<4>(kelvin));
/// first radiation constant
BOOST_UNITS_PHYSICAL_CONSTANT(c_1,quantity<power_area>,3.74177118e-16*watt*square_meters,1.9e-23*watt*square_meters);
/// first radiation constant for spectral radiance
BOOST_UNITS_PHYSICAL_CONSTANT(c_1L,quantity<power_area_over_solid_angle>,1.191042759e-16*watt*square_meters/steradian,5.9e-24*watt*square_meters/steradian);
/// second radiation constant
BOOST_UNITS_PHYSICAL_CONSTANT(c_2,quantity<length_temperature>,1.4387752e-2*meter*kelvin,2.5e-8*meter*kelvin);
/// Wien displacement law constant : lambda_max T
BOOST_UNITS_PHYSICAL_CONSTANT(b,quantity<length_temperature>,2.8977685e-3*meter*kelvin,5.1e-9*meter*kelvin);
/// Wien displacement law constant : nu_max/T
BOOST_UNITS_PHYSICAL_CONSTANT(b_prime,quantity<frequency_over_temperature>,5.878933e10*hertz/kelvin,1.0e15*hertz/kelvin);

} // namespace codata

} // namespace constants    

} // namespace si

} // namespace units

} // namespace boost

#endif // BOOST_UNITS_CODATA_PHYSICO_CHEMICAL_CONSTANTS_HPP
