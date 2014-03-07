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
    
\brief test_base_dimension.cpp

\details
Test base_dimension class.

Output:
@verbatim
@endverbatim
**/

#include <boost/units/base_dimension.hpp>

struct dimension : boost::units::base_dimension<dimension, 1> {
    typedef boost::units::base_dimension<dimension, 1> base;
};

int main() {
}
