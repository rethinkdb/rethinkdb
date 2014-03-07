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
    
\brief systems.cpp

\details
Test various non-si units

Output:
@verbatim

@endverbatim
**/

#define BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(namespace_,unit_name_,dimension_)              \
namespace boost {                                                                            \
namespace units {                                                                            \
namespace namespace_ {                                                                       \
typedef make_system<unit_name_ ## _base_unit>::type    unit_name_ ## system_;                \
typedef unit<dimension_ ## _dimension,unit_name_ ## system_> unit_name_ ## _ ## dimension_;  \
static const unit_name_ ## _ ## dimension_    unit_name_ ## s;                               \
}                                                                                            \
}                                                                                            \
}                                                                                            \

#include <iostream>
#include <sstream>
#include <algorithm>

#include <boost/units/conversion.hpp>
#include <boost/units/io.hpp>
#include <boost/units/pow.hpp>

#include <boost/units/systems/cgs.hpp>
#include <boost/units/systems/si.hpp>

// angle base units
#include <boost/units/base_units/angle/arcminute.hpp>
#include <boost/units/base_units/angle/arcsecond.hpp>
#include <boost/units/base_units/angle/degree.hpp>
#include <boost/units/base_units/angle/gradian.hpp>
#include <boost/units/base_units/angle/revolution.hpp>
#include <boost/units/base_units/angle/radian.hpp>
#include <boost/units/base_units/angle/steradian.hpp>

BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(angle,arcminute,plane_angle)
BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(angle,arcsecond,plane_angle)
BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(angle,degree,plane_angle)
BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(angle,gradian,plane_angle)
BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(angle,radian,plane_angle)
BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(angle,revolution,plane_angle)

BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(angle,steradian,solid_angle)

// astronomical base units
#include <boost/units/base_units/astronomical/astronomical_unit.hpp>
#include <boost/units/base_units/astronomical/light_second.hpp>
#include <boost/units/base_units/astronomical/light_minute.hpp>
#include <boost/units/base_units/astronomical/light_hour.hpp>
#include <boost/units/base_units/astronomical/light_day.hpp>
#include <boost/units/base_units/astronomical/light_year.hpp>
#include <boost/units/base_units/astronomical/parsec.hpp>

BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(astronomical,astronomical_unit,length)
BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(astronomical,light_second,length)
BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(astronomical,light_minute,length)
BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(astronomical,light_hour,length)
BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(astronomical,light_day,length)
BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(astronomical,light_year,length)
BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(astronomical,parsec,length)

// imperial base units
#include <boost/units/base_units/imperial/thou.hpp>
#include <boost/units/base_units/imperial/inch.hpp>
#include <boost/units/base_units/imperial/foot.hpp>
#include <boost/units/base_units/imperial/yard.hpp>
#include <boost/units/base_units/imperial/furlong.hpp>
#include <boost/units/base_units/imperial/mile.hpp>
#include <boost/units/base_units/imperial/league.hpp>

#include <boost/units/base_units/imperial/grain.hpp>
#include <boost/units/base_units/imperial/drachm.hpp>
#include <boost/units/base_units/imperial/ounce.hpp>
#include <boost/units/base_units/imperial/pound.hpp>
#include <boost/units/base_units/imperial/stone.hpp>
#include <boost/units/base_units/imperial/quarter.hpp>
#include <boost/units/base_units/imperial/hundredweight.hpp>
#include <boost/units/base_units/imperial/ton.hpp>

#include <boost/units/base_units/imperial/fluid_ounce.hpp>
#include <boost/units/base_units/imperial/gill.hpp>
#include <boost/units/base_units/imperial/pint.hpp>
#include <boost/units/base_units/imperial/quart.hpp>
#include <boost/units/base_units/imperial/gallon.hpp>

#include <boost/units/base_units/imperial/conversions.hpp>

BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(imperial,thou,length)
BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(imperial,inch,length)
BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(imperial,foot,length)
BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(imperial,yard,length)
BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(imperial,furlong,length)
BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(imperial,mile,length)
BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(imperial,league,length)

BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(imperial,grain,mass)
BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(imperial,drachm,mass)
BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(imperial,ounce,mass)
BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(imperial,pound,mass)
BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(imperial,stone,mass)
BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(imperial,quarter,mass)
BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(imperial,hundredweight,mass)
BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(imperial,ton,mass)

BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(imperial,fluid_ounce,volume)
BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(imperial,gill,volume)
BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(imperial,pint,volume)
BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(imperial,quart,volume)
BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(imperial,gallon,volume)

// metric base units
#include <boost/units/base_units/metric/angstrom.hpp>
#include <boost/units/base_units/metric/fermi.hpp>
#include <boost/units/base_units/metric/micron.hpp>
#include <boost/units/base_units/metric/nautical_mile.hpp>

#include <boost/units/base_units/metric/ton.hpp>

#include <boost/units/base_units/metric/day.hpp>
#include <boost/units/base_units/metric/hour.hpp>
#include <boost/units/base_units/metric/minute.hpp>
#include <boost/units/base_units/metric/year.hpp>

#include <boost/units/base_units/metric/knot.hpp>

#include <boost/units/base_units/metric/are.hpp>
#include <boost/units/base_units/metric/barn.hpp>
#include <boost/units/base_units/metric/hectare.hpp>

#include <boost/units/base_units/metric/liter.hpp>

#include <boost/units/base_units/metric/atmosphere.hpp>
#include <boost/units/base_units/metric/bar.hpp>
#include <boost/units/base_units/metric/mmHg.hpp>
#include <boost/units/base_units/metric/torr.hpp>

BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(metric,angstrom,length)
BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(metric,fermi,length)
BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(metric,micron,length)
BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(metric,nautical_mile,length)

BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(metric,ton,mass)

BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(metric,day,time)
BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(metric,hour,time)
BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(metric,minute,time)
BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(metric,year,time)

BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(metric,knot,velocity)

BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(metric,are,area)
BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(metric,barn,area)
BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(metric,hectare,area)

BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(metric,liter,volume)

BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(metric,atmosphere,pressure)
BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(metric,bar,pressure)
BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(metric,mmHg,pressure)
BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(metric,torr,pressure)

// us base units

#include <boost/units/base_units/us/mil.hpp>
#include <boost/units/base_units/us/inch.hpp>
#include <boost/units/base_units/us/foot.hpp>
#include <boost/units/base_units/us/yard.hpp>
#include <boost/units/base_units/us/mile.hpp>

#include <boost/units/base_units/us/grain.hpp>
#include <boost/units/base_units/us/dram.hpp>
#include <boost/units/base_units/us/ounce.hpp>
#include <boost/units/base_units/us/pound.hpp>
#include <boost/units/base_units/us/hundredweight.hpp>
#include <boost/units/base_units/us/ton.hpp>

#include <boost/units/base_units/us/minim.hpp>
#include <boost/units/base_units/us/fluid_dram.hpp>
#include <boost/units/base_units/us/teaspoon.hpp>
#include <boost/units/base_units/us/tablespoon.hpp>
#include <boost/units/base_units/us/fluid_ounce.hpp>
#include <boost/units/base_units/us/gill.hpp>
#include <boost/units/base_units/us/cup.hpp>
#include <boost/units/base_units/us/pint.hpp>
#include <boost/units/base_units/us/quart.hpp>
#include <boost/units/base_units/us/gallon.hpp>

BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(us,mil,length)
BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(us,inch,length)
BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(us,foot,length)
BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(us,yard,length)
BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(us,mile,length)

BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(us,grain,mass)
BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(us,dram,mass)
BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(us,ounce,mass)
BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(us,pound,mass)
BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(us,hundredweight,mass)
BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(us,ton,mass)

BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(us,minim,volume)
BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(us,fluid_dram,volume)
BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(us,teaspoon,volume)
BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(us,tablespoon,volume)
BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(us,fluid_ounce,volume)
BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(us,gill,volume)
BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(us,cup,volume)
BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(us,pint,volume)
BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(us,quart,volume)
BOOST_UNITS_DEFINE_SINGLE_UNIT_SYSTEM(us,gallon,volume)

#ifdef __GNUC__
#include <cxxabi.h>
#endif

inline std::string demangle(const char* name)
{
    #ifdef __GNUC__
    // need to demangle C++ symbols
    char*       realname;
    std::size_t len; 
    int         stat;
     
    realname = abi::__cxa_demangle(name,NULL,&len,&stat);
    
    if (realname != NULL)
    {
        const std::string    out(realname);
        free(realname);
        
        return out;
    }
    
    return std::string("demangle :: error - unable to demangle specified symbol");
    #else
    return name;
    #endif
}

int main(void)
{
    using namespace boost::units;
    
    {
        using namespace boost::units::angle;
        
        std::cout << "Testing angle base units..." << std::endl;
        
        quantity<arcsecond_plane_angle>        as(1.0*arcseconds);
        quantity<arcminute_plane_angle>        am(1.0*arcminutes);
        quantity<degree_plane_angle>        d(1.0*degrees);
        quantity<gradian_plane_angle>        g(1.0*gradians);
        quantity<radian_plane_angle>        r(1.0*radians);
        quantity<revolution_plane_angle>    rev(1.0*revolutions);
        
        std::cout << as << " = " << quantity<si::plane_angle>(as) << std::endl
                  << am << " = " << quantity<si::plane_angle>(am) << std::endl
                  << d << " = " << quantity<si::plane_angle>(d) << std::endl
                  << g << " = " << quantity<si::plane_angle>(g) << std::endl
                  << r << " = " << quantity<si::plane_angle>(r) << std::endl
                  << rev << " = " << quantity<si::plane_angle>(rev) << std::endl
                  << std::endl;
    
        std::cout << rev << "/" << as << " = " << quantity<si::dimensionless>(rev/as) << std::endl
                  << rev << "/" << am << " = " << quantity<si::dimensionless>(rev/am) << std::endl
                  << rev << "/" << d << " = " << quantity<si::dimensionless>(rev/d) << std::endl
                  << rev << "/" << g << " = " << quantity<si::dimensionless>(rev/g) << std::endl
                  << rev << "/" << r << " = " << quantity<si::dimensionless>(rev/r) << std::endl
                  << std::endl;
                  
        // conversions only work with exponent of +/- 1 in scaled_base_unit?
        std::cout << as << " = " << quantity<arcsecond_plane_angle>(as) << std::endl
                  << am << " = " << quantity<arcsecond_plane_angle>(am) << std::endl
                  << d << " = " << quantity<arcsecond_plane_angle>(d) << std::endl
                  << rev << " = " << quantity<arcsecond_plane_angle>(rev) << std::endl
                  << std::endl;
                  
        // conversions only work with exponent of +/- 1 in scaled_base_unit? see arcsecond.hpp
        std::cout << as << " = " << quantity<arcminute_plane_angle>(as) << std::endl
                  << am << " = " << quantity<arcminute_plane_angle>(am) << std::endl
                  << d << " = " << quantity<arcminute_plane_angle>(d) << std::endl
                  << rev << " = " << quantity<arcminute_plane_angle>(rev) << std::endl
                  << std::endl;

        std::cout << as << " = " << quantity<degree_plane_angle>(as) << std::endl
                  << am << " = " << quantity<degree_plane_angle>(am) << std::endl
                  << d << " = " << quantity<degree_plane_angle>(d) << std::endl
                  << rev << " = " << quantity<degree_plane_angle>(rev) << std::endl
                  << std::endl;
                  
        std::cout << as << " = " << quantity<revolution_plane_angle>(as) << std::endl
                  << am << " = " << quantity<revolution_plane_angle>(am) << std::endl
                  << d << " = " << quantity<revolution_plane_angle>(d) << std::endl
                  << rev << " = " << quantity<revolution_plane_angle>(rev) << std::endl
                  << std::endl;
                  
        quantity<steradian_solid_angle>        sa1(1.0*steradians);
        
        std::cout << sa1 << std::endl
                  << std::endl;
    }
    
    {
        using namespace boost::units::astronomical;

        std::cout << "Testing astronomical base units..." << std::endl;
        
        quantity<light_second_length>        ls(1.0*light_seconds);
        quantity<light_minute_length>        lm(1.0*light_minutes);
        quantity<astronomical_unit_length>    au(1.0*astronomical_units);
        quantity<light_hour_length>            lh(1.0*light_hours);
        quantity<light_day_length>            ld(1.0*light_days);
        quantity<light_year_length>            ly(1.0*light_years);
        quantity<parsec_length>                ps(1.0*parsecs);
        
        std::cout << ls << " = " << quantity<si::length>(ls) << std::endl
                  << lm << " = " << quantity<si::length>(lm) << std::endl
                  << au << " = " << quantity<si::length>(au) << std::endl
                  << lh << " = " << quantity<si::length>(lh) << std::endl
                  << ld << " = " << quantity<si::length>(ld) << std::endl
                  << ly << " = " << quantity<si::length>(ly) << std::endl
                  << ps << " = " << quantity<si::length>(ps) << std::endl
                  << std::endl;
                  
        std::cout << ly << "/" << ls << " = " << quantity<si::dimensionless>(ly/ls) << std::endl
                  << ly << "/" << lm << " = " << quantity<si::dimensionless>(ly/lm) << std::endl
                  << ly << "/" << au << " = " << quantity<si::dimensionless>(ly/au) << std::endl
                  << ly << "/" << lh << " = " << quantity<si::dimensionless>(ly/ld) << std::endl
                  << ly << "/" << ld << " = " << quantity<si::dimensionless>(ly/lh) << std::endl
                  << ly << "/" << ps << " = " << quantity<si::dimensionless>(ly/ps) << std::endl
                  << std::endl;
                  
        std::cout << ls << " = " << quantity<light_second_length>(ls) << std::endl
                  << lm << " = " << quantity<light_second_length>(lm) << std::endl
                  << lh << " = " << quantity<light_second_length>(lh) << std::endl
                  << ld << " = " << quantity<light_second_length>(ld) << std::endl
                  << ly << " = " << quantity<light_second_length>(ly) << std::endl
                  << std::endl;
                  
        std::cout << ls << " = " << quantity<light_minute_length>(ls) << std::endl
                  << lm << " = " << quantity<light_minute_length>(lm) << std::endl
                  << lh << " = " << quantity<light_minute_length>(lh) << std::endl
                  << ld << " = " << quantity<light_minute_length>(ld) << std::endl
                  << ly << " = " << quantity<light_minute_length>(ly) << std::endl
                  << std::endl;
                  
        std::cout << ls << " = " << quantity<light_hour_length>(ls) << std::endl
                  << lm << " = " << quantity<light_hour_length>(lm) << std::endl
                  << lh << " = " << quantity<light_hour_length>(lh) << std::endl
                  << ld << " = " << quantity<light_hour_length>(ld) << std::endl
                  << ly << " = " << quantity<light_hour_length>(ly) << std::endl
                  << std::endl;
                  
        std::cout << ls << " = " << quantity<light_day_length>(ls) << std::endl
                  << lm << " = " << quantity<light_day_length>(lm) << std::endl
                  << lh << " = " << quantity<light_day_length>(lh) << std::endl
                  << ld << " = " << quantity<light_day_length>(ld) << std::endl
                  << ly << " = " << quantity<light_day_length>(ly) << std::endl
                  << std::endl;
                  
        std::cout << ls << " = " << quantity<light_year_length>(ls) << std::endl
                  << lm << " = " << quantity<light_year_length>(lm) << std::endl
                  << lh << " = " << quantity<light_year_length>(ld) << std::endl
                  << ld << " = " << quantity<light_year_length>(lh) << std::endl
                  << ly << " = " << quantity<light_year_length>(ly) << std::endl
                  << std::endl;
    }
    
    {
        using namespace boost::units::imperial;
        
        std::cout << "Testing imperial base units..." << std::endl;
        
        quantity<thou_length>            iml1(1.0*thous);
        quantity<inch_length>            iml2(1.0*inchs);
        quantity<foot_length>            iml3(1.0*foots);
        quantity<yard_length>            iml4(1.0*yards);
        quantity<furlong_length>        iml5(1.0*furlongs);
        quantity<mile_length>            iml6(1.0*miles);
        quantity<league_length>            iml7(1.0*leagues);
        
        std::cout << iml1 << " = " << quantity<si::length>(iml1) << std::endl
                  << iml2 << " = " << quantity<si::length>(iml2) << std::endl
                  << iml3 << " = " << quantity<si::length>(iml3) << std::endl
                  << iml4 << " = " << quantity<si::length>(iml4) << std::endl
                  << iml5 << " = " << quantity<si::length>(iml5) << std::endl
                  << iml6 << " = " << quantity<si::length>(iml6) << std::endl
                  << iml7 << " = " << quantity<si::length>(iml7) << std::endl
                  << std::endl;

        std::cout << iml7 << "/" << iml1 << " = " << quantity<si::dimensionless>(iml7/iml1) << std::endl
                  << iml7 << "/" << iml2 << " = " << quantity<si::dimensionless>(iml7/iml2) << std::endl
                  << iml7 << "/" << iml3 << " = " << quantity<si::dimensionless>(iml7/iml3) << std::endl
                  << iml7 << "/" << iml4 << " = " << quantity<si::dimensionless>(iml7/iml4) << std::endl
                  << iml7 << "/" << iml5 << " = " << quantity<si::dimensionless>(iml7/iml5) << std::endl
                  << iml7 << "/" << iml6 << " = " << quantity<si::dimensionless>(iml7/iml6) << std::endl
                  << std::endl;
                  
        std::cout << iml1 << " = " << quantity<thou_length>(iml1) << std::endl
                  << iml2 << " = " << quantity<thou_length>(iml2) << std::endl
                  << iml3 << " = " << quantity<thou_length>(iml3) << std::endl
                  << iml4 << " = " << quantity<thou_length>(iml4) << std::endl
                  << iml5 << " = " << quantity<thou_length>(iml5) << std::endl
                  << iml6 << " = " << quantity<thou_length>(iml6) << std::endl
                  << iml7 << " = " << quantity<thou_length>(iml7) << std::endl
                  << std::endl;

        std::cout << iml1 << " = " << quantity<inch_length>(iml1) << std::endl
                  << iml2 << " = " << quantity<inch_length>(iml2) << std::endl
                  << iml3 << " = " << quantity<inch_length>(iml3) << std::endl
                  << iml4 << " = " << quantity<inch_length>(iml4) << std::endl
                  << iml5 << " = " << quantity<inch_length>(iml5) << std::endl
                  << iml6 << " = " << quantity<inch_length>(iml6) << std::endl
                  << iml7 << " = " << quantity<inch_length>(iml7) << std::endl
                  << std::endl;

        std::cout << iml1 << " = " << quantity<foot_length>(iml1) << std::endl
                  << iml2 << " = " << quantity<foot_length>(iml2) << std::endl
                  << iml3 << " = " << quantity<foot_length>(iml3) << std::endl
                  << iml4 << " = " << quantity<foot_length>(iml4) << std::endl
                  << iml5 << " = " << quantity<foot_length>(iml5) << std::endl
                  << iml6 << " = " << quantity<foot_length>(iml6) << std::endl
                  << iml7 << " = " << quantity<foot_length>(iml7) << std::endl
                  << std::endl;

        std::cout << iml1 << " = " << quantity<yard_length>(iml1) << std::endl
                  << iml2 << " = " << quantity<yard_length>(iml2) << std::endl
                  << iml3 << " = " << quantity<yard_length>(iml3) << std::endl
                  << iml4 << " = " << quantity<yard_length>(iml4) << std::endl
                  << iml5 << " = " << quantity<yard_length>(iml5) << std::endl
                  << iml6 << " = " << quantity<yard_length>(iml6) << std::endl
                  << iml7 << " = " << quantity<yard_length>(iml7) << std::endl
                  << std::endl;

        std::cout << iml1 << " = " << quantity<furlong_length>(iml1) << std::endl
                  << iml2 << " = " << quantity<furlong_length>(iml2) << std::endl
                  << iml3 << " = " << quantity<furlong_length>(iml3) << std::endl
                  << iml4 << " = " << quantity<furlong_length>(iml4) << std::endl
                  << iml5 << " = " << quantity<furlong_length>(iml5) << std::endl
                  << iml6 << " = " << quantity<furlong_length>(iml6) << std::endl
                  << iml7 << " = " << quantity<furlong_length>(iml7) << std::endl
                  << std::endl;

        std::cout << iml1 << " = " << quantity<mile_length>(iml1) << std::endl
                  << iml2 << " = " << quantity<mile_length>(iml2) << std::endl
                  << iml3 << " = " << quantity<mile_length>(iml3) << std::endl
                  << iml4 << " = " << quantity<mile_length>(iml4) << std::endl
                  << iml5 << " = " << quantity<mile_length>(iml5) << std::endl
                  << iml6 << " = " << quantity<mile_length>(iml6) << std::endl
                  << iml7 << " = " << quantity<mile_length>(iml7) << std::endl
                  << std::endl;

        std::cout << iml1 << " = " << quantity<league_length>(iml1) << std::endl
                  << iml2 << " = " << quantity<league_length>(iml2) << std::endl
                  << iml3 << " = " << quantity<league_length>(iml3) << std::endl
                  << iml4 << " = " << quantity<league_length>(iml4) << std::endl
                  << iml5 << " = " << quantity<league_length>(iml5) << std::endl
                  << iml6 << " = " << quantity<league_length>(iml6) << std::endl
                  << iml7 << " = " << quantity<league_length>(iml7) << std::endl
                  << std::endl;
                  
        quantity<grain_mass>            imm1(1.0*grains);
        quantity<drachm_mass>            imm2(1.0*drachms);
        quantity<ounce_mass>            imm3(1.0*ounces);
        quantity<pound_mass>            imm4(1.0*pounds);
        quantity<stone_mass>            imm5(1.0*stones);
        quantity<quarter_mass>            imm6(1.0*quarters);
        quantity<hundredweight_mass>    imm7(1.0*hundredweights);
        quantity<ton_mass>                imm8(1.0*tons);

        std::cout << imm1 << " = " << quantity<si::mass>(imm1) << std::endl
                  << imm2 << " = " << quantity<si::mass>(imm2) << std::endl
                  << imm3 << " = " << quantity<si::mass>(imm3) << std::endl
                  << imm4 << " = " << quantity<si::mass>(imm4) << std::endl
                  << imm5 << " = " << quantity<si::mass>(imm5) << std::endl
                  << imm6 << " = " << quantity<si::mass>(imm6) << std::endl
                  << imm7 << " = " << quantity<si::mass>(imm7) << std::endl
                  << imm8 << " = " << quantity<si::mass>(imm8) << std::endl
                  << std::endl;
                  
        std::cout << imm8 << "/" << imm1 << " = " << quantity<si::dimensionless>(imm8/imm1) << std::endl
                  << imm8 << "/" << imm2 << " = " << quantity<si::dimensionless>(imm8/imm2) << std::endl
                  << imm8 << "/" << imm3 << " = " << quantity<si::dimensionless>(imm8/imm3) << std::endl
                  << imm8 << "/" << imm4 << " = " << quantity<si::dimensionless>(imm8/imm4) << std::endl
                  << imm8 << "/" << imm5 << " = " << quantity<si::dimensionless>(imm8/imm5) << std::endl
                  << imm8 << "/" << imm6 << " = " << quantity<si::dimensionless>(imm8/imm6) << std::endl
                  << imm8 << "/" << imm7 << " = " << quantity<si::dimensionless>(imm8/imm7) << std::endl
                  << std::endl;

        std::cout << imm1 << " = " << quantity<grain_mass>(imm1) << std::endl
                  << imm2 << " = " << quantity<grain_mass>(imm2) << std::endl
                  << imm3 << " = " << quantity<grain_mass>(imm3) << std::endl
                  << imm4 << " = " << quantity<grain_mass>(imm4) << std::endl
                  << imm5 << " = " << quantity<grain_mass>(imm5) << std::endl
                  << imm6 << " = " << quantity<grain_mass>(imm6) << std::endl
                  << imm7 << " = " << quantity<grain_mass>(imm7) << std::endl
                  << imm8 << " = " << quantity<grain_mass>(imm8) << std::endl
                  << std::endl;
                  
        std::cout << imm1 << " = " << quantity<drachm_mass>(imm1) << std::endl
                  << imm2 << " = " << quantity<drachm_mass>(imm2) << std::endl
                  << imm3 << " = " << quantity<drachm_mass>(imm3) << std::endl
                  << imm4 << " = " << quantity<drachm_mass>(imm4) << std::endl
                  << imm5 << " = " << quantity<drachm_mass>(imm5) << std::endl
                  << imm6 << " = " << quantity<drachm_mass>(imm6) << std::endl
                  << imm7 << " = " << quantity<drachm_mass>(imm7) << std::endl
                  << imm8 << " = " << quantity<drachm_mass>(imm8) << std::endl
                  << std::endl;
                  
        std::cout << imm1 << " = " << quantity<ounce_mass>(imm1) << std::endl
                  << imm2 << " = " << quantity<ounce_mass>(imm2) << std::endl
                  << imm3 << " = " << quantity<ounce_mass>(imm3) << std::endl
                  << imm4 << " = " << quantity<ounce_mass>(imm4) << std::endl
                  << imm5 << " = " << quantity<ounce_mass>(imm5) << std::endl
                  << imm6 << " = " << quantity<ounce_mass>(imm6) << std::endl
                  << imm7 << " = " << quantity<ounce_mass>(imm7) << std::endl
                  << imm8 << " = " << quantity<ounce_mass>(imm8) << std::endl
                  << std::endl;
                  
        std::cout << imm1 << " = " << quantity<pound_mass>(imm1) << std::endl
                  << imm2 << " = " << quantity<pound_mass>(imm2) << std::endl
                  << imm3 << " = " << quantity<pound_mass>(imm3) << std::endl
                  << imm4 << " = " << quantity<pound_mass>(imm4) << std::endl
                  << imm5 << " = " << quantity<pound_mass>(imm5) << std::endl
                  << imm6 << " = " << quantity<pound_mass>(imm6) << std::endl
                  << imm7 << " = " << quantity<pound_mass>(imm7) << std::endl
                  << imm8 << " = " << quantity<pound_mass>(imm8) << std::endl
                  << std::endl;
                  
        std::cout << imm1 << " = " << quantity<stone_mass>(imm1) << std::endl
                  << imm2 << " = " << quantity<stone_mass>(imm2) << std::endl
                  << imm3 << " = " << quantity<stone_mass>(imm3) << std::endl
                  << imm4 << " = " << quantity<stone_mass>(imm4) << std::endl
                  << imm5 << " = " << quantity<stone_mass>(imm5) << std::endl
                  << imm6 << " = " << quantity<stone_mass>(imm6) << std::endl
                  << imm7 << " = " << quantity<stone_mass>(imm7) << std::endl
                  << imm8 << " = " << quantity<stone_mass>(imm8) << std::endl
                  << std::endl;
                  
        std::cout << imm1 << " = " << quantity<quarter_mass>(imm1) << std::endl
                  << imm2 << " = " << quantity<quarter_mass>(imm2) << std::endl
                  << imm3 << " = " << quantity<quarter_mass>(imm3) << std::endl
                  << imm4 << " = " << quantity<quarter_mass>(imm4) << std::endl
                  << imm5 << " = " << quantity<quarter_mass>(imm5) << std::endl
                  << imm6 << " = " << quantity<quarter_mass>(imm6) << std::endl
                  << imm7 << " = " << quantity<quarter_mass>(imm7) << std::endl
                  << imm8 << " = " << quantity<quarter_mass>(imm8) << std::endl
                  << std::endl;
                  
        std::cout << imm1 << " = " << quantity<hundredweight_mass>(imm1) << std::endl
                  << imm2 << " = " << quantity<hundredweight_mass>(imm2) << std::endl
                  << imm3 << " = " << quantity<hundredweight_mass>(imm3) << std::endl
                  << imm4 << " = " << quantity<hundredweight_mass>(imm4) << std::endl
                  << imm5 << " = " << quantity<hundredweight_mass>(imm5) << std::endl
                  << imm6 << " = " << quantity<hundredweight_mass>(imm6) << std::endl
                  << imm7 << " = " << quantity<hundredweight_mass>(imm7) << std::endl
                  << imm8 << " = " << quantity<hundredweight_mass>(imm8) << std::endl
                  << std::endl;
                  
        std::cout << imm1 << " = " << quantity<ton_mass>(imm1) << std::endl
                  << imm2 << " = " << quantity<ton_mass>(imm2) << std::endl
                  << imm3 << " = " << quantity<ton_mass>(imm3) << std::endl
                  << imm4 << " = " << quantity<ton_mass>(imm4) << std::endl
                  << imm5 << " = " << quantity<ton_mass>(imm5) << std::endl
                  << imm6 << " = " << quantity<ton_mass>(imm6) << std::endl
                  << imm7 << " = " << quantity<ton_mass>(imm7) << std::endl
                  << imm8 << " = " << quantity<ton_mass>(imm8) << std::endl
                  << std::endl;
                  
        quantity<fluid_ounce_volume>    imv1(1.0*fluid_ounces);
        quantity<gill_volume>            imv2(1.0*gills);
        quantity<pint_volume>            imv3(1.0*pints);
        quantity<quart_volume>            imv4(1.0*quarts);
        quantity<gallon_volume>            imv5(1.0*gallons);
        
        std::cout << imv1 << " = " << quantity<si::volume>(imv1) << std::endl
                  << imv2 << " = " << quantity<si::volume>(imv2) << std::endl
                  << imv3 << " = " << quantity<si::volume>(imv3) << std::endl
                  << imv4 << " = " << quantity<si::volume>(imv4) << std::endl
                  << imv5 << " = " << quantity<si::volume>(imv5) << std::endl
                  << std::endl;
                  
        std::cout << imv5 << "/" << imv1 << " = " << quantity<si::dimensionless>(imv5/imv1) << std::endl
                  << imv5 << "/" << imv2 << " = " << quantity<si::dimensionless>(imv5/imv2) << std::endl
                  << imv5 << "/" << imv3 << " = " << quantity<si::dimensionless>(imv5/imv3) << std::endl
                  << imv5 << "/" << imv4 << " = " << quantity<si::dimensionless>(imv5/imv4) << std::endl
                  << std::endl;

        std::cout << imv1 << " = " << quantity<fluid_ounce_volume>(imv1) << std::endl
                  << imv2 << " = " << quantity<fluid_ounce_volume>(imv2) << std::endl
                  << imv3 << " = " << quantity<fluid_ounce_volume>(imv3) << std::endl
                  << imv4 << " = " << quantity<fluid_ounce_volume>(imv4) << std::endl
                  << imv5 << " = " << quantity<fluid_ounce_volume>(imv5) << std::endl
                  << std::endl;
                  
        std::cout << imv1 << " = " << quantity<gill_volume>(imv1) << std::endl
                  << imv2 << " = " << quantity<gill_volume>(imv2) << std::endl
                  << imv3 << " = " << quantity<gill_volume>(imv3) << std::endl
                  << imv4 << " = " << quantity<gill_volume>(imv4) << std::endl
                  << imv5 << " = " << quantity<gill_volume>(imv5) << std::endl
                  << std::endl;
                  
        std::cout << imv1 << " = " << quantity<pint_volume>(imv1) << std::endl
                  << imv2 << " = " << quantity<pint_volume>(imv2) << std::endl
                  << imv3 << " = " << quantity<pint_volume>(imv3) << std::endl
                  << imv4 << " = " << quantity<pint_volume>(imv4) << std::endl
                  << imv5 << " = " << quantity<pint_volume>(imv5) << std::endl
                  << std::endl;
                  
        std::cout << imv1 << " = " << quantity<quart_volume>(imv1) << std::endl
                  << imv2 << " = " << quantity<quart_volume>(imv2) << std::endl
                  << imv3 << " = " << quantity<quart_volume>(imv3) << std::endl
                  << imv4 << " = " << quantity<quart_volume>(imv4) << std::endl
                  << imv5 << " = " << quantity<quart_volume>(imv5) << std::endl
                  << std::endl;
                  
        std::cout << imv1 << " = " << quantity<gallon_volume>(imv1) << std::endl
                  << imv2 << " = " << quantity<gallon_volume>(imv2) << std::endl
                  << imv3 << " = " << quantity<gallon_volume>(imv3) << std::endl
                  << imv4 << " = " << quantity<gallon_volume>(imv4) << std::endl
                  << imv5 << " = " << quantity<gallon_volume>(imv5) << std::endl
                  << std::endl;
    }
    
    {
        using namespace boost::units::metric;
        
        std::cout << "Testing metric base units..." << std::endl;
        
        quantity<fermi_length>            ml1(1.0*fermis);
        quantity<angstrom_length>        ml2(1.0*angstroms);
        quantity<micron_length>            ml3(1.0*microns);
        quantity<nautical_mile_length>    ml4(1.0*nautical_miles);
    
        std::cout << ml1 << " = " << quantity<si::length>(ml1) << std::endl
                  << ml2 << " = " << quantity<si::length>(ml2) << std::endl
                  << ml3 << " = " << quantity<si::length>(ml3) << std::endl
                  << ml4 << " = " << quantity<si::length>(ml4) << std::endl
                  << std::endl;

        std::cout << ml4 << "/" << ml1 << " = " << quantity<si::dimensionless>(ml4/ml1) << std::endl
                  << ml4 << "/" << ml2 << " = " << quantity<si::dimensionless>(ml4/ml2) << std::endl
                  << ml4 << "/" << ml3 << " = " << quantity<si::dimensionless>(ml4/ml3) << std::endl
                  << std::endl;

        std::cout << ml1 << " = " << quantity<fermi_length>(ml1) << std::endl
                  << ml2 << " = " << quantity<fermi_length>(ml2) << std::endl
                  << ml3 << " = " << quantity<fermi_length>(ml3) << std::endl
                  << ml4 << " = " << quantity<fermi_length>(ml4) << std::endl
                  << std::endl;

        std::cout << ml1 << " = " << quantity<angstrom_length>(ml1) << std::endl
                  << ml2 << " = " << quantity<angstrom_length>(ml2) << std::endl
                  << ml3 << " = " << quantity<angstrom_length>(ml3) << std::endl
                  << ml4 << " = " << quantity<angstrom_length>(ml4) << std::endl
                  << std::endl;

        std::cout << ml1 << " = " << quantity<micron_length>(ml1) << std::endl
                  << ml2 << " = " << quantity<micron_length>(ml2) << std::endl
                  << ml3 << " = " << quantity<micron_length>(ml3) << std::endl
                  << ml4 << " = " << quantity<micron_length>(ml4) << std::endl
                  << std::endl;

        std::cout << ml1 << " = " << quantity<nautical_mile_length>(ml1) << std::endl
                  << ml2 << " = " << quantity<nautical_mile_length>(ml2) << std::endl
                  << ml3 << " = " << quantity<nautical_mile_length>(ml3) << std::endl
                  << ml4 << " = " << quantity<nautical_mile_length>(ml4) << std::endl
                  << std::endl;

        quantity<ton_mass>    mm1(1.0*tons);
        
        std::cout << mm1 << " = " << quantity<cgs::mass>(mm1) << std::endl
                  //<< quantity<si::mass>(mm1) << std::endl    // this should work... 
                  << std::endl;
                  
        quantity<minute_time>    mt1(1.0*minutes);
        quantity<hour_time>        mt2(1.0*hours);
        quantity<day_time>        mt3(1.0*days);
        quantity<year_time>        mt4(1.0*years);
        
        std::cout << mt1 << " = " << quantity<si::time>(mt1) << std::endl
                  << mt2 << " = " << quantity<si::time>(mt2) << std::endl
                  << mt3 << " = " << quantity<si::time>(mt3) << std::endl
                  << mt4 << " = " << quantity<si::time>(mt4) << std::endl
                  << std::endl;

        std::cout << mt4 << "/" << mt1 << " = " << quantity<si::dimensionless>(mt4/mt1) << std::endl
                  << mt4 << "/" << mt2 << " = " << quantity<si::dimensionless>(mt4/mt2) << std::endl
                  << mt4 << "/" << mt3 << " = " << quantity<si::dimensionless>(mt4/mt3) << std::endl
                  << std::endl;

        std::cout << mt1 << " = " << quantity<minute_time>(mt1) << std::endl
                  << mt2 << " = " << quantity<minute_time>(mt2) << std::endl
                  << mt3 << " = " << quantity<minute_time>(mt3) << std::endl
                  << mt4 << " = " << quantity<minute_time>(mt4) << std::endl
                  << std::endl;

        std::cout << mt1 << " = " << quantity<hour_time>(mt1) << std::endl
                  << mt2 << " = " << quantity<hour_time>(mt2) << std::endl
                  << mt3 << " = " << quantity<hour_time>(mt3) << std::endl
                  << mt4 << " = " << quantity<hour_time>(mt4) << std::endl
                  << std::endl;

        std::cout << mt1 << " = " << quantity<day_time>(mt1) << std::endl
                  << mt2 << " = " << quantity<day_time>(mt2) << std::endl
                  << mt3 << " = " << quantity<day_time>(mt3) << std::endl
                  << mt4 << " = " << quantity<day_time>(mt4) << std::endl
                  << std::endl;

        std::cout << mt1 << " = " << quantity<year_time>(mt1) << std::endl
                  << mt2 << " = " << quantity<year_time>(mt2) << std::endl
                  << mt3 << " = " << quantity<year_time>(mt3) << std::endl
                  << mt4 << " = " << quantity<year_time>(mt4) << std::endl
                  << std::endl;
                          
        quantity<knot_velocity>    ms1(1.0*knots);
        
        std::cout << ms1 << " = " << quantity<si::velocity>(ms1) << std::endl
                  << std::endl;
                  
        quantity<barn_area>        ma1(1.0*barns);
        quantity<are_area>        ma2(1.0*ares);
        quantity<hectare_area>    ma3(1.0*hectares);
        
        std::cout << ma1 << " = " << quantity<si::area>(ma1) << std::endl
                  << ma2 << " = " << quantity<si::area>(ma2) << std::endl
                  << ma3 << " = " << quantity<si::area>(ma3) << std::endl
                  << std::endl;

        std::cout << ma3 << "/" << ma1 << " = " << quantity<si::dimensionless>(ma3/ma1) << std::endl
                  << ma3 << "/" << ma2 << " = " << quantity<si::dimensionless>(ma3/ma2) << std::endl
                  << std::endl;

        std::cout << ma1 << " = " << quantity<barn_area>(ma1) << std::endl
                  << ma2 << " = " << quantity<barn_area>(ma2) << std::endl
                  << ma3 << " = " << quantity<barn_area>(ma3) << std::endl
                  << std::endl;

        std::cout << ma1 << " = " << quantity<are_area>(ma1) << std::endl
                  << ma2 << " = " << quantity<are_area>(ma2) << std::endl
                  << ma3 << " = " << quantity<are_area>(ma3) << std::endl
                  << std::endl;

        std::cout << ma1 << " = " << quantity<hectare_area>(ma1) << std::endl
                  << ma2 << " = " << quantity<hectare_area>(ma2) << std::endl
                  << ma3 << " = " << quantity<hectare_area>(ma3) << std::endl
                  << std::endl;
        
        quantity<liter_volume>    mv1(1.0*liters);
        
        std::cout << mv1 << " = " << quantity<si::volume>(mv1) << std::endl
                  << std::endl;
        
        quantity<mmHg_pressure>            mp1(1.0*mmHgs);
        quantity<torr_pressure>            mp2(1.0*torrs);
        quantity<bar_pressure>            mp3(1.0*bars);
        quantity<atmosphere_pressure>    mp4(1.0*atmospheres);
        
        std::cout << mp1 << " = " << quantity<si::pressure>(mp1) << std::endl
                  << mp2 << " = " << quantity<si::pressure>(mp2) << std::endl
                  << mp3 << " = " << quantity<si::pressure>(mp3) << std::endl
                  << mp4 << " = " << quantity<si::pressure>(mp4) << std::endl
                  << std::endl;

        std::cout << mp4 << "/" << mp1 << " = " << quantity<si::dimensionless>(mp4/mp1) << std::endl
                  << mp4 << "/" << mp2  << " = " << quantity<si::dimensionless>(mp4/mp2) << std::endl
                  << mp4 << "/" << mp3  << " = " << quantity<si::dimensionless>(mp4/mp3) << std::endl
                  << std::endl;

        std::cout << mp1 << " = " << quantity<mmHg_pressure>(mp1) << std::endl
                  << mp2 << " = " << quantity<mmHg_pressure>(mp2) << std::endl
                  << mp3 << " = " << quantity<mmHg_pressure>(mp3) << std::endl
                  << mp4 << " = " << quantity<mmHg_pressure>(mp4) << std::endl
                  << std::endl;

        std::cout << mp1 << " = " << quantity<torr_pressure>(mp1) << std::endl
                  << mp2 << " = " << quantity<torr_pressure>(mp2) << std::endl
                  << mp3 << " = " << quantity<torr_pressure>(mp3) << std::endl
                  << mp4 << " = " << quantity<torr_pressure>(mp4) << std::endl
                  << std::endl;

        std::cout << mp1 << " = " << quantity<bar_pressure>(mp1) << std::endl
                  << mp2 << " = " << quantity<bar_pressure>(mp2) << std::endl
                  << mp3 << " = " << quantity<bar_pressure>(mp3) << std::endl
                  << mp4 << " = " << quantity<bar_pressure>(mp4) << std::endl
                  << std::endl;

        std::cout << mp1 << " = " << quantity<atmosphere_pressure>(mp1) << std::endl
                  << mp2 << " = " << quantity<atmosphere_pressure>(mp2) << std::endl
                  << mp3 << " = " << quantity<atmosphere_pressure>(mp3) << std::endl
                  << mp4 << " = " << quantity<atmosphere_pressure>(mp4) << std::endl
                  << std::endl;
    }
    
    {
        using namespace boost::units::us;
        
        std::cout << "Testing U.S. customary base units..." << std::endl;
        
        quantity<mil_length>            iml1(1.0*mils);
        quantity<inch_length>            iml2(1.0*inchs);
        quantity<foot_length>            iml3(1.0*foots);
        quantity<yard_length>            iml4(1.0*yards);
        quantity<mile_length>            iml5(1.0*miles);
        
        std::cout << iml1 << " = " << quantity<si::length>(iml1) << std::endl
                  << iml2 << " = " << quantity<si::length>(iml2) << std::endl
                  << iml3 << " = " << quantity<si::length>(iml3) << std::endl
                  << iml4 << " = " << quantity<si::length>(iml4) << std::endl
                  << iml5 << " = " << quantity<si::length>(iml5) << std::endl
                  << std::endl;

        std::cout << iml5 << "/" << iml1 << " = " << quantity<si::dimensionless>(iml5/iml1) << std::endl
                  << iml5 << "/" << iml2 << " = " << quantity<si::dimensionless>(iml5/iml2) << std::endl
                  << iml5 << "/" << iml3 << " = " << quantity<si::dimensionless>(iml5/iml3) << std::endl
                  << iml5 << "/" << iml4 << " = " << quantity<si::dimensionless>(iml5/iml4) << std::endl
                  << std::endl;

        std::cout << iml1 << " = " << quantity<mil_length>(iml1) << std::endl
                  << iml2 << " = " << quantity<mil_length>(iml2) << std::endl
                  << iml3 << " = " << quantity<mil_length>(iml3) << std::endl
                  << iml4 << " = " << quantity<mil_length>(iml4) << std::endl
                  << iml5 << " = " << quantity<mil_length>(iml5) << std::endl
                  << std::endl;

        std::cout << iml1 << " = " << quantity<inch_length>(iml1) << std::endl
                  << iml2 << " = " << quantity<inch_length>(iml2) << std::endl
                  << iml3 << " = " << quantity<inch_length>(iml3) << std::endl
                  << iml4 << " = " << quantity<inch_length>(iml4) << std::endl
                  << iml5 << " = " << quantity<inch_length>(iml5) << std::endl
                  << std::endl;

        std::cout << iml1 << " = " << quantity<foot_length>(iml1) << std::endl
                  << iml2 << " = " << quantity<foot_length>(iml2) << std::endl
                  << iml3 << " = " << quantity<foot_length>(iml3) << std::endl
                  << iml4 << " = " << quantity<foot_length>(iml4) << std::endl
                  << iml5 << " = " << quantity<foot_length>(iml5) << std::endl
                  << std::endl;

        std::cout << iml1 << " = " << quantity<yard_length>(iml1) << std::endl
                  << iml2 << " = " << quantity<yard_length>(iml2) << std::endl
                  << iml3 << " = " << quantity<yard_length>(iml3) << std::endl
                  << iml4 << " = " << quantity<yard_length>(iml4) << std::endl
                  << iml5 << " = " << quantity<yard_length>(iml5) << std::endl
                  << std::endl;

        std::cout << iml1 << " = " << quantity<mile_length>(iml1) << std::endl
                  << iml2 << " = " << quantity<mile_length>(iml2) << std::endl
                  << iml3 << " = " << quantity<mile_length>(iml3) << std::endl
                  << iml4 << " = " << quantity<mile_length>(iml4) << std::endl
                  << iml5 << " = " << quantity<mile_length>(iml5) << std::endl
                  << std::endl;

        quantity<grain_mass>            imm1(1.0*grains);
        quantity<dram_mass>                imm2(1.0*drams);
        quantity<ounce_mass>            imm3(1.0*ounces);
        quantity<pound_mass>            imm4(1.0*pounds);
        quantity<hundredweight_mass>    imm5(1.0*hundredweights);
        quantity<ton_mass>                imm6(1.0*tons);

        std::cout << imm1 << " = " << quantity<si::mass>(imm1) << std::endl
                  << imm2 << " = " << quantity<si::mass>(imm2) << std::endl
                  << imm3 << " = " << quantity<si::mass>(imm3) << std::endl
                  << imm4 << " = " << quantity<si::mass>(imm4) << std::endl
                  << imm5 << " = " << quantity<si::mass>(imm5) << std::endl
                  << imm6 << " = " << quantity<si::mass>(imm6) << std::endl
                  << std::endl;
                  
        std::cout << imm6 << "/" << imm1 << " = " << quantity<si::dimensionless>(imm6/imm1) << std::endl
                  << imm6 << "/" << imm2 << " = " << quantity<si::dimensionless>(imm6/imm2) << std::endl
                  << imm6 << "/" << imm3 << " = " << quantity<si::dimensionless>(imm6/imm3) << std::endl
                  << imm6 << "/" << imm4 << " = " << quantity<si::dimensionless>(imm6/imm4) << std::endl
                  << imm6 << "/" << imm5 << " = " << quantity<si::dimensionless>(imm6/imm5) << std::endl
                  << std::endl;

        std::cout << imm1 << " = " << quantity<grain_mass>(imm1) << std::endl
                  << imm2 << " = " << quantity<grain_mass>(imm2) << std::endl
                  << imm3 << " = " << quantity<grain_mass>(imm3) << std::endl
                  << imm4 << " = " << quantity<grain_mass>(imm4) << std::endl
                  << imm5 << " = " << quantity<grain_mass>(imm5) << std::endl
                  << imm6 << " = " << quantity<grain_mass>(imm6) << std::endl
                  << std::endl;
                  
        std::cout << imm1 << " = " << quantity<dram_mass>(imm1) << std::endl
                  << imm2 << " = " << quantity<dram_mass>(imm2) << std::endl
                  << imm3 << " = " << quantity<dram_mass>(imm3) << std::endl
                  << imm4 << " = " << quantity<dram_mass>(imm4) << std::endl
                  << imm5 << " = " << quantity<dram_mass>(imm5) << std::endl
                  << imm6 << " = " << quantity<dram_mass>(imm6) << std::endl
                  << std::endl;
                  
        std::cout << imm1 << " = " << quantity<ounce_mass>(imm1) << std::endl
                  << imm2 << " = " << quantity<ounce_mass>(imm2) << std::endl
                  << imm3 << " = " << quantity<ounce_mass>(imm3) << std::endl
                  << imm4 << " = " << quantity<ounce_mass>(imm4) << std::endl
                  << imm5 << " = " << quantity<ounce_mass>(imm5) << std::endl
                  << imm6 << " = " << quantity<ounce_mass>(imm6) << std::endl
                  << std::endl;
                  
        std::cout << imm1 << " = " << quantity<pound_mass>(imm1) << std::endl
                  << imm2 << " = " << quantity<pound_mass>(imm2) << std::endl
                  << imm3 << " = " << quantity<pound_mass>(imm3) << std::endl
                  << imm4 << " = " << quantity<pound_mass>(imm4) << std::endl
                  << imm5 << " = " << quantity<pound_mass>(imm5) << std::endl
                  << imm6 << " = " << quantity<pound_mass>(imm6) << std::endl
                  << std::endl;
                  
        std::cout << imm1 << " = " << quantity<hundredweight_mass>(imm1) << std::endl
                  << imm2 << " = " << quantity<hundredweight_mass>(imm2) << std::endl
                  << imm3 << " = " << quantity<hundredweight_mass>(imm3) << std::endl
                  << imm4 << " = " << quantity<hundredweight_mass>(imm4) << std::endl
                  << imm5 << " = " << quantity<hundredweight_mass>(imm5) << std::endl
                  << imm6 << " = " << quantity<hundredweight_mass>(imm6) << std::endl
                  << std::endl;
                  
        std::cout << imm1 << " = " << quantity<ton_mass>(imm1) << std::endl
                  << imm2 << " = " << quantity<ton_mass>(imm2) << std::endl
                  << imm3 << " = " << quantity<ton_mass>(imm3) << std::endl
                  << imm4 << " = " << quantity<ton_mass>(imm4) << std::endl
                  << imm5 << " = " << quantity<ton_mass>(imm5) << std::endl
                  << imm6 << " = " << quantity<ton_mass>(imm6) << std::endl
                  << std::endl;
                  
        quantity<minim_volume>            imv1(1.0*minims);
        quantity<fluid_dram_volume>        imv2(1.0*fluid_drams);
        quantity<teaspoon_volume>        imv3(1.0*teaspoons);
        quantity<tablespoon_volume>        imv4(1.0*tablespoons);
        quantity<fluid_ounce_volume>    imv5(1.0*fluid_ounces);
        quantity<gill_volume>            imv6(1.0*gills);
        quantity<cup_volume>            imv7(1.0*cups);
        quantity<pint_volume>            imv8(1.0*pints);
        quantity<quart_volume>            imv9(1.0*quarts);
        quantity<gallon_volume>            imv10(1.0*gallons);
        
        std::cout << imv1 << " = " << quantity<si::volume>(imv1) << std::endl
                  << imv2 << " = " << quantity<si::volume>(imv2) << std::endl
                  << imv3 << " = " << quantity<si::volume>(imv3) << std::endl
                  << imv4 << " = " << quantity<si::volume>(imv4) << std::endl
                  << imv5 << " = " << quantity<si::volume>(imv5) << std::endl
                  << imv6 << " = " << quantity<si::volume>(imv6) << std::endl
                  << imv7 << " = " << quantity<si::volume>(imv7) << std::endl
                  << imv8 << " = " << quantity<si::volume>(imv8) << std::endl
                  << imv9 << " = " << quantity<si::volume>(imv9) << std::endl
                  << imv10 << " = " << quantity<si::volume>(imv10) << std::endl
                  << std::endl;

        std::cout << imv10 << "/" << imv1 << " = " << quantity<si::dimensionless>(imv10/imv1) << std::endl
                  << imv10 << "/" << imv2 << " = " << quantity<si::dimensionless>(imv10/imv2) << std::endl
                  << imv10 << "/" << imv3 << " = " << quantity<si::dimensionless>(imv10/imv3) << std::endl
                  << imv10 << "/" << imv4 << " = " << quantity<si::dimensionless>(imv10/imv4) << std::endl
                  << imv10 << "/" << imv5 << " = " << quantity<si::dimensionless>(imv10/imv5) << std::endl
                  << imv10 << "/" << imv6 << " = " << quantity<si::dimensionless>(imv10/imv6) << std::endl
                  << imv10 << "/" << imv7 << " = " << quantity<si::dimensionless>(imv10/imv7) << std::endl
                  << imv10 << "/" << imv8 << " = " << quantity<si::dimensionless>(imv10/imv8) << std::endl
                  << imv10 << "/" << imv9 << " = " << quantity<si::dimensionless>(imv10/imv9) << std::endl
                  << std::endl;

        std::cout << imv1 << " = " << quantity<minim_volume>(imv1) << std::endl
                  << imv2 << " = " << quantity<minim_volume>(imv2) << std::endl
                  << imv3 << " = " << quantity<minim_volume>(imv3) << std::endl
                  << imv4 << " = " << quantity<minim_volume>(imv4) << std::endl
                  << imv5 << " = " << quantity<minim_volume>(imv5) << std::endl
                  << imv6 << " = " << quantity<minim_volume>(imv6) << std::endl
                  << imv7 << " = " << quantity<minim_volume>(imv7) << std::endl
                  << imv8 << " = " << quantity<minim_volume>(imv8) << std::endl
                  << imv9 << " = " << quantity<minim_volume>(imv9) << std::endl
                  << imv10 << " = " << quantity<minim_volume>(imv10) << std::endl
                  << std::endl;

        std::cout << imv1 << " = " << quantity<fluid_dram_volume>(imv1) << std::endl
                  << imv2 << " = " << quantity<fluid_dram_volume>(imv2) << std::endl
                  << imv3 << " = " << quantity<fluid_dram_volume>(imv3) << std::endl
                  << imv4 << " = " << quantity<fluid_dram_volume>(imv4) << std::endl
                  << imv5 << " = " << quantity<fluid_dram_volume>(imv5) << std::endl
                  << imv6 << " = " << quantity<fluid_dram_volume>(imv6) << std::endl
                  << imv7 << " = " << quantity<fluid_dram_volume>(imv7) << std::endl
                  << imv8 << " = " << quantity<fluid_dram_volume>(imv8) << std::endl
                  << imv9 << " = " << quantity<fluid_dram_volume>(imv9) << std::endl
                  << imv10 << " = " << quantity<fluid_dram_volume>(imv10) << std::endl
                  << std::endl;

        std::cout << imv1 << " = " << quantity<teaspoon_volume>(imv1) << std::endl
                  << imv2 << " = " << quantity<teaspoon_volume>(imv2) << std::endl
                  << imv3 << " = " << quantity<teaspoon_volume>(imv3) << std::endl
                  << imv4 << " = " << quantity<teaspoon_volume>(imv4) << std::endl
                  << imv5 << " = " << quantity<teaspoon_volume>(imv5) << std::endl
                  << imv6 << " = " << quantity<teaspoon_volume>(imv6) << std::endl
                  << imv7 << " = " << quantity<teaspoon_volume>(imv7) << std::endl
                  << imv8 << " = " << quantity<teaspoon_volume>(imv8) << std::endl
                  << imv9 << " = " << quantity<teaspoon_volume>(imv9) << std::endl
                  << imv10 << " = " << quantity<teaspoon_volume>(imv10) << std::endl
                  << std::endl;

        std::cout << imv1 << " = " << quantity<tablespoon_volume>(imv1) << std::endl
                  << imv2 << " = " << quantity<tablespoon_volume>(imv2) << std::endl
                  << imv3 << " = " << quantity<tablespoon_volume>(imv3) << std::endl
                  << imv4 << " = " << quantity<tablespoon_volume>(imv4) << std::endl
                  << imv5 << " = " << quantity<tablespoon_volume>(imv5) << std::endl
                  << imv6 << " = " << quantity<tablespoon_volume>(imv6) << std::endl
                  << imv7 << " = " << quantity<tablespoon_volume>(imv7) << std::endl
                  << imv8 << " = " << quantity<tablespoon_volume>(imv8) << std::endl
                  << imv9 << " = " << quantity<tablespoon_volume>(imv9) << std::endl
                  << imv10 << " = " << quantity<tablespoon_volume>(imv10) << std::endl
                  << std::endl;

        std::cout << imv1 << " = " << quantity<fluid_ounce_volume>(imv1) << std::endl
                  << imv2 << " = " << quantity<fluid_ounce_volume>(imv2) << std::endl
                  << imv3 << " = " << quantity<fluid_ounce_volume>(imv3) << std::endl
                  << imv4 << " = " << quantity<fluid_ounce_volume>(imv4) << std::endl
                  << imv5 << " = " << quantity<fluid_ounce_volume>(imv5) << std::endl
                  << imv6 << " = " << quantity<fluid_ounce_volume>(imv6) << std::endl
                  << imv7 << " = " << quantity<fluid_ounce_volume>(imv7) << std::endl
                  << imv8 << " = " << quantity<fluid_ounce_volume>(imv8) << std::endl
                  << imv9 << " = " << quantity<fluid_ounce_volume>(imv9) << std::endl
                  << imv10 << " = " << quantity<fluid_ounce_volume>(imv10) << std::endl
                  << std::endl;

        std::cout << imv1 << " = " << quantity<gill_volume>(imv1) << std::endl
                  << imv2 << " = " << quantity<gill_volume>(imv2) << std::endl
                  << imv3 << " = " << quantity<gill_volume>(imv3) << std::endl
                  << imv4 << " = " << quantity<gill_volume>(imv4) << std::endl
                  << imv5 << " = " << quantity<gill_volume>(imv5) << std::endl
                  << imv6 << " = " << quantity<gill_volume>(imv6) << std::endl
                  << imv7 << " = " << quantity<gill_volume>(imv7) << std::endl
                  << imv8 << " = " << quantity<gill_volume>(imv8) << std::endl
                  << imv9 << " = " << quantity<gill_volume>(imv9) << std::endl
                  << imv10 << " = " << quantity<gill_volume>(imv10) << std::endl
                  << std::endl;

        std::cout << imv1 << " = " << quantity<cup_volume>(imv1) << std::endl
                  << imv2 << " = " << quantity<cup_volume>(imv2) << std::endl
                  << imv3 << " = " << quantity<cup_volume>(imv3) << std::endl
                  << imv4 << " = " << quantity<cup_volume>(imv4) << std::endl
                  << imv5 << " = " << quantity<cup_volume>(imv5) << std::endl
                  << imv6 << " = " << quantity<cup_volume>(imv6) << std::endl
                  << imv7 << " = " << quantity<cup_volume>(imv7) << std::endl
                  << imv8 << " = " << quantity<cup_volume>(imv8) << std::endl
                  << imv9 << " = " << quantity<cup_volume>(imv9) << std::endl
                  << imv10 << " = " << quantity<cup_volume>(imv10) << std::endl
                  << std::endl;

        std::cout << imv1 << " = " << quantity<pint_volume>(imv1) << std::endl
                  << imv2 << " = " << quantity<pint_volume>(imv2) << std::endl
                  << imv3 << " = " << quantity<pint_volume>(imv3) << std::endl
                  << imv4 << " = " << quantity<pint_volume>(imv4) << std::endl
                  << imv5 << " = " << quantity<pint_volume>(imv5) << std::endl
                  << imv6 << " = " << quantity<pint_volume>(imv6) << std::endl
                  << imv7 << " = " << quantity<pint_volume>(imv7) << std::endl
                  << imv8 << " = " << quantity<pint_volume>(imv8) << std::endl
                  << imv9 << " = " << quantity<pint_volume>(imv9) << std::endl
                  << imv10 << " = " << quantity<pint_volume>(imv10) << std::endl
                  << std::endl;

        std::cout << imv1 << " = " << quantity<quart_volume>(imv1) << std::endl
                  << imv2 << " = " << quantity<quart_volume>(imv2) << std::endl
                  << imv3 << " = " << quantity<quart_volume>(imv3) << std::endl
                  << imv4 << " = " << quantity<quart_volume>(imv4) << std::endl
                  << imv5 << " = " << quantity<quart_volume>(imv5) << std::endl
                  << imv6 << " = " << quantity<quart_volume>(imv6) << std::endl
                  << imv7 << " = " << quantity<quart_volume>(imv7) << std::endl
                  << imv8 << " = " << quantity<quart_volume>(imv8) << std::endl
                  << imv9 << " = " << quantity<quart_volume>(imv9) << std::endl
                  << imv10 << " = " << quantity<quart_volume>(imv10) << std::endl
                  << std::endl;

        std::cout << imv1 << " = " << quantity<gallon_volume>(imv1) << std::endl
                  << imv2 << " = " << quantity<gallon_volume>(imv2) << std::endl
                  << imv3 << " = " << quantity<gallon_volume>(imv3) << std::endl
                  << imv4 << " = " << quantity<gallon_volume>(imv4) << std::endl
                  << imv5 << " = " << quantity<gallon_volume>(imv5) << std::endl
                  << imv6 << " = " << quantity<gallon_volume>(imv6) << std::endl
                  << imv7 << " = " << quantity<gallon_volume>(imv7) << std::endl
                  << imv8 << " = " << quantity<gallon_volume>(imv8) << std::endl
                  << imv9 << " = " << quantity<gallon_volume>(imv9) << std::endl
                  << imv10 << " = " << quantity<gallon_volume>(imv10) << std::endl
                  << std::endl;
    }
    
    return 0;
}
