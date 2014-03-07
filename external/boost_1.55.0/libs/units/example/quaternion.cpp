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
    
\brief quaternion.cpp

\details
Demonstrate interoperability with Boost.Quaternion.

Output:
@verbatim

//[quaternion_output_1
+L      = (4,3,2,1) m
-L      = (-4,-3,-2,-1) m
L+L     = (8,6,4,2) m
L-L     = (0,0,0,0) m
L*L     = (2,24,16,8) m^2
L/L     = (1,0,0,0) dimensionless 
L^3     = (-104,102,68,34) m^3
//]

//[quaternion_output_2
+L      = (4 m,3 m,2 m,1 m)
-L      = (-4 m,-3 m,-2 m,-1 m)
L+L     = (8 m,6 m,4 m,2 m)
L-L     = (0 m,0 m,0 m,0 m)
L^3     = (-104 m^3,102 m^3,68 m^3,34 m^3)
//]

@endverbatim
**/

#include <iostream>

#include <boost/math/quaternion.hpp>
#include <boost/mpl/list.hpp>

#include <boost/units/pow.hpp>
#include <boost/units/quantity.hpp>
#include <boost/units/io.hpp>

#include "test_system.hpp"

#if BOOST_UNITS_HAS_BOOST_TYPEOF

#include BOOST_TYPEOF_INCREMENT_REGISTRATION_GROUP()

BOOST_TYPEOF_REGISTER_TEMPLATE(boost::math::quaternion, 1)

#endif

namespace boost {

namespace units {

//[quaternion_class_snippet_1a
/// specialize power typeof helper
template<class Y,long N,long D> 
struct power_typeof_helper<boost::math::quaternion<Y>,static_rational<N,D> >
{ 
    // boost::math::quaternion only supports integer powers
    BOOST_STATIC_ASSERT(D==1);
    
    typedef boost::math::quaternion<
        typename power_typeof_helper<Y,static_rational<N,D> >::type
    > type; 
    
    static type value(const boost::math::quaternion<Y>& x)  
    {   
        return boost::math::pow(x,static_cast<int>(N));
    }
};
//]

//[quaternion_class_snippet_1b
/// specialize root typeof helper
template<class Y,long N,long D> 
struct root_typeof_helper<boost::math::quaternion<Y>,static_rational<N,D> >
{ 
    // boost::math::quaternion only supports integer powers
    BOOST_STATIC_ASSERT(N==1);
    
    typedef boost::math::quaternion<
        typename root_typeof_helper<Y,static_rational<N,D> >::type
    > type; 
    
    static type value(const boost::math::quaternion<Y>& x)  
    { 
        return boost::math::pow(x,static_cast<int>(D));
    }
};
//]

//[quaternion_class_snippet_2a
/// specialize power typeof helper for quaternion<quantity<Unit,Y> >
template<class Unit,long N,long D,class Y> 
struct power_typeof_helper<
    boost::math::quaternion<quantity<Unit,Y> >,
    static_rational<N,D> >                
{ 
    typedef typename power_typeof_helper<
        Y,
        static_rational<N,D>
    >::type     value_type;

    typedef typename power_typeof_helper<
        Unit,
        static_rational<N,D>
    >::type  unit_type;

    typedef quantity<unit_type,value_type>         quantity_type;
    typedef boost::math::quaternion<quantity_type> type; 
    
    static type value(const boost::math::quaternion<quantity<Unit,Y> >& x)  
    { 
        const boost::math::quaternion<value_type>   tmp = 
            pow<static_rational<N,D> >(boost::math::quaternion<Y>(
                x.R_component_1().value(),
                x.R_component_2().value(),
                x.R_component_3().value(),
                x.R_component_4().value()));
        
        return type(quantity_type::from_value(tmp.R_component_1()),
                    quantity_type::from_value(tmp.R_component_2()),
                    quantity_type::from_value(tmp.R_component_3()),
                    quantity_type::from_value(tmp.R_component_4()));
    }
};
//]

//[quaternion_class_snippet_2b
/// specialize root typeof helper for quaternion<quantity<Unit,Y> >
template<class Unit,long N,long D,class Y> 
struct root_typeof_helper<
    boost::math::quaternion<quantity<Unit,Y> >,
    static_rational<N,D> >                
{ 
    typedef typename root_typeof_helper<
        Y,
        static_rational<N,D>
    >::type      value_type;

    typedef typename root_typeof_helper<
        Unit,
        static_rational<N,D>
    >::type   unit_type;

    typedef quantity<unit_type,value_type>         quantity_type;
    typedef boost::math::quaternion<quantity_type> type; 
    
    static type value(const boost::math::quaternion<quantity<Unit,Y> >& x)  
    { 
        const boost::math::quaternion<value_type>   tmp = 
            root<static_rational<N,D> >(boost::math::quaternion<Y>(
                x.R_component_1().value(),
                x.R_component_2().value(),
                x.R_component_3().value(),
                x.R_component_4().value()));
        
        return type(quantity_type::from_value(tmp.R_component_1()),
                    quantity_type::from_value(tmp.R_component_2()),
                    quantity_type::from_value(tmp.R_component_3()),
                    quantity_type::from_value(tmp.R_component_4()));
    }
};
//]

} // namespace units

} // namespace boost

int main(void)
{
    using boost::math::quaternion;
    using namespace boost::units;
    using namespace boost::units::test;
    using boost::units::pow;
    
    {
    //[quaternion_snippet_1
    typedef quantity<length,quaternion<double> >     length_dimension;
        
    length_dimension    L(quaternion<double>(4.0,3.0,2.0,1.0)*meters);
    //]
    
    std::cout << "+L      = " << +L << std::endl
              << "-L      = " << -L << std::endl
              << "L+L     = " << L+L << std::endl
              << "L-L     = " << L-L << std::endl
              << "L*L     = " << L*L << std::endl
              << "L/L     = " << L/L << std::endl
              // unfortunately, without qualification msvc still
              // finds boost::math::pow by ADL.
              << "L^3     = " << boost::units::pow<3>(L) << std::endl
//              << "L^(3/2) = " << pow< static_rational<3,2> >(L) << std::endl
//              << "3vL     = " << root<3>(L) << std::endl
//              << "(3/2)vL = " << root< static_rational<3,2> >(L) << std::endl
              << std::endl;
    }
    
    {
    //[quaternion_snippet_2
    typedef quaternion<quantity<length> >     length_dimension;
        
    length_dimension    L(4.0*meters,3.0*meters,2.0*meters,1.0*meters);
    //]
    
    std::cout << "+L      = " << +L << std::endl
              << "-L      = " << -L << std::endl
              << "L+L     = " << L+L << std::endl
              << "L-L     = " << L-L << std::endl
//              << "L*L     = " << L*L << std::endl
//              << "L/L     = " << L/L << std::endl
              << "L^3     = " << boost::units::pow<3>(L) << std::endl
//              << "L^(3/2) = " << pow< static_rational<3,2> >(L) << std::endl
//              << "3vL     = " << root<3>(L) << std::endl
//              << "(3/2)vL = " << root< static_rational<3,2> >(L) << std::endl
              << std::endl;
    }

    return 0;
}
