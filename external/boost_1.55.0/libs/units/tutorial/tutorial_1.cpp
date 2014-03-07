// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>

#include <boost/units/units_fwd.hpp>

#include <boost/units/base_dimension.hpp>
#include <boost/units/base_unit.hpp>
#include <boost/units/derived_dimension.hpp>
#include <boost/units/make_system.hpp>
#include <boost/units/io.hpp>
#include <boost/units/quantity.hpp>
#include <boost/units/static_constant.hpp>
#include <boost/units/unit.hpp>

namespace boost {

namespace units {

struct length_base_dimension : boost::units::base_dimension<length_base_dimension,1> { };       ///> base dimension of length
struct time_base_dimension : boost::units::base_dimension<time_base_dimension,3> { };           ///> base dimension of time

typedef length_base_dimension::dimension_type    length_dimension;
typedef time_base_dimension::dimension_type      time_dimension;

struct length1_base_unit : base_unit<length1_base_unit,length_dimension,1>
{
    static std::string name()               { return "length 1"; }
    static std::string symbol()             { return "l1"; }
};

struct length2_base_unit : base_unit<length2_base_unit,length_dimension,2>
{
    static std::string name()               { return "length2"; }
    static std::string symbol()             { return "l2"; }
};

struct time1_base_unit : base_unit<time1_base_unit,time_dimension,3> 
{
    static std::string name()               { return "time1"; }
    static std::string symbol()             { return "t1"; }
};

struct time2_base_unit : base_unit<time2_base_unit,time_dimension,4> 
{
    static std::string name()               { return "time2"; }
    static std::string symbol()             { return "t2"; }
};

namespace s1 {

typedef make_system<length1_base_unit,time1_base_unit>::type   system;

/// unit typedefs
typedef unit<dimensionless_type,system>     dimensionless;

typedef unit<length_dimension,system>       length;
typedef unit<time_dimension,system>         time;

/// unit constants 
BOOST_UNITS_STATIC_CONSTANT(length1,length);
BOOST_UNITS_STATIC_CONSTANT(time1,time);

} // namespace s1

namespace s2 {

typedef make_system<length2_base_unit,time2_base_unit>::type   system;

/// unit typedefs
typedef unit<dimensionless_type,system>     dimensionless;

typedef unit<length_dimension,system>       length;
typedef unit<time_dimension,system>         time;

/// unit constants 
BOOST_UNITS_STATIC_CONSTANT(length2,length);
BOOST_UNITS_STATIC_CONSTANT(time2,time);

} // namespace s2

template<class X,class Y>
struct conversion_helper< quantity<s1::length,X>,quantity<s2::length,Y> >
{
    static quantity<s2::length,Y> convert(const quantity<s1::length,X>& source)
    {
        return quantity<s2::length,Y>::from_value(2.5*source.value());
    }
};

template<class X,class Y>
struct conversion_helper< quantity<s2::length,X>,quantity<s1::length,Y> >
{
    static quantity<s1::length,Y> convert(const quantity<s2::length,X>& source)
    {
        return quantity<s1::length,Y>::from_value((1.0/2.5)*source.value());
    }
};

template<class X,class Y>
struct conversion_helper< quantity<s1::time,X>,quantity<s2::time,Y> >
{
    static quantity<s2::time,Y> convert(const quantity<s1::time,X>& source)
    {
        return quantity<s2::time,Y>::from_value(0.5*source.value());
    }
};

} // namespace units

} // namespace boost

int main(void)
{
    using namespace boost::units;

    quantity<s1::length,float>  l1(1.0*s1::length1);
    quantity<s2::length,double> l2(1.5*l1);
    quantity<s1::length,float>  l3(2.0*l2/3.0);

    quantity<s1::time,float>    t1(1.0*s1::time1);
    quantity<s2::time,double>   t2(1.5*t1);
//    quantity<s1::time,float>    t3(2.0*t2/3.0);
    
    return 0;
}

