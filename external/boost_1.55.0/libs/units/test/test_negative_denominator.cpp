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
    
\brief test_negative_denominator.cpp

\details
Test negative denominator for static_rational class.

Output:
@verbatim
@endverbatim
**/

#include <boost/units/static_rational.hpp>

#include <boost/mpl/assert.hpp>
#include <boost/type_traits/is_same.hpp>

BOOST_MPL_ASSERT((boost::is_same<boost::units::static_rational<-2,1>::type, boost::units::static_rational<2, -1>::type>));

int main() {
}
