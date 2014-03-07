// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2008 Matthias Christian Schabel
// Copyright (C) 2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/** 
\file
    
\brief test_custom_unit.cpp

\details
Make sure that a minimal + - * / unit class is fully functional.

Output:
@verbatim
@endverbatim
**/

#include <boost/units/quantity.hpp>

#include <boost/test/minimal.hpp>

namespace bu = boost::units;

template<int Mass, int Length, int Time>
struct simple_unit {};

BOOST_TYPEOF_REGISTER_TEMPLATE(simple_unit, (int)(int)(int))

template<int Mass, int Length, int Time>
simple_unit<Mass, Length, Time> operator+(const simple_unit<Mass, Length, Time>&,
                                          const simple_unit<Mass, Length, Time>&)
{
    return simple_unit<Mass, Length, Time>();
}

template<int Mass, int Length, int Time>
simple_unit<Mass, Length, Time> operator-(const simple_unit<Mass, Length, Time>&,
                                          const simple_unit<Mass, Length, Time>&)
{
    return simple_unit<Mass, Length, Time>();
}

template<int Mass1, int Length1, int Time1, int Mass2, int Length2, int Time2>
simple_unit<Mass1 + Mass2, Length1 + Length2, Time1 + Time2>
operator*(const simple_unit<Mass1, Length1, Time1>&,
          const simple_unit<Mass2, Length2, Time2>&) 
{
    return simple_unit<Mass1 + Mass2, Length1 + Length2, Time1 + Time2>();
}

template<int Mass1, int Length1, int Time1, int Mass2, int Length2, int Time2>
simple_unit<Mass1 - Mass2, Length1 - Length2, Time1 - Time2>
operator/(const simple_unit<Mass1, Length1, Time1>&,
          const simple_unit<Mass2, Length2, Time2>&)
{
    return simple_unit<Mass1 - Mass2, Length1 - Length2, Time1 - Time2>();
}

int test_main(int,char *[])
{
    bu::quantity<simple_unit<1, 0, 0> > mass = bu::quantity<simple_unit<1, 0, 0> >::from_value(2);
    bu::quantity<simple_unit<0, 1, 0> > length = bu::quantity<simple_unit<0, 1, 0> >::from_value(4);

    bu::quantity<simple_unit<1, 1, 0> > ml = mass * length;
    bu::quantity<simple_unit<1, -1, 0> > m_per_l = mass/length;

    BOOST_CHECK(ml.value() == 8);
    BOOST_CHECK(m_per_l.value() == 0.5);

    mass += mass;

    BOOST_CHECK(mass.value() == 4);

    length -= length;
    BOOST_CHECK(length.value() == 0);

    return 0;
}