/*
// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>

#include <boost/units/units_fwd.hpp>

#include <boost/units/base_dimension.hpp>
#include <boost/units/base_unit.hpp>
#include <boost/units/derived_dimension.hpp>
#include <boost/units/make_system.hpp>
#include <boost/units/io.hpp>
#include <boost/units/quantity.hpp>
#include <boost/units/static_constant.hpp>
#include <boost/units/unit.hpp>

namespace boost {

namespace units {

struct length_base_dimension : boost::units::base_dimension<length_base_dimension,1> { };       ///> base dimension of length
struct mass_base_dimension : boost::units::base_dimension<mass_base_dimension,2> { };           ///> base dimension of mass
struct time_base_dimension : boost::units::base_dimension<time_base_dimension,3> { };           ///> base dimension of time

typedef length_base_dimension::dimension_type    length_dimension;
typedef mass_base_dimension::dimension_type      mass_dimension;
typedef time_base_dimension::dimension_type      time_dimension;

struct centimeter_base_unit : base_unit<centimeter_base_unit,length_dimension,1>
{
    static std::string name()               { return "centimeter"; }
    static std::string symbol()             { return "cm"; }
};

struct gram_base_unit : base_unit<gram_base_unit,mass_dimension,2> 
{
    static std::string name()               { return "gram"; }
    static std::string symbol()             { return "g"; }
};

struct second_base_unit : base_unit<second_base_unit,time_dimension,3> 
{
    static std::string name()               { return "second"; }
    static std::string symbol()             { return "s"; }
};

namespace CG {

typedef make_system<centimeter_base_unit,gram_base_unit>::type   system;

/// unit typedefs
typedef unit<dimensionless_type,system>     dimensionless;

typedef unit<length_dimension,system>       length;
typedef unit<mass_dimension,system>         mass;

/// unit constants 
BOOST_UNITS_STATIC_CONSTANT(centimeter,length);
BOOST_UNITS_STATIC_CONSTANT(gram,mass);

} // namespace CG

namespace cgs {

typedef make_system<centimeter_base_unit,gram_base_unit,second_base_unit>::type system;

/// unit typedefs
typedef unit<dimensionless_type,system>     dimensionless;

typedef unit<length_dimension,system>       length;
typedef unit<mass_dimension,system>         mass;
typedef unit<time_dimension,system>         time;

/// unit constants 
BOOST_UNITS_STATIC_CONSTANT(centimeter,length);
BOOST_UNITS_STATIC_CONSTANT(gram,mass);
BOOST_UNITS_STATIC_CONSTANT(second,time);

} // namespace cgs

namespace esu {

typedef make_system<centimeter_base_unit,
                    gram_base_unit,
                    second_base_unit>::type system;

/// derived dimension for force in electrostatic units : L M T^-2
typedef derived_dimension<length_base_dimension,1,
                          mass_base_dimension,1,
                          time_base_dimension,-2>::type                                             force_dimension;
                          
/// derived dimension for charge in electrostatic units : L^3/2 M^1/2 T^-1
typedef make_dimension_list< mpl::list< dim<length_base_dimension,static_rational<3,2> >,
                                        dim<mass_base_dimension,static_rational<1,2> >,
                                        dim<time_base_dimension,static_rational<-1> > > >::type     charge_dimension; 

/// derived dimension for current in electrostatic units : L^3/2 M^1/2 T^-2
typedef make_dimension_list< mpl::list< dim<length_base_dimension,static_rational<3,2> >,
                                        dim<mass_base_dimension,static_rational<1,2> >,
                                        dim<time_base_dimension,static_rational<-2> > > >::type     current_dimension; 

/// derived dimension for electric potential in electrostatic units : L^1/2 M^1/2 T^-1
typedef make_dimension_list< mpl::list< dim<length_base_dimension,static_rational<1,2> >,
                                        dim<mass_base_dimension,static_rational<1,2> >,
                                        dim<time_base_dimension,static_rational<-1> > > >::type     electric_potential_dimension; 

/// derived dimension for electric field in electrostatic units : L^-1/2 M^1/2 T^-1
typedef make_dimension_list< mpl::list< dim<length_base_dimension,static_rational<-1,2> >,
                                        dim<mass_base_dimension,static_rational<1,2> >,
                                        dim<time_base_dimension,static_rational<-1> > > >::type     electric_field_dimension; 

/// unit typedefs
typedef unit<dimensionless_type,system>     dimensionless;

typedef unit<length_dimension,system>       length;
typedef unit<mass_dimension,system>         mass;
typedef unit<time_dimension,system>         time;

typedef unit<force_dimension,system>        force;

typedef unit<charge_dimension,system>               charge;
typedef unit<current_dimension,system>              current;
typedef unit<electric_potential_dimension,system>   electric_potential;
typedef unit<electric_field_dimension,system>       electric_field;

/// unit constants 
BOOST_UNITS_STATIC_CONSTANT(centimeter,length);
BOOST_UNITS_STATIC_CONSTANT(gram,mass);
BOOST_UNITS_STATIC_CONSTANT(second,time);

BOOST_UNITS_STATIC_CONSTANT(dyne,force);

BOOST_UNITS_STATIC_CONSTANT(esu,charge);
BOOST_UNITS_STATIC_CONSTANT(statvolt,electric_potential);

} // namespace esu

template<class Y>
quantity<esu::force,Y> coulombLaw(const quantity<esu::charge,Y>& q1,
                                  const quantity<esu::charge,Y>& q2,
                                  const quantity<esu::length,Y>& r)
{
    return q1*q2/(r*r);
}

} // namespace units

} // namespace boost

int main(void)
{
    using namespace boost::units;

    quantity<CG::length>    cg_length(1.0*CG::centimeter);
    quantity<cgs::length>   cgs_length(1.0*cgs::centimeter);
    
    std::cout << cg_length/cgs_length << std::endl;

    std::cout << esu::gram*pow<2>(esu::centimeter/esu::second)/esu::esu << std::endl;
    std::cout << esu::statvolt/esu::centimeter << std::endl;
    
    quantity<esu::charge>   q1 = 1.0*esu::esu,
                            q2 = 2.0*esu::esu;
    quantity<esu::length>   r = 1.0*esu::centimeter;
    
    std::cout << coulombLaw(q1,q2,r) << std::endl;
    std::cout << coulombLaw(q1,q2,cgs_length) << std::endl;
    
    return 0;
}
*/
