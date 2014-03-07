
//  Copyright John Maddock 2011.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//
// This tests two things: that e_float meets our
// conceptual requirements, and that we can instantiate
// all our distributions and special functions on this type.
//
#define BOOST_MATH_ASSERT_UNDEFINED_POLICY false
#define TEST_MPFR

#ifdef _MSC_VER
#  pragma warning(disable:4800)
#  pragma warning(disable:4512)
#  pragma warning(disable:4127)
#  pragma warning(disable:4512)
#  pragma warning(disable:4503) // decorated name length exceeded, name was truncated
#endif

#define E_FLOAT_TYPE_EFX

#include <boost/math/bindings/e_float.hpp>

#include <boost/math/concepts/real_type_concept.hpp>
#include "compile_test/instantiate.hpp"

void foo()
{
   instantiate(boost::math::ef::e_float());
}

int main()
{
   BOOST_CONCEPT_ASSERT((boost::math::concepts::RealTypeConcept<boost::math::ef::e_float>));
}


