// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2007-2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/units/quantity.hpp>
#include <boost/units/unit.hpp>
#include <boost/units/systems/si/length.hpp>

namespace bu = boost::units;

int main() {
    bu::quantity<bu::si::length, double> q1;
    bu::quantity<bu::si::length, int> q2;
    q1 + q2;
    q1 -= q2;
}
