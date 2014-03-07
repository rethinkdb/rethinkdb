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
    
\brief fail_adl_detail.cpp

\details
make sure that namespace detail is not pulled in by ADL.

Output:
@verbatim
@endverbatim
**/

#include <boost/units/quantity.hpp>
#include <boost/units/systems/si.hpp>

namespace bu = boost::units;

namespace boost {
namespace units {
namespace detail {

    template<class T>
    void this_function_should_not_be_found_by_adl(const T&) {};

}
}
}

int main(int argc,char *argv[])
{

    bu::quantity<bu::si::energy> q;

    this_function_should_not_be_found_by_adl(q);

    return 0;
}
