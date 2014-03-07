/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// test_traits_pass.cpp: test implementation level trait

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// compile test for traits
#include "test_tools.hpp"

#include <boost/serialization/level.hpp>
#include <boost/serialization/tracking.hpp>
#include <boost/serialization/version.hpp>

class B
{
};

BOOST_CLASS_IMPLEMENTATION(B, boost::serialization::object_class_info)
BOOST_CLASS_VERSION(B, 2)
BOOST_CLASS_TRACKING(B, boost::serialization::track_always)

int
test_main( int argc, char* argv[] )
{
    return EXIT_SUCCESS;
}

// EOF
