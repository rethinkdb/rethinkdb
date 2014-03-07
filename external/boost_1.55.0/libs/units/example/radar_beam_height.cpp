// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/** 
\file
    
\brief radar_beam_height.cpp

\details
Demonstrate library usage for user test cases suggested by Michael Fawcett.

Output:
@verbatim

//[radar_beam_height_output
radar range        : 300 nmi
earth radius       : 6.37101e+06 m
beam height 1      : 18169.7 m
beam height 2      : 9.81085 nmi
beam height 3      : 18169.7 m
beam height 4      : 9.81085 nmi
beam height approx : 59488.4 ft
beam height approx : 18132.1 m
//]

@endverbatim
**/

#include <iostream>

#include <boost/units/conversion.hpp>
#include <boost/units/io.hpp>
#include <boost/units/pow.hpp>
#include <boost/units/systems/si.hpp>
#include <boost/units/systems/si/prefixes.hpp>

using boost::units::length_dimension;
using boost::units::pow;
using boost::units::root;
using boost::units::quantity;
using boost::units::unit;

//[radar_beam_height_class_snippet_1
namespace nautical {

struct length_base_unit : 
    boost::units::base_unit<length_base_unit, length_dimension, 1>
{
    static std::string name()       { return "nautical mile"; }
    static std::string symbol()     { return "nmi"; }
};

typedef boost::units::make_system<length_base_unit>::type system;

/// unit typedefs
typedef unit<length_dimension,system>    length;

static const length mile,miles;

} // namespace nautical

// helper for conversions between nautical length and si length
BOOST_UNITS_DEFINE_CONVERSION_FACTOR(nautical::length_base_unit,
                                     boost::units::si::meter_base_unit,
                                     double, 1.852e3);
//]

//[radar_beam_height_class_snippet_2
namespace imperial {

struct length_base_unit : 
    boost::units::base_unit<length_base_unit, length_dimension, 2>
{
    static std::string name()       { return "foot"; }
    static std::string symbol()     { return "ft"; }
};

typedef boost::units::make_system<length_base_unit>::type system;

/// unit typedefs
typedef unit<length_dimension,system>    length;

static const length foot,feet;

} // imperial

// helper for conversions between imperial length and si length
BOOST_UNITS_DEFINE_CONVERSION_FACTOR(imperial::length_base_unit,
                                     boost::units::si::meter_base_unit,
                                     double, 1.0/3.28083989501312);
//]

// radar beam height functions
//[radar_beam_height_function_snippet_1
template<class System,typename T>
quantity<unit<boost::units::length_dimension,System>,T>
radar_beam_height(const quantity<unit<length_dimension,System>,T>& radar_range,
                  const quantity<unit<length_dimension,System>,T>& earth_radius,
                  T k = 4.0/3.0)
{
    return quantity<unit<length_dimension,System>,T>
        (pow<2>(radar_range)/(2.0*k*earth_radius));
}
//]

//[radar_beam_height_function_snippet_2
template<class return_type,class System1,class System2,typename T>
return_type
radar_beam_height(const quantity<unit<length_dimension,System1>,T>& radar_range,
                  const quantity<unit<length_dimension,System2>,T>& earth_radius,
                  T k = 4.0/3.0)
{
    // need to decide which system to use for calculation
    const return_type   rr(radar_range),
                        er(earth_radius);

    return return_type(pow<2>(rr)/(2.0*k*er));
}
//]

//[radar_beam_height_function_snippet_3
quantity<imperial::length>
radar_beam_height(const quantity<nautical::length>& range)
{
    return quantity<imperial::length>
        (pow<2>(range/(1.23*nautical::miles/root<2>(imperial::feet))));
}
//]

int main(void)
{
    using namespace boost::units;
    using namespace boost::units::si;
    using namespace nautical;

    //[radar_beam_height_snippet_1
    const quantity<nautical::length> radar_range(300.0*miles);
    const quantity<si::length>       earth_radius(6371.0087714*kilo*meters);
    
    const quantity<si::length>       beam_height_1(radar_beam_height(quantity<si::length>(radar_range),earth_radius));
    const quantity<nautical::length> beam_height_2(radar_beam_height(radar_range,quantity<nautical::length>(earth_radius)));
    const quantity<si::length>       beam_height_3(radar_beam_height< quantity<si::length> >(radar_range,earth_radius));
    const quantity<nautical::length> beam_height_4(radar_beam_height< quantity<nautical::length> >(radar_range,earth_radius));
    //]
    
    std::cout << "radar range        : " << radar_range << std::endl
              << "earth radius       : " << earth_radius << std::endl
              << "beam height 1      : " << beam_height_1 << std::endl
              << "beam height 2      : " << beam_height_2 << std::endl
              << "beam height 3      : " << beam_height_3 << std::endl
              << "beam height 4      : " << beam_height_4 << std::endl
              << "beam height approx : " << radar_beam_height(radar_range)
              << std::endl
              << "beam height approx : "
              << quantity<si::length>(radar_beam_height(radar_range))
              << std::endl << std::endl;

    return 0;
}
