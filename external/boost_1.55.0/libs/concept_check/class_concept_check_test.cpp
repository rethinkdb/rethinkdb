// (C) Copyright Jeremy Siek 2000.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/concept_check.hpp>

/*

  This file verifies that the BOOST_CLASS_REQUIRE macro of the Boost
  Concept Checking Library does not cause errors when it is not suppose
  to.

*/

struct foo { bool operator()(int) { return true; } };
struct bar { bool operator()(int, char) { return true; } };

class class_requires_test
{
    BOOST_CONCEPT_ASSERT((boost::EqualityComparable<int>));
    typedef int* int_ptr; typedef const int* const_int_ptr;
    BOOST_CONCEPT_ASSERT((boost::EqualOp<int_ptr,const_int_ptr>));
    BOOST_CONCEPT_ASSERT((boost::UnaryFunction<foo,bool,int>));
    BOOST_CONCEPT_ASSERT((boost::BinaryFunction<bar,bool,int,char>));
};

int
main()
{
    class_requires_test x;
    boost::ignore_unused_variable_warning(x);
    return 0;
}
