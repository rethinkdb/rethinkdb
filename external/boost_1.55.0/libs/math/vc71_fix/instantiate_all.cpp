//  Copyright John Maddock 2006.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//
// MSVC-7.1 has a problem with our tests: sometimes when a 
// function is used via a function pointer, it does *not*
// instantiate the template, leading to unresolved externals
// at link time.  Therefore we create a small library that
// instantiates "everything", and link all our tests against
// it for msvc-7.1 only.  Note that due to some BBv2 limitations
// we can not place this in a sub-folder of the test directory
// as that would lead to recursive project dependencies...
//

#define BOOST_MATH_ASSERT_UNDEFINED_POLICY false
#define BOOST_MATH_INSTANTIATE_MINIMUM
#include <boost/math/concepts/real_concept.hpp>
#include "../test/compile_test/instantiate.hpp"

void some_proc()
{
   instantiate(float(0));
   instantiate(double(0));
   instantiate(static_cast<long double>(0));
   instantiate(static_cast<boost::math::concepts::real_concept>(0));
}



