// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/units/base_dimension.hpp>
#include <boost/units/base_unit.hpp>
#include <boost/units/unit.hpp>
#include <boost/units/quantity.hpp>

//[runtime_conversion_factor_snippet_1

using boost::units::base_dimension;
using boost::units::base_unit;

static const long currency_base = 1;

struct currency_base_dimension : base_dimension<currency_base_dimension, 1> {};

typedef currency_base_dimension::dimension_type currency_type;

template<long N>
struct currency_base_unit :
    base_unit<currency_base_unit<N>, currency_type, currency_base + N> {};

typedef currency_base_unit<0> us_dollar_base_unit;
typedef currency_base_unit<1> euro_base_unit;

typedef us_dollar_base_unit::unit_type us_dollar;
typedef euro_base_unit::unit_type euro;

// an array of all possible conversions
double conversion_factors[2][2] = {
    {1.0, 1.0},
    {1.0, 1.0}
};

double get_conversion_factor(long from, long to) {
    return(conversion_factors[from][to]);
}

void set_conversion_factor(long from, long to, double value) {
    conversion_factors[from][to] = value;
    conversion_factors[to][from] = 1.0 / value;
}

BOOST_UNITS_DEFINE_CONVERSION_FACTOR_TEMPLATE((long N1)(long N2),
    currency_base_unit<N1>,
    currency_base_unit<N2>,
    double, get_conversion_factor(N1, N2));

//]

int main() {
    boost::units::quantity<us_dollar> dollars = 2.00 * us_dollar();
    boost::units::quantity<euro> euros(dollars);
    set_conversion_factor(0, 1, 2.0);
    dollars = static_cast<boost::units::quantity<us_dollar> >(euros);
    set_conversion_factor(0, 1, .5);
    euros = static_cast<boost::units::quantity<euro> >(dollars);
    double value = euros.value(); // = .5
    if(value != .5) {
        return(1);
    } else {
        return(0);
    }
}
