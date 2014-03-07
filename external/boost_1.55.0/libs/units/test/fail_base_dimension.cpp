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
    
\brief fail_base_dimension.cpp

\details
make sure that trying to use the same ordinal for multiple
base dimensions fails.

Output:
@verbatim
@endverbatim
**/

#include <boost/units/base_dimension.hpp>

struct dim1 : boost::units::base_dimension<dim1, 1> {};
struct dim2 : boost::units::base_dimension<dim2, 1> {};

int main()
{
}
